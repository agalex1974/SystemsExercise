//
// Created by marios on 18/5/20.
//

#ifndef SYSTEMS2NDEXERCISE_NAMEDPIPEINOUT_H
#define SYSTEMS2NDEXERCISE_NAMEDPIPEINOUT_H

namespace exercise3namespace {
    using uint = unsigned int;
    class NamedPipeInOut {
    private:
        uint bufferSize;
        char* nameOfPipe;
    public:
        explicit NamedPipeInOut(const char* nameOfPipe, uint bufferSize = PIPE_BUF);
        ~NamedPipeInOut();
        void setBufferSize(uint bufferSize){
            this->bufferSize = bufferSize;
        }
        void sendBuffer(const char* buffer);
        char* receiveBuffer();
    };
}

#endif //SYSTEMS2NDEXERCISE_NAMEDPIPEINOUT_H
