# Tests for tracking allocations in PRRTE
This repo provides simple tests for the allocation tracking in PRRTE.
It runs a scheduler which connects to PRRTE, spawns the 'spawner' job and asnwers allocation request.
The spawner job:
    * requests to extend its own allocation
    * requests a new allocation
    * spawns a new spawner job in the new allocation

## Running the tests
export path to PMIx installation:
```
export PMIX_ROOT=/path/to/pmix
```
Build the executables:
```
make
```
Run the server:
```
./server [arg0] [arg1] [arg2] [arg3]
    arg0: node_pool         - nodes available for scheduling 
    arg1: initial_hosts     - host initially used by the PRRTE DVM 
    arg2: recursive_jobs    - number of spawner jobs to be run (total jobs: recursive_jobs + 1) 
    arg3: num procs         - number of processes per spawner job 
```
## Example
```
./server  n01:8,n02:8,n03:8,n04:8,n05:8,n06:8,n07:8,n08:8,n09:8 n01:8,n02:8 3 8
```
Runs the scheduler with 9 nodes available for scheduling, starts the PRRTE DVM on the first two nodes and recursively spawns 3 spawner jobs a 8 processes.

Output:
```
[mpiuser@n01 pmix_allocation_tests]$ ./server n01:8,n02:8,n03:8,n04:8,n05:8,n06:8,n07:8,n08:8,n09:8 n01:8,n02:8 3 8
Starting PRRTE DVM on hosts n01:8,n02:8

Starting 8 procs with executable ./spawner

    SCHEDULER: RECEIVED ALLOCATION REQUEST WITH DIRECTIVE: 2 for SESSION: 0
    SCHEDULER: COULD SATISFY ALLOCATION REQUEST: SESSION_ID=0, NEW_NODES=n03, NEW_CPUS=8

    SCHEDULER: RECEIVED ALLOCATION REQUEST WITH DIRECTIVE: 1 for SESSION: 0
    SCHEDULER: COULD SATISFY ALLOCATION REQUEST: SESSION_ID=2, NEW_NODES=n04, NEW_CPUS=8

    SCHEDULER: RECEIVED ALLOCATION REQUEST WITH DIRECTIVE: 2 for SESSION: 2
    SCHEDULER: COULD SATISFY ALLOCATION REQUEST: SESSION_ID=2, NEW_NODES=n05, NEW_CPUS=8

    SCHEDULER: RECEIVED ALLOCATION REQUEST WITH DIRECTIVE: 1 for SESSION: 2
    SCHEDULER: COULD SATISFY ALLOCATION REQUEST: SESSION_ID=3, NEW_NODES=n06, NEW_CPUS=8

    SCHEDULER: RECEIVED ALLOCATION REQUEST WITH DIRECTIVE: 2 for SESSION: 3
    SCHEDULER: COULD SATISFY ALLOCATION REQUEST: SESSION_ID=3, NEW_NODES=n07, NEW_CPUS=8

    SCHEDULER: RECEIVED ALLOCATION REQUEST WITH DIRECTIVE: 1 for SESSION: 3
    SCHEDULER: COULD SATISFY ALLOCATION REQUEST: SESSION_ID=4, NEW_NODES=n08, NEW_CPUS=8

    SCHEDULER: RECEIVED ALLOCATION REQUEST WITH DIRECTIVE: 2 for SESSION: 4
    SCHEDULER: COULD SATISFY ALLOCATION REQUEST: SESSION_ID=4, NEW_NODES=n09, NEW_CPUS=8

TERMINATING DVM...DONE
```

Additional output from the spawner jobs is written in files `spawner-[nspace].txt`.

NOTE: The scheduling loop is currently hardcoded to run for 10 seconds.