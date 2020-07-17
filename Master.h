//
// Created by marios on 25/5/20.
//

#ifndef SYSTEMS2NDEXERCISE_DISEASEAGGREGATOR_H
#define SYSTEMS2NDEXERCISE_DISEASEAGGREGATOR_H

namespace exercise3namespace {
    class Master {
    private:
        char* init_directory;
        int workerCount;
        int bufferSize;
        NamedPipe** pipes;
        void initializeWorkers();
        GenericLinkedList<MyString>* pipeNames;
        PipesHashTable* pipesHashTable;
        GenericLinkedList<MyString> countryNames;
        struct AgeRange{
            int index;
            int ageRange;
        };
        MyString serverIp;
        uint serverPort;
    public:
        Master(const char *init_directory, int workerCount, int bufferSize,
               const char* serverIP, int serverPort,
               GenericLinkedList<MyString>* pipeNames, PipesHashTable* pipesHashTable);
        bool KillWorkers();
        ~Master();
        void UpdateWorker(const char* previousPipeNane, const char* newPipeName);
    };


}


#endif //SYSTEMS2NDEXERCISE_DISEASEAGGREGATOR_H
