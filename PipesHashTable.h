//
// Created by marios on 22/3/20.
//

#ifndef AUXILIARYHASHTABLE_H
#define AUXILIARYHASHTABLE_H

namespace exercise3namespace {

    using uint = unsigned int;
    class PipesHashTable {
        class HashTableElement
        {
        public:
            char* key;
            bool valid;
            int processId;
            int namedPipeIndex;
            HashTableElement():
                key(nullptr),
                valid(false),
                namedPipeIndex(-1),
                processId(-1)
            {}
            ~HashTableElement()
            {
                delete[] key;
            }
        };
        GenericLinkedList<HashTableElement*>* hashedElements;
        uint findPrimeNumberLessEqual(uint n);
        uint getPosition(const char *key);
        uint entriesCount;
    public:
        struct ReturnPipeElement{
            int processId;
            int namedPipeIndex;
        };
        explicit PipesHashTable(uint elementCount);
        bool GetState(char *key);
        ~PipesHashTable();
        bool NegateState(const char* key);
        bool insertPipeProcessId(const char* key, int processId);
        bool insertPipeNamedPipeIndex(const char* key, int namedPipeIndex);
        ReturnPipeElement* GetElement(const char* key);
    };
}
#endif
