//
// Created by marios on 26/5/20.
//

#ifndef SYSTEMS2NDEXERCISE_GENERICHASHTABLE_H
#define SYSTEMS2NDEXERCISE_GENERICHASHTABLE_H

#include <cstdio>

namespace exercise3namespace {
    using uint = unsigned int;
    template<typename T>
    class GenericHashTable {

        class HashTableElement {
        public:
            char *key;
            T *data;
            bool eraseUponDestroy;
            HashTableElement() :
                    key(nullptr),
                    data(nullptr),
                    eraseUponDestroy(true){}

            ~HashTableElement() {
                delete[] key;
                if (eraseUponDestroy)
                    delete data;
            }
        };

        GenericLinkedList<HashTableElement*> *hashedElements;
        uint findPrimeNumberLessEqual(uint n);
        uint getPosition(const char *key);
        uint entriesCount;
    public:
        explicit GenericHashTable(uint elementCount);
        T* GetElement(const char *key);
        bool InsertElement(char* key, T* data, bool eraseUponDestoy = true);
        ~GenericHashTable();
    };

    template<typename T>
    uint GenericHashTable<T>::findPrimeNumberLessEqual(uint n) {
        uint currentPrime = 2;
        uint nextNumber = 3;
        if (n <= 1) return 0;

        while (nextNumber <= n) {
            uint currentNumber = 2;
            for (; currentNumber < nextNumber; currentNumber++) {
                if (nextNumber % currentNumber == 0)
                    break;
            }
            if (currentNumber == nextNumber) {
                currentPrime = currentNumber;
            }
            nextNumber++;
        }
        return currentPrime;
    }

    template<typename T>
    uint GenericHashTable<T>::getPosition(const char *key) {
        unsigned long long value = 0;
        uint iterationCount = strlen(key) < 6 ? strlen(key) : 6;
        unsigned long long currentRadix = 1;
        for (int i = 0; i < iterationCount; i++) {
            value += currentRadix * key[i];
            currentRadix *= 128;
        }
        return value % entriesCount;
    }

    template<typename T>
    GenericHashTable<T>::GenericHashTable(uint elementCount) {
        entriesCount = findPrimeNumberLessEqual(elementCount);
        hashedElements = new GenericLinkedList<HashTableElement *>[entriesCount];
    }

    template<typename T>
    bool GenericHashTable<T>::InsertElement(char *key, T* data, bool eraseUponDestoy) {
        uint position = getPosition(key);
        if (hashedElements[position].IsListEmpty()) {
            HashTableElement* hashedElement = new HashTableElement();
            hashedElement->key = new char[strlen(key) + 1];
            strcpy(hashedElement->key, key);
            hashedElement->data = data;
            hashedElement->eraseUponDestroy = eraseUponDestoy;
            hashedElements[position].Append(hashedElement);
        } else {
            HashTableElement* element = nullptr;
            HashTableElement** currentBucket = nullptr;
            for (GenericLinkedListIterator<HashTableElement*> bucketIterator(
                    hashedElements[position]); !bucketIterator.End(); bucketIterator++) {
                currentBucket = bucketIterator.GetElementOnIterator();
                char *currentKey = (*currentBucket)->key;
                if (!strcmp(key, currentKey)) {
                    element = *currentBucket;
                    break;
                }
            }
            if (!element) {
                HashTableElement *hashedElement = new HashTableElement();
                hashedElement->key = new char[strlen(key) + 1];
                strcpy(hashedElement->key, key);
                hashedElement->data = data;
                hashedElement->eraseUponDestroy = eraseUponDestoy;
                hashedElements[position].Append(hashedElement);
            } else {
                return false;
            }
        }
        return true;
    }

    template<typename T>
    T* GenericHashTable<T>::GetElement(const char *key) {
        uint position = getPosition(key);
        if (hashedElements[position].IsListEmpty())
            return nullptr;
        for (GenericLinkedListIterator<HashTableElement*> bucketIterator(
                hashedElements[position]); !bucketIterator.End(); bucketIterator++) {
            HashTableElement **element = bucketIterator.GetElementOnIterator();
            if (!strcmp((*element)->key, key))
                return (*element)->data;
        }
        return nullptr;
    }

    template<typename T>
    GenericHashTable<T>::~GenericHashTable() {
        for (uint i = 0; i < entriesCount; i++) {
            if (!hashedElements[i].IsListEmpty()) {
                for (GenericLinkedListIterator<HashTableElement *> bucketIterator(hashedElements[i]); !bucketIterator.End(); bucketIterator++) {
                    delete *(bucketIterator.GetElementOnIterator());
                }
            }
        }
        delete[] hashedElements;
    }
}
#endif //SYSTEMS2NDEXERCISE_GENERICHASHTABLE_H
