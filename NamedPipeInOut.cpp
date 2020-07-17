//
// Created by marios on 18/5/20.
//
#include <unistd.h>
#include <cstdlib>
#include <cstdio>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <climits>
#include <cstring>
#include "NamedPipeInOut.h"

using namespace exercise3namespace;

NamedPipeInOut::NamedPipeInOut(const char* nOfPipe, uint bufferSize){
    this->bufferSize = bufferSize;
    nameOfPipe = new char[strlen(nOfPipe)];
    strcpy(nameOfPipe, nOfPipe);
}

NamedPipeInOut::~NamedPipeInOut(){
    delete[] nameOfPipe;
}

char* NamedPipeInOut::receiveBuffer()
{
    char* numberStr = new char[bufferSize];
    while (access(nameOfPipe, F_OK) == -1);
    int res = open(nameOfPipe, O_RDONLY);
    read(res, numberStr, bufferSize);

    uint byteCountOfBuffer = atoi(numberStr);
    uint iterationCount = byteCountOfBuffer % bufferSize ? byteCountOfBuffer / bufferSize  + 1 : byteCountOfBuffer / bufferSize;
    char* buffer = new char[iterationCount * bufferSize];
    for (int i = 0; i < iterationCount * bufferSize; i++)
        buffer[i] = '\0';
    uint offset = 0;
    for (uint iterationSending = 0; iterationSending < iterationCount; iterationSending++) {
        read(res, buffer + offset, bufferSize);
        offset += bufferSize;
    }

    delete[] numberStr;
    close(res);
    return buffer;
}

void NamedPipeInOut::sendBuffer(const char* buffer){
    if (access(nameOfPipe, F_OK) == -1) {
        int res = mkfifo(nameOfPipe, 0777);
        if (res != 0) {
            fprintf(stderr, "Could not create fifo %s\n", nameOfPipe);
            exit(EXIT_FAILURE);
        }
    }
    uint byteCountOfBuffer = strlen(buffer) + 1;
    char* numberStr = new char[bufferSize];

    sprintf(numberStr, "%d", byteCountOfBuffer);
    int res = open(nameOfPipe, O_WRONLY); /* open fifo in read-only mode */
    write(res, numberStr, bufferSize);
    uint iterationCount = byteCountOfBuffer % bufferSize ? byteCountOfBuffer / bufferSize  + 1 : byteCountOfBuffer / bufferSize;
    uint offset = 0;
    for (uint iterationSending = 0; iterationSending < iterationCount; iterationSending++)
    {
        write(res, buffer + offset, bufferSize);
        offset += bufferSize;
    }

    close(res);
    unlink(nameOfPipe);
    delete[] numberStr;
}