#define _GNU_SOURCE /* See feature_test_macros(7) */
#include <stdio.h>
#include <stdlib.h>
#include "server.h"


struct node_item;

typedef struct node_item{
    struct node_item *prev;
    struct node_item *next;
    char * hostname;
    uint64_t num_cpus;
    bool free;
}node_item_t;


typedef struct{
    node_item_t *first;
    node_item_t *last;
    size_t size;
}node_list_t;

void node_free(node_item_t *node){
    free(node->hostname);
}
node_item_t * node_create(){
    node_item_t *node = malloc(sizeof(node_item_t));
    node->hostname = NULL;
    node->num_cpus = 0;
    node->next = NULL;
    node->prev = NULL;
    node->free = true;
    return node;
}


node_list_t * node_list_create(){
    node_list_t * list = malloc(sizeof(node_list_t));
    list->size = 0;
    list->first = NULL;
    list->last = NULL;
    return list;
}

void node_list_free(node_list_t *list){
    node_item_t *item;
    while(list->first != NULL){
        item = list->first;
        list->first = item->next;
        list->size --;
        node_free(item);
    }
    free(list);
}

void node_list_append(node_list_t *list, node_item_t *item){
    if(NULL == list->last){
        list->first = list->last  = item;
    }else{
        list->last->next = item;
    }
    item->prev = list->last;
    item->next = NULL;
    list->last = item;
    list->size++;
}

void node_list_prepend(node_list_t *list, node_item_t *item){
    if(NULL == list->first){
        list->first = list->last  = item;
    }else{
        list->last->next = item;
    }
    item->next = list->first;
    item->prev = NULL;
    list->first = item;
    list->size--;
}

node_item_t * node_list_remove(node_list_t *list, char * hostname){
    node_item_t *item;
    item = list->first;
    while(item->next != NULL){
        if(0 == strcmp(item->hostname, hostname)){
            item->prev->next = item->next;
            item->next->prev = item->prev;
            list->size--;
            return item;
        }
        item = item->next;
    }
    return NULL;
}


pmix_status_t pmix_server_alloc_fn(const pmix_proc_t *client,
                                   pmix_alloc_directive_t directive,
                                   const pmix_info_t data[], size_t ndata,
                                   pmix_info_cbfunc_t cbfunc, void *cbdata);


static pmix_server_module_t pmix_server = {
    .client_connected = NULL,
    .client_finalized = NULL,
    .abort = NULL,
    .fence_nb = NULL,
    .direct_modex = NULL,
    .publish = NULL,
    .lookup = NULL,
    .unpublish = NULL,
    .spawn = NULL,
    .connect = NULL,
    .disconnect = NULL,
    .register_events = NULL,
    .deregister_events = NULL,
    .notify_event = NULL,
    .query = NULL,
    .tool_connected = NULL,
    .log = NULL,
    .job_control = NULL,
    .iof_pull = NULL,
    .push_stdin = NULL,
    .group = NULL,
    .allocate = pmix_server_alloc_fn,
#if PMIX_NUMERIC_VERSION >= 0x00050000
    .session_control = NULL
#endif
};


node_list_t *node_pool;
static int request_counter = 1;

typedef struct{
    pmix_info_t *info;
    size_t ninfo;
}info_struct_t;


static void release_info(void *cbdata){
    info_struct_t * info_struct = (info_struct_t *) cbdata;

    PMIX_INFO_FREE(info_struct->info, info_struct->ninfo);
    free(info_struct);
}

/* Stub for DYNPM reconf command callback */
static int request_nodes_callback(int alloc_id, char *nodelist, char *cpulist, pmix_info_cbfunc_t cbfunc, void *cbdata){
    char alloc_id_string[64];
    pmix_info_t *info;

    sprintf(alloc_id_string, "%d", alloc_id);
    
    PMIX_INFO_CREATE(info, 3);
    PMIX_INFO_LOAD(&info[0], PMIX_ALLOC_ID, alloc_id_string, PMIX_STRING);
    PMIX_INFO_LOAD(&info[1], PMIX_ALLOC_NODE_LIST, nodelist, PMIX_STRING);
    PMIX_INFO_LOAD(&info[2], PMIX_ALLOC_NUM_CPU_LIST, cpulist, PMIX_STRING);

    info_struct_t * info_struct = (info_struct_t *) malloc(sizeof(info_struct_t));
    info_struct->info = info;
    info_struct->ninfo = 3;

    cbfunc(PMIX_SUCCESS, info, 3, cbdata, release_info, info_struct);

    return 0;
}

/* Stub for DYNPM active request */
static int request_nodes(int alloc_id, uint64_t num_nodes, uint64_t num_cpus, pmix_info_cbfunc_t cbfunc, void *cbdata){

    char ** nodes = NULL, **cpus = NULL;
    char *alloc_node_list, *alloc_cpu_list;
    char cpu[64];
    node_item_t *node;
    uint64_t assigned_nodes = 0, assigned_cpus = 0;

    node = node_pool->first;
    while(node != NULL){
        if(node->free){
            sprintf(cpu, "%lu", node->num_cpus);
            PMIX_ARGV_APPEND_NOSIZE_COMPAT(&cpus, cpu);
            PMIX_ARGV_APPEND_NOSIZE_COMPAT(&nodes, node->hostname);
            assigned_nodes++;
            assigned_cpus+=node->num_cpus;
            node->free = false;
        }
        if((0 != num_nodes && assigned_nodes >= num_nodes) ||
            (0 != num_cpus && assigned_cpus >= num_cpus)){
            break;
        }
        node = node->next;
    }

    if(assigned_nodes < num_nodes || assigned_cpus < num_cpus){
        printf("    SCHEDULER: COULD NOT SATISFY ALLOCATION REQUEST\n\n");
        return PMIX_ERR_OUT_OF_RESOURCE;
    }

    alloc_node_list = PMIX_ARGV_JOIN_COMPAT(nodes, ',');
    alloc_cpu_list = PMIX_ARGV_JOIN_COMPAT(cpus, ',');
    PMIX_ARGV_FREE_COMPAT(nodes);
    PMIX_ARGV_FREE_COMPAT(cpus);

    printf("    SCHEDULER: COULD SATISFY ALLOCATION REQUEST: SESSION_ID=%d, NEW_NODES=%s, NEW_CPUS=%s\n\n", alloc_id, alloc_node_list, alloc_cpu_list);
    request_nodes_callback(alloc_id, alloc_node_list, alloc_cpu_list, cbfunc, cbdata);

    free(alloc_node_list);
    free(alloc_cpu_list);

    return PMIX_SUCCESS;
}

pmix_status_t pmix_server_alloc_fn(const pmix_proc_t *client,
                                   pmix_alloc_directive_t directive,
                                   const pmix_info_t data[], size_t ndata,
                                   pmix_info_cbfunc_t cbfunc, void *cbdata){

    size_t n;
    uint64_t num_cpus, num_nodes;
    int rc, alloc_id = -1;

    if(directive != PMIX_ALLOC_NEW && directive != PMIX_ALLOC_EXTEND){
        printf("    SCHEDULER: RECEIVED ALLOCATION REQUEST WITH UNSUPPORTED DIRECTIVE: %u\n\n", directive);
        cbfunc(PMIX_ERR_NOT_SUPPORTED, NULL, 0, cbdata, NULL, NULL);
    }

    for(n = 0; n < ndata; n++){
        if(PMIX_CHECK_KEY(&data[n], PMIX_ALLOC_NUM_CPUS)){
            num_cpus = data[n].value.data.uint64;
            /* Todo: request cpus instead of nodes 
                Default: 8 PROCS / NODE 
            */

            num_nodes = num_cpus % 8 != 0 ? 1 : 0;
            num_nodes += num_cpus / 8;
        }else if(PMIX_CHECK_KEY(&data[n], PMIX_ALLOC_NUM_NODES)){
            num_nodes = data[n].value.data.uint64;
        }else if(PMIX_CHECK_KEY(&data[n], PMIX_ALLOC_ID)){
        	alloc_id = atoi(data[n].value.data.string);
        }
    }

    printf("    SCHEDULER: RECEIVED ALLOCATION REQUEST WITH DIRECTIVE: %u for SESSION: %d\n", directive, alloc_id);

    if((0 == num_nodes && 0 == num_cpus) || alloc_id == -1){
        printf("    SCHEDULER: ALLOCATION REQUEST IS INVALID\n\n");
        cbfunc(PMIX_ERR_BAD_PARAM, NULL, 0, cbdata, NULL, NULL);
        return PMIX_SUCCESS;
    }
    
    if(directive == PMIX_ALLOC_NEW){
    	alloc_id = ++request_counter;
    }
    
    if(PMIX_SUCCESS != (rc = request_nodes(alloc_id, num_nodes, num_cpus, cbfunc, cbdata))){
        cbfunc(rc, NULL, 0, cbdata, NULL, NULL);    
    }
    
    return PMIX_SUCCESS;
}


int main(int argc, char **argv)
{
    pmix_status_t rc;
    int n, num_procs, num_children;
    char *initial_hosts, *num_procs_arg;
    char ** nodes, **tmp;
    char cmd[1024], pid_str[256], children[64];
    pmix_nspace_t nspace;
    char my_nspace[] = "my_nspace@5", hostname[] = "n01", pid_file[] = "./pid.txt";
    pmix_rank_t my_rank = 0;
    node_item_t *node_item;
    FILE *file;
    pmix_proc_t proc;
    pmix_app_t *app;
    pmix_info_t *job_info, *info;
    pid_t pid;

    if (argc < 5){
        printf("Not enough arguments specified\n");
        exit(1);
    }

    node_pool = node_list_create();
    

    nodes = PMIX_ARGV_SPLIT_COMPAT(argv[1], ',');
    for (n = 0; n < PMIX_ARGV_COUNT_COMPAT(nodes); n++){
        tmp = PMIX_ARGV_SPLIT_COMPAT(nodes[n], ':');
        node_item = node_create();
        node_item->hostname = strdup(tmp[0]);
        node_item->num_cpus = PMIX_ARGV_COUNT_COMPAT(tmp) > 1 ? atoi(tmp[1]) : 1;
        node_list_append(node_pool, node_item);
    }
    PMIX_ARGV_FREE_COMPAT(nodes);
    initial_hosts = argv[2];
    nodes = PMIX_ARGV_SPLIT_COMPAT(initial_hosts, ',');

    for (n = 0; n < PMIX_ARGV_COUNT_COMPAT(nodes); n++){
        tmp = PMIX_ARGV_SPLIT_COMPAT(nodes[n], ':');
        node_item = node_pool->first;
        while(node_item != NULL){
            if(0 == strcmp(node_item->hostname, tmp[0])){
                node_item->free = false;
                free(tmp);
                break;
            }
            node_item = node_item->next;
        }
    }
    PMIX_ARGV_FREE_COMPAT(nodes);
    
    num_children = atoi(argv[3]);
    sprintf(children, "%d", num_children - 1);
    num_procs_arg = argv[4];
    num_procs = atoi(num_procs_arg);

    printf("Starting PRRTE DVM on hosts %s\n\n", initial_hosts);
    sprintf(cmd, "prte --report-pid %s --daemonize --host %s", pid_file, initial_hosts);
    system(cmd);

    while(1){

        file = fopen(pid_file, "r");

        if (file != NULL) {
            fgets(pid_str, sizeof(pid_str), file);
            pid = atoi(pid_str);

            PMIX_INFO_CREATE(info, 7);
            PMIX_INFO_LOAD(&info[0], PMIX_SERVER_PIDINFO, &pid, PMIX_PID);
            PMIX_INFO_LOAD(&info[1], PMIX_SERVER_SCHEDULER, NULL, PMIX_BOOL);
            PMIX_INFO_LOAD(&info[2], PMIX_SERVER_REMOTE_CONNECTIONS, NULL, PMIX_BOOL);
            PMIX_INFO_LOAD(&info[3], PMIX_TOOL_NSPACE, my_nspace, PMIX_STRING);
            PMIX_INFO_LOAD(&info[4], PMIX_TOOL_RANK, &my_rank, PMIX_PROC_RANK);
            PMIX_INFO_LOAD(&info[5], PMIX_HOSTNAME, hostname, PMIX_STRING);
            PMIX_INFO_LOAD(&info[6], PMIX_FWD_STDOUT, NULL, PMIX_BOOL);            
            
            rc = PMIx_tool_init(&proc, info, 7);
            PMIX_INFO_FREE(info, 7);
            fclose(file);
            break;
        } else {
            printf("File does not exist yet: %s\n", pid_file);
        }
        sleep(1);
    }

    rc = PMIx_tool_set_server_module(&pmix_server);


    printf("Starting spawner job 1 of %d with %d procs\n\n", num_children, num_procs);

    PMIX_APP_CREATE(app, 1);
    if (0 > asprintf(&app->cmd, "./spawner")) {
        exit(1);
    }
    app->maxprocs = num_procs;
    PMIx_Argv_append_nosize(&app->argv, children);
    PMIx_Argv_append_nosize(&app->argv, num_procs_arg);

    PMIX_INFO_CREATE(job_info, 1);
    PMIX_INFO_LOAD(&job_info[0], PMIX_DISPLAY_MAP, NULL, PMIX_BOOL);

    if (PMIX_SUCCESS != (rc = PMIx_Spawn(job_info, 1, app, 1, nspace))) {
        printf("Spawn failed with rc=%d\n", rc);
        PMIX_INFO_FREE(job_info, 1);
        return -1;
    }
    PMIX_INFO_FREE(job_info, 1);

    n = 0;
    while(n < 10){
        sleep(1);
        ++n;
    }

    sprintf(cmd, "pterm --pid %u", pid);
    system(cmd);
    sprintf(cmd, "rm %s", pid_file);
    system(cmd);

    PMIx_tool_finalize();

    node_list_free(node_pool);

    return 0;
}
