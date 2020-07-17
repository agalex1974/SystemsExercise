//
// Created by marios on 18/6/20.
//

#include <cstdlib>
#include <cstdio>
#include <pthread.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "MyString.h"
#include "SendReceiveProtocol.h"
#include "Client.h"

using namespace exercise3namespace;

void printUsage(){
    printf("Usage: whoClient -q queryFile -w numThreads -sp serverPorty -sip serverIP\n");
}

int main(int argc, char** argv) {
    if (argc != 9){
        printf("Error in input\n");
        printUsage();
        return 1;
    }


    if (strcmp(argv[1], "-q"))
    {
        printf("Error in input -q does not exist\n");
        printUsage();
        return 1;
    }
    MyString queryFile(argv[2]);

    int numThreads;
    if (strcmp(argv[3], "-w"))
    {
        printf("Error in input -w does not exist\n");
        printUsage();
        return 1;
    }
    numThreads = atoi(argv[4]);

    int serverPort;
    if (strcmp(argv[5], "-sp"))
    {
        printf("Error in input -sp does not exist\n");
        printUsage();
        return 1;
    }
    serverPort = atoi(argv[6]);

    if (strcmp(argv[7], "-sip"))
    {
        printf("Error in input -sip does not exist\n");
        printUsage();
        return 1;
    }
    MyString serverIP(argv[8]);

    Client client(numThreads, queryFile.data, serverPort, serverIP.data);
    client.execute_query_file();
}

