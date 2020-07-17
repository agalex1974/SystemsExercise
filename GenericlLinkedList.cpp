//
// Created by marios on 15/3/20.
//

#include "PatientEntry.h"
#include "GenericlLinkedList.h"

using namespace exercise3namespace;

GenericLinkedList<PatientEntry*>::GenericLinkedList() {
    _head = nullptr;
    _tail = nullptr;
}

bool GenericLinkedList<PatientEntry*>::IsListEmpty() const {
    return _head == nullptr;
}

// Function that adds to the end the patient entry
// You need to consider two entries.
//
bool GenericLinkedList<PatientEntry*>::Append(PatientEntry* element, bool noRecordId)
{
    if (IsListEmpty()){
        if (noRecordId) return false;
        GenericNodeElement* pElement = new GenericNodeElement();
        pElement->dataElement = element;
        pElement->next = nullptr;
        _head = pElement;
        _tail = _head;
    }
    else{
        uint max = _head->dataElement->recordId;
        GenericNodeElement* currentNode = _head;
        GenericNodeElement* previousNode = nullptr;
        while (currentNode){
            uint currentRecordId = currentNode->dataElement->recordId;
            if (!noRecordId && element->recordId == currentRecordId){
                return false;
            }
            if (max < currentNode->dataElement->recordId){
                max = currentRecordId;
            }
            previousNode = currentNode;
            currentNode = currentNode->next;
        }
        GenericNodeElement* pElement = new GenericNodeElement();
        pElement->dataElement = element;
        if (noRecordId) pElement->dataElement->recordId = max + 1;
        pElement->next = nullptr;
        previousNode->next = pElement;
        _tail = pElement;
    }
    return true;
}

GenericLinkedList<PatientEntry*>::~GenericLinkedList()
{
    GenericNodeElement* currentNode = _head;
    while (currentNode)
    {
        GenericNodeElement* toDeleteNode = currentNode;
        currentNode = currentNode->next;
        delete toDeleteNode;
    }
}

bool GenericLinkedList<PatientEntry*>::ErasePatientEntries()
{
    GenericNodeElement* currentNode = _head;
    while (currentNode)
    {
        //GenericNodeElement* toDeleteNode = currentNode;
        delete currentNode->dataElement;
        currentNode = currentNode->next;
        //delete toDeleteNode;
    }
    return true;
}
