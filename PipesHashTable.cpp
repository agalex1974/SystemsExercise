//
// Created by marios on 22/3/20.
//

#include <cstring>
#include <cstdio>
#include "PatientEntry.h"
#include "GenericlLinkedList.h"
#include "NamedPipe.h"
#include "PipesHashTable.h"

using namespace exercise3namespace;

uint PipesHashTable::findPrimeNumberLessEqual(uint n) {
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

uint PipesHashTable::getPosition(const char *key)
{
    unsigned long long value = 0;
    uint iterationCount = strlen(key) < 6 ? strlen(key) : 6;
    unsigned long long currentRadix = 1;
    for (int i = 0; i < iterationCount; i++)
    {
        value += currentRadix * key[i];
        currentRadix *= 128;
    }
    return value % entriesCount;
}

PipesHashTable::PipesHashTable(uint elementCount){
    this->entriesCount = findPrimeNumberLessEqual(elementCount);
    hashedElements = new GenericLinkedList<HashTableElement*>[entriesCount];
}

bool PipesHashTable::insertPipeNamedPipeIndex(const char *key, int namedPipeIndex) {
    uint position = getPosition(key);
    if (hashedElements[position].IsListEmpty()) {
        return false;
    } else {
        HashTableElement *element = nullptr;
        HashTableElement **currentBucket = nullptr;
        for (GenericLinkedListIterator<HashTableElement *> bucketIterator(
                hashedElements[position]); !bucketIterator.End(); bucketIterator++) {
            currentBucket = bucketIterator.GetElementOnIterator();
            char *currentKey = (*currentBucket)->key;
            if (!strcmp(key, currentKey)) {
                element = *currentBucket;
                break;
            }
        }
        if (!element) {
            return false;
        } else {
            element->namedPipeIndex = namedPipeIndex;
        }
    }
    return true;
}
bool PipesHashTable::insertPipeProcessId(const char *key, int processId){
    uint position = getPosition(key);
    if (hashedElements[position].IsListEmpty()) {
        HashTableElement *hashedElement = new HashTableElement();
        hashedElement->key = new char[strlen(key) + 1];
        strcpy(hashedElement->key, key);
        hashedElement->valid = !hashedElement->valid;
        hashedElement->processId = processId;
        hashedElements[position].Append(hashedElement);
        return true;
    } else {
        HashTableElement *element = nullptr;
        HashTableElement **currentBucket = nullptr;
        for (GenericLinkedListIterator<HashTableElement *> bucketIterator(
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
            hashedElement->valid = !hashedElement->valid;
            hashedElement->processId = processId;
            hashedElements[position].Append(hashedElement);
        } else {
            return false;
        }
    }
    return true;
}

bool PipesHashTable::NegateState(const char *key)
{
    uint position = getPosition(key);
    if (hashedElements[position].IsListEmpty())
    {
        return false;
    }
    else{
        HashTableElement* element = nullptr;
        HashTableElement** currentBucket = nullptr;
        for (GenericLinkedListIterator<HashTableElement*> bucketIterator(hashedElements[position]); !bucketIterator.End(); bucketIterator++){
            currentBucket = bucketIterator.GetElementOnIterator();
            char* currentKey = (*currentBucket)->key;
            if (!strcmp(key, currentKey))
            {
                element = *currentBucket;
                break;
            }
        }
        if (!element) {
            return false;
        }
        else{
            element->valid = !element->valid;
        }
    }
    return true;
}

PipesHashTable::ReturnPipeElement* PipesHashTable::GetElement(const char *key)
{
    uint position = getPosition(key);
    if (hashedElements[position].IsListEmpty())
    {
        return nullptr;
    }
    else{
        HashTableElement* element = nullptr;
        HashTableElement** currentBucket = nullptr;
        for (GenericLinkedListIterator<HashTableElement*> bucketIterator(hashedElements[position]); !bucketIterator.End(); bucketIterator++){
            currentBucket = bucketIterator.GetElementOnIterator();
            char* currentKey = (*currentBucket)->key;
            if (!strcmp(key, currentKey))
            {
                element = *currentBucket;
                break;
            }
        }
        if (!element) {
            return nullptr;
        }
        else{
            ReturnPipeElement* returnElement = new ReturnPipeElement;
            returnElement->namedPipeIndex = element->namedPipeIndex;
            returnElement->processId = element->processId;
            return returnElement;
        }
    }
}

bool PipesHashTable::GetState(char *key) {
    uint position = getPosition(key);
    if (hashedElements[position].IsListEmpty())
        return false;
    for (GenericLinkedListIterator<HashTableElement*> bucketIterator(hashedElements[position]); !bucketIterator.End(); bucketIterator++){
        HashTableElement** element = bucketIterator.GetElementOnIterator();
        if (!strcmp((*element)->key, key))
            return (*element)->valid;
    }
    return false;
}

PipesHashTable::~PipesHashTable()
{
    for (uint i = 0; i < entriesCount; i++){
        if (!hashedElements[i].IsListEmpty())
        {
            for (GenericLinkedListIterator<HashTableElement*> bucketIterator(hashedElements[i]); !bucketIterator.End(); bucketIterator++){
                delete *(bucketIterator.GetElementOnIterator());
            }
        }
    }
    delete[] hashedElements;
}