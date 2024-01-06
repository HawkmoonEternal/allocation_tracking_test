#define _GNU_SOURCE /* See feature_test_macros(7) */
#include <stdio.h>
#include <stdlib.h>
#include "pmix.h"

int main(int argc, char **argv)
{
    pmix_status_t rc;
    pmix_value_t value;
    pmix_value_t *val = &value;
    pmix_proc_t proc, myproc;
    pmix_app_t *app;
    pmix_info_t *job_info;
    uint32_t nprocs;
    int num_children = 0;

    char *alloc_id, *hostlist, *cpus;
    uint64_t num_cpus = 16;
    pmix_info_t *alloc_info, *alloc_results;
    size_t n_alloc_results, n;
    
    FILE *fp;
    pid_t pid;
    char hostname[256], children[64], dir[1024], filename[1024], nsp2[PMIX_MAX_NSLEN + 1];
    sleep(1);



    pid = getpid();
    gethostname(hostname, 256);

    if (NULL == getcwd(dir, 1024)) {
        exit(1);
    }

    if(argc < 2) {
        exit(1);
    }
    
    num_children = atoi(argv[1]);
    sprintf(children, "%d", num_children -1);

    if(argc > 2){
        cpus = argv[2];
        num_cpus = atoi(cpus);
    }

    /* init us */
    if (PMIX_SUCCESS != (rc = PMIx_Init(&myproc, NULL, 0))) {
        fp = fopen("./spawner-init-error.txt", "a+");
        fprintf(fp, "PMIx_Init failed: %d\n", rc);
        fclose(fp);
        exit(0);
    }

    sprintf(filename, "./spawner-%s.txt", myproc.nspace);    
    fp = fopen(filename, "a+");
    fprintf(fp, "Client ns %s rank %d pid %lu: Running on host %s\n", myproc.nspace, myproc.rank, 
            (unsigned long) pid, hostname);
    fflush(fp);

    /* rank 0 prints out some info about the namespace */
    if(myproc.rank == 0){
        PMIX_PROC_CONSTRUCT(&proc);
        PMIX_LOAD_PROCID(&proc, myproc.nspace, PMIX_RANK_WILDCARD);

        /* get our universe size */
        if (PMIX_SUCCESS != (rc = PMIx_Get(&proc, PMIX_UNIV_SIZE, NULL, 0, &val))) {
            fprintf(fp, "Client ns %s rank %d: PMIx_Get universe size failed: %s\n", myproc.nspace,
                    myproc.rank, PMIx_Error_string(rc));
            goto done;
        }
        fprintf(fp, "Client %s:%d universe size %d\n", myproc.nspace, myproc.rank,
                val->data.uint32);
        PMIX_VALUE_RELEASE(val);

        /* get our job size */
        if (PMIX_SUCCESS != (rc = PMIx_Get(&proc, PMIX_JOB_SIZE, NULL, 0, &val))) {
            fprintf(fp, "Client ns %s rank %d: PMIx_Get job size failed: %d\n", myproc.nspace,
                    myproc.rank, rc);

            goto done;
        }
        nprocs = val->data.uint32;
        PMIX_VALUE_RELEASE(val);
        fprintf(fp, "Client %s rank %d: num procs in nspace %d\n", myproc.nspace, myproc.rank, nprocs);
    }

    /* rank 0 calls PMIx_Allocation_request + PMIx_Spawn (could also be other rank) */
    if (0 == myproc.rank) {

        PMIX_INFO_CREATE(alloc_info, 1);
        PMIX_INFO_LOAD(&alloc_info[0], PMIX_ALLOC_NUM_CPUS, &num_cpus, PMIX_UINT64);
        fprintf(fp, " ====> Client %s:%d CALLING PMIx_Allocation_request: EXPAND %ld CPUS\n", myproc.nspace, myproc.rank, num_cpus);
        fflush(fp);
        if (PMIX_SUCCESS != (rc = PMIx_Allocation_request(PMIX_ALLOC_EXTEND, alloc_info, 1, &alloc_results, &n_alloc_results))) {
            fprintf(fp, "Client ns %s rank %d: PMIx_Alloc failed: %d\n", myproc.nspace,
                    myproc.rank, rc);
            fflush(fp);
            goto done;
        }
        PMIX_INFO_FREE(alloc_info, 1);

        /* retrieve the alloc_id from the results. Note: The results will also contain the PMIX_ALLOC_NODE_LIST and PMIX_ALLOC_NUM_CPU list */
        for(n = 0; n < n_alloc_results; n++){
            if(PMIX_CHECK_KEY(&alloc_results[n], PMIX_ALLOC_ID)){
            	alloc_id = strdup(alloc_results[n].value.data.string);
            }else if(PMIX_CHECK_KEY(&alloc_results[n], PMIX_ALLOC_NODE_LIST)){
                hostlist = strdup(alloc_results[n].value.data.string);
            }
        }
        PMIX_INFO_FREE(alloc_results, n_alloc_results);
        fprintf(fp, " ====> Client %s:%d EXPANSION GRAMTED: SESSION_ID: %s, ALLOC_NODE_LIST: %s\n", myproc.nspace, myproc.rank, alloc_id, hostlist);
        free(alloc_id);
        free(hostlist);

        if(0 < num_children){

            PMIX_INFO_CREATE(alloc_info, 1);
            PMIX_INFO_LOAD(&alloc_info[0], PMIX_ALLOC_NUM_CPUS, &num_cpus, PMIX_UINT64);
            fprintf(fp, " ====> Client %s:%d CALLING PMIx_Allocation_request: NEW %ld CPUS\n", myproc.nspace, myproc.rank, num_cpus);
            fflush(fp);
            if (PMIX_SUCCESS != (rc = PMIx_Allocation_request(PMIX_ALLOC_NEW, alloc_info, 1, &alloc_results, &n_alloc_results))) {
                fprintf(fp, "Client ns %s rank %d: PMIx_Alloc failed: %d\n", myproc.nspace,
                        myproc.rank, rc);
                fflush(fp);
                goto done;
            }
            PMIX_INFO_FREE(alloc_info, 1);

            /* retrieve the alloc_id from the results. Note: The results will also contain the PMIX_ALLOC_NODE_LIST and PMIX_ALLOC_NUM_CPU list */
            for(n = 0; n < n_alloc_results; n++){
                if(PMIX_CHECK_KEY(&alloc_results[n], PMIX_ALLOC_ID)){
                	alloc_id = strdup(alloc_results[n].value.data.string);
                }else if(PMIX_CHECK_KEY(&alloc_results[n], PMIX_ALLOC_NODE_LIST)){
                    hostlist = strdup(alloc_results[n].value.data.string);
                }
            }
            PMIX_INFO_FREE(alloc_results, n_alloc_results);
            fprintf(fp, " ====> Client %s:%d NEW ALLOCATION GRANTED: SESSION_ID: %s, ALLOC_NODE_LIST: %s\n", myproc.nspace, myproc.rank, alloc_id, hostlist);

            PMIX_APP_CREATE(app, 1);
            if (0 > asprintf(&app->cmd, "%s/spawner", dir)) {
                exit(1);
            }
            app->maxprocs = num_cpus;
            PMIx_Argv_append_nosize(&app->argv, children);
            PMIx_Argv_append_nosize(&app->argv, cpus);

            app->env = (char **) malloc(2 * sizeof(char *));
            app->env[0] = strdup("PMIX_ENV_VALUE=3");
            app->env[1] = NULL;

            /* Add the retrived nodelist with the PMIX_ADD_HOST key */
            app->ninfo = 1;
            PMIX_INFO_CREATE(app->info, 1);
            PMIX_INFO_LOAD(&app->info[1], PMIX_HOST, hostlist, PMIX_STRING);

            PMIX_INFO_CREATE(job_info, 2);
            PMIX_INFO_LOAD(&job_info[0], PMIX_DISPLAY_MAP, NULL, PMIX_BOOL);
            PMIX_INFO_LOAD(&job_info[1], PMIX_ALLOC_ID, alloc_id, PMIX_STRING);

            fprintf(fp, " ====> Client ns %s rank %d: CALLING PMIx_Spawn WITH alloc_id %s\n", myproc.nspace, myproc.rank, alloc_id);
            fflush(fp);
            if (PMIX_SUCCESS != (rc = PMIx_Spawn(job_info, 2, app, 1, nsp2))) {
                fprintf(fp, "Client ns %s rank %d: PMIx_Spawn failed: %d\n", myproc.nspace,
                        myproc.rank, rc);
                goto done;
            }

            PMIX_APP_FREE(app, 1);
            PMIX_INFO_FREE(job_info, 2);

            fprintf(fp, " ===> Client %s:%d  SUCCESSFULLY SPAWNED JOB %s\n", myproc.nspace, myproc.rank, nsp2);
            fflush(fp);
        }
    }
    
done:

    sleep(5);
    /* finalize us */  
    if (PMIX_SUCCESS != (rc = PMIx_Finalize(NULL, 0))) {
        fprintf(fp, "Client ns %s rank %d:PMIx_Finalize failed: %d\n", myproc.nspace,
                myproc.rank, rc);
    } else {
        fprintf(fp, "Client ns %s rank %d:PMIx_Finalize successfully completed\n",
                myproc.nspace, myproc.rank);
    }
    fclose(fp);
    return (0);
}
