//
// Created by marios on 24/5/20.
//
#include <iostream>
#include <unistd.h>
#include <cstdlib>
#include <cstdio>
#include <fcntl.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <cstring>
#include <climits>
#include "NamedPipeInOut.h"
#include "SendReceiveProtocol.h"
#include "PatientEntry.h"
#include "GenericlLinkedList.h"
#include "GenericHashTable.h"
#include "MyString.h"
#include "RedBlackTrees.h"
#include "Worker.h"

using namespace exercise3namespace;

int main(){
    char namePipe[30];
    sprintf(namePipe, "fifo%d", getpid());
    NamedPipeInOut* namedPipeInOutInit = new NamedPipeInOut(namePipe);
    char* buffer = namedPipeInOutInit->receiveBuffer();
    //printf("receiving:\n%s\n", buffer);

    Worker worker(buffer, namedPipeInOutInit);
    worker.AcceptCommandsFromServer();
    delete namedPipeInOutInit;
    //sleep(1);
    //unlink(namePipe);
    delete[] buffer;
}