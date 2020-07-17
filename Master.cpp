//
// Created by marios on 25/5/20.
//

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <csignal>
#include "StatisticsElement.h"
#include "PatientEntry.h"
#include "GenericlLinkedList.h"
#include "RedBlackTrees.h"
#include "NamedPipe.h"
#include "NamedPipeInOut.h"
#include "MyString.h"
#include "PipesHashTable.h"
#include "GenericHashTable.h"
#include "Master.h"

using namespace exercise3namespace;

Master::Master(const char *init_directory, int workerCount, int bufferSize,
               const char* serverIP, int serverPort,
               GenericLinkedList<MyString>* pipeNames, PipesHashTable* pipesHashTable):
               serverPort(serverPort),
               serverIp(serverIP){

    this->pipeNames = pipeNames;
    this->pipesHashTable = pipesHashTable;
    this->init_directory = new char[strlen(init_directory) + 1];
    strcpy(this->init_directory, init_directory);
    this->workerCount = workerCount;
    this->bufferSize = bufferSize;

    pipes = new NamedPipe*[workerCount];
    uint counter = 0;
    for (GenericLinkedListIterator<MyString> pipeNameIterator(*pipeNames); !pipeNameIterator.End(); pipeNameIterator++){
        const char* pipeName = (*(pipeNameIterator.GetElementOnIterator())).data;
        PipesHashTable::ReturnPipeElement* pipeElement = pipesHashTable->GetElement(pipeName);
        NamedPipe* pipe = new NamedPipe(pipeName, pipeElement->processId);
        pipesHashTable->insertPipeNamedPipeIndex(pipeName, counter);
        pipes[counter++] = pipe;
        delete pipeElement;
        //printf("pipeName:%s\n", pipe->pipeName);
    }
    initializeWorkers();
}

void Master::UpdateWorker(const char *previousPipeNane, const char *newPipeName){
    PipesHashTable::ReturnPipeElement* pipeElement = pipesHashTable->GetElement(previousPipeNane);
    pipesHashTable->insertPipeNamedPipeIndex(newPipeName, pipeElement->namedPipeIndex);
    delete pipeElement;
    pipeElement = pipesHashTable->GetElement(newPipeName);
    pipes[pipeElement->namedPipeIndex]->changeIdentity(newPipeName, pipeElement->processId);

    char buffer[1024] = "\0";
    char numberStr[24];
    sprintf(numberStr, "%d", bufferSize);

    strcat(buffer, numberStr);
    strcat(buffer, "\n");
    strcat(buffer, "NO");
    strcat(buffer, "\n");
    for (GenericLinkedListIterator<char*> countryNameIterator(pipes[pipeElement->namedPipeIndex]->Countries); !countryNameIterator.End(); countryNameIterator++) {
        char fullPath[1024];
        strcpy(fullPath, init_directory);
        strcat(fullPath, "/");
        strcat(fullPath, *(countryNameIterator.GetElementOnIterator()));
        strcat(buffer, *(countryNameIterator.GetElementOnIterator()));
        strcat(buffer, "\n");
        strcat(buffer, fullPath);
        strcat(buffer, "\n");
    }

    NamedPipeInOut namedPipeInOut(pipes[pipeElement->namedPipeIndex]->pipeName);
    //printf("PipeName:%s\n", pipes[pipeElement->namedPipeIndex]->pipeName);
    namedPipeInOut.sendBuffer(buffer);

    delete pipeElement;
}

void Master::initializeWorkers(){
    int dir_count = 0;
    struct dirent* dent;
    DIR* srcdir = opendir(init_directory);

    while((dent = readdir(srcdir)) != NULL)
    {
        struct stat st;

        if(strcmp(dent->d_name, ".") == 0 || strcmp(dent->d_name, "..") == 0)
            continue;

        if (fstatat(dirfd(srcdir), dent->d_name, &st, 0) < 0)
        {
            continue;
        }

        if (S_ISDIR(st.st_mode))
        {
            pipes[dir_count % workerCount]->addCountry(dent->d_name);
            //printf("%d %s", dir_count % workerCount, dent->d_name);
            dir_count++;
        }
    }
    closedir(srcdir);

    for (uint workerIndex = 0; workerIndex < workerCount; workerIndex++)
    {
        char buffer[1024] = "\0";
        char numberStr[24];
        sprintf(numberStr, "%d", bufferSize);
        strcat(buffer, numberStr);
        strcat(buffer, "\n");
        strcat(buffer, "YES");
        strcat(buffer, "\n");
        strcat(buffer, serverIp.data);
        strcat(buffer, "\n");
        sprintf(numberStr, "%d", serverPort);
        strcat(buffer, numberStr);
        strcat(buffer, "\n");
        for (GenericLinkedListIterator<char*> countryNameIterator(pipes[workerIndex]->Countries); !countryNameIterator.End(); countryNameIterator++) {
            char fullPath[1024];
            strcpy(fullPath, init_directory);
            strcat(fullPath, "/");
            strcat(fullPath, *(countryNameIterator.GetElementOnIterator()));
            strcat(buffer, *(countryNameIterator.GetElementOnIterator()));
            strcat(buffer, "\n");
            strcat(buffer, fullPath);
            strcat(buffer, "\n");
        }
        NamedPipeInOut namedPipeInOut(pipes[workerIndex]->pipeName);
        //printf("Master Sending Buffer:\n%s", buffer);
        namedPipeInOut.sendBuffer(buffer);
    }
}

Master::~Master()
{
    delete[] this->init_directory;
    for (uint pipeIndex = 0; pipeIndex < workerCount; pipeIndex++)
    {
        delete pipes[pipeIndex];
    }
    delete[] pipes;
}

bool Master::KillWorkers() {
    sigset_t mask;
    sigfillset(&mask);
    sigprocmask(SIG_SETMASK, &mask, NULL);
    for (uint i = 0; i < workerCount; i++){
        kill(pipes[i]->processId, SIGKILL);
    }
    return true;
}