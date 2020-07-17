//
// Created by marios on 23/6/20.
//

#ifndef SYSTEMSEXERCISE3_SENDRECEIVEPROTOCOL_H
#define SYSTEMSEXERCISE3_SENDRECEIVEPROTOCOL_H

namespace exercise3namespace {
    class SendReceiveProtocol {
    private:
        using uint = unsigned int;
        int bufferSize;
        int embeddedSocket;
    public:
        void SetEmbeddedSocket(int socket){
            embeddedSocket = socket;
        }
        explicit SendReceiveProtocol(uint bufferSize):bufferSize(bufferSize){}
        void send(int socket, const char* buffer);
        char* receive(int socket);
    };
}

#endif //SYSTEMSEXERCISE3_SENDRECEIVEPROTOCOL_H
