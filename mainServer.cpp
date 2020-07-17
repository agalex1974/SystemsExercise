//
// Created by marios on 21/6/20.
//

#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>

#include "PatientEntry.h"
#include "MyString.h"
#include "StatisticsElement.h"
#include "GenericlLinkedList.h"
#include "RedBlackTrees.h"
#include "NamedPipe.h"
#include "PipesHashTable.h"
#include "SendReceiveProtocol.h"
#include "StatisticsElement.h"
#include "GenericHashTable.h"

#include "Server.h"

using namespace exercise3namespace;
void printUsage(){
    printf("Usage: whoServer -q queryPortNum -s statisticsPortNum -w numThreads -b buffersize\n");
}

int main(int argc, char** argv) {
    if (argc != 9){
        printf("Error in input\n");
        printUsage();
        return 1;
    }

    int qPort;
    if (strcmp(argv[1], "-q"))
    {
        printf("Error in input -q does not exist\n");
        printUsage();
        return 1;
    }
    qPort = atoi(argv[2]);

    int sPort;
    if (strcmp(argv[3], "-s"))
    {
        printf("Error in input -s does not exist\n");
        printUsage();
        return 1;
    }
    sPort = atoi(argv[4]);

    int numThreads;
    if (strcmp(argv[5], "-w"))
    {
        printf("Error in input -w does not exist\n");
        printUsage();
        return 1;
    }
    numThreads = atoi(argv[6]);

    int bufferSize;
    if (strcmp(argv[7], "-b"))
    {
        printf("Error in input -b does not exist\n");
        printUsage();
        return 1;
    }
    bufferSize = atoi(argv[8]);

    Server server(qPort, sPort, numThreads, bufferSize);
    server.Listen();
}