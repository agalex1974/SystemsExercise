//
// Created by marios on 25/5/20.
//

#ifndef SYSTEMS2NDEXERCISE_NAMEDPIPE_H
#define SYSTEMS2NDEXERCISE_NAMEDPIPE_H

namespace exercise3namespace {
    class NamedPipe {
    public:
        char* pipeName;
        int processId;
        GenericLinkedList<char*> Countries;
        explicit NamedPipe(const char* pipeName, int processId);
        void changeIdentity(const char* pipeName, int processId);
        void addCountry(const char* countryName);
        ~NamedPipe();
    };
}

#endif //SYSTEMS2NDEXERCISE_NAMEDPIPE_H
