//
// Created by marios on 25/5/20.
//

#include <cstdio>
#include <cstring>
#include "PatientEntry.h"
#include "GenericlLinkedList.h"
#include "NamedPipe.h"

using namespace exercise3namespace;

NamedPipe::NamedPipe(const char* pipeName, int processId):
    Countries()
{
    this->processId = processId;
    this->pipeName = new char[strlen(pipeName) + 1];
    strcpy(this->pipeName, pipeName);
}

void NamedPipe::addCountry(const char *countryName) {
    char* country = new char[strlen(countryName) + 1];
    strcpy(country, countryName);
    Countries.Append(country);
}

void NamedPipe::changeIdentity(const char *pipeName, int processId) {
    delete[] this->pipeName;
    this->processId = processId;
    this->pipeName = new char[strlen(pipeName) + 1];
    strcpy(this->pipeName, pipeName);
}
NamedPipe::~NamedPipe()
{
    delete[] pipeName;
    for (GenericLinkedListIterator<char*> countryIterator(Countries); !countryIterator.End(); countryIterator++){
        delete *(countryIterator.GetElementOnIterator());
    }
}