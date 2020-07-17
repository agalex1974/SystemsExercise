#include <iostream>
#include <unistd.h>
#include <cstdlib>
#include <cstdio>
#include <csignal>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <cstring>
#include <climits>
#include <sys/wait.h>
#include "NamedPipeInOut.h"
#include "PatientEntry.h"
#include "MyString.h"
#include "StatisticsElement.h"
#include "GenericlLinkedList.h"
#include "RedBlackTrees.h"
#include "NamedPipe.h"
#include "PipesHashTable.h"
#include "StatisticsElement.h"
#include "GenericHashTable.h"
#include "Master.h"

using namespace exercise3namespace;

GenericLinkedList<MyString>* pipeNames;
PipesHashTable* pipesHashTable;
Master* master;

static void execute(const char* pgm)
{
    /* this is the exec syscall. it overwrites the current process if successful
     * we start the shell so that we have a set PATH variable
     */
    (void) execl(pgm, (char *)0);

    perror("execlp failed");
    exit(EXIT_FAILURE);
}

void worker_exit()
{
    int wstat;
    pid_t	pid;

    while (true) {
        pid = wait3 (&wstat, WNOHANG, (struct rusage *)NULL);
        if (pid == 0)
            return;
        else if (pid == -1)
            return;
        else{
            int fork_result = fork();
            if (fork_result == -1) {
                fprintf(stderr, "fork error");
                exit(EXIT_FAILURE);
            }
            if (fork_result == 0) {
                execute("./Worker");
            }
            else{
                printf("A worker was destroyed restoring system worker integrity!\n");
                MyString namePipe(64);
                MyString previousPipeName(64);
                sprintf(namePipe.data, "fifo%d", fork_result);
                sprintf(previousPipeName.data, "fifo%d", pid);
                if (pipeNames) pipeNames->Append(namePipe);
                if (pipesHashTable)
                {
                    pipesHashTable->NegateState(previousPipeName.data);
                    pipesHashTable->insertPipeProcessId(namePipe.data, fork_result);
                }
                if (master) master->UpdateWorker(previousPipeName.data, namePipe.data);
            }
        }
    }
}

void printUsage(){
    printf("Usage: Master -w numWorkers -b bufferSize -s serverIP -p port -i input_dir\n");
}

int main(int argc, char** argv) {
    if (argc != 11){
        printf("Error in input\n");
        printUsage();
        return 1;
    }

    int numWorkers;
    if (strcmp(argv[1], "-w"))
    {
        printf("Error in input -w does not exist\n");
        printUsage();
        return 1;
    }
    numWorkers = atoi(argv[2]);

    int bufferSize;
    if (strcmp(argv[3], "-b"))
    {
        printf("Error in input -b does not exist\n");
        printUsage();
        return 1;
    }
    bufferSize = atoi(argv[4]);

    if (strcmp(argv[5], "-s"))
    {
        printf("Error in input -s does not exist\n");
        printUsage();
        return 1;
    }
    MyString serverIP(argv[6]);

    int port;
    if (strcmp(argv[7], "-p"))
    {
        printf("Error in input -p does not exist\n");
        printUsage();
        return 1;
    }
    port = atoi(argv[8]);

    if (strcmp(argv[9], "-i"))
    {
        printf("Error in input -i does not exist\n");
        printUsage();
        return 1;
    }
    MyString inputDir(argv[10]);

    int aggregatorId = getpid();
    //signal(SIGCHLD, (__sighandler_t)(worker_exit));
    pipeNames = new GenericLinkedList<MyString>;
    pipesHashTable = new PipesHashTable(111);
    for (int i = 0; i < numWorkers; i++)
    {
        if (getpid() == aggregatorId) {
            int fork_result = fork();
            if (fork_result == -1) {
                fprintf(stderr, "fork error");
                exit(EXIT_FAILURE);
            }
            if (fork_result == 0) {
                execute("./Worker");
                exit(0);
            }
            else{
                MyString namePipe(64);
                sprintf(namePipe.data, "fifo%d", fork_result);
                pipeNames->Append(namePipe);
                pipesHashTable->insertPipeProcessId(namePipe.data, fork_result);
            }
        }
    }

    //Change this accordingly
    master = new Master(inputDir.data, numWorkers, bufferSize, serverIP.data, port, pipeNames, pipesHashTable);

    int status;
    while (wait(&status) > 0);
    delete pipesHashTable;
    delete pipeNames;
    delete master;
    //delete diseaseAggregator;
}
