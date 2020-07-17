//
// Created by marios on 27/5/20.
//

#ifndef SYSTEMS2NDEXERCISE_MYSTRING_H
#define SYSTEMS2NDEXERCISE_MYSTRING_H

#include <cstring>
#include <cstdlib>
#include <cstdio>

namespace exercise3namespace {

    class MyString {
    public:
        char *data;
        int stringSize;

        MyString()
        {
            stringSize = 1;
            data = new char[stringSize];
            data[0] = '\0';
        }
        explicit MyString(int size){
            data = new char[size];
            for (int i = 0; i < size; i++)
                data[i] = '\0';
            stringSize = size;
        }

        explicit MyString(const char* cstring){
            stringSize = strlen(cstring) + 1;
            data = new char[strlen(cstring) + 1];
            strcpy(data, cstring);
        }

        MyString(const MyString& stringToCopy){
            stringSize = stringToCopy.stringSize;
            data = new char[stringSize];
            strcpy(data, stringToCopy.data);
        }

        MyString& operator=(const MyString& stringToAssign){
            if (this != &stringToAssign) {
                delete[] data;
                stringSize = stringToAssign.stringSize;
                data = new char[stringSize];
                strcpy(data, stringToAssign.data);
            }
            return *this;
        }

        void appendToString(const char* stringToAppend){
            int totalSize = stringSize + strlen(stringToAppend);
            char* totalString = new char[totalSize];
            strcpy(totalString, data);
            strcat(totalString, stringToAppend);
            delete[] data;
            data = new char[totalSize];
            strcpy(data, totalString);
            stringSize = totalSize;
            delete[] totalString;
        }

        ~MyString(){
            delete[] data;
        }
    };

}
#endif //SYSTEMS2NDEXERCISE_MYSTRING_H
