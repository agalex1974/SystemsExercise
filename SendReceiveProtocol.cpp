//
// Created by marios on 23/6/20.
//

#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "SendReceiveProtocol.h"

using namespace exercise3namespace;

char* SendReceiveProtocol::receive(int socket)
{
    if (socket < 0){
        socket = embeddedSocket;
    }
    char* numberStr = new char[bufferSize];
    read(socket, numberStr, bufferSize);

    uint byteCountOfBuffer = atoi(numberStr);
    uint iterationCount = byteCountOfBuffer % bufferSize ? byteCountOfBuffer / bufferSize  + 1 : byteCountOfBuffer / bufferSize;
    char* buffer = new char[iterationCount * bufferSize];
    for (int i = 0; i < iterationCount * bufferSize; i++)
        buffer[i] = '\0';
    uint offset = 0;
    for (uint iterationSending = 0; iterationSending < iterationCount; iterationSending++) {
        read(socket, buffer + offset, bufferSize);
        offset += bufferSize;
    }

    delete[] numberStr;

    return buffer;
}

void SendReceiveProtocol::send(int socket, const char *buffer) {

    if (socket < 0){
        socket = embeddedSocket;
    }

    uint byteCountOfBuffer = strlen(buffer) + 1;
    char* numberStr = new char[bufferSize];

    sprintf(numberStr, "%d", byteCountOfBuffer);
    write(socket, numberStr, bufferSize);
    uint iterationCount = byteCountOfBuffer % bufferSize ? byteCountOfBuffer / bufferSize  + 1 : byteCountOfBuffer / bufferSize;
    uint offset = 0;
    for (uint iterationSending = 0; iterationSending < iterationCount; iterationSending++)
    {
        write(socket, buffer + offset, bufferSize);
        offset += bufferSize;
    }
    delete[] numberStr;
}
