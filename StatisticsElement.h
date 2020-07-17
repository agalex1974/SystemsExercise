//
// Created by marios on 29/5/20.
//

#ifndef SYSTEMS2NDEXERCISE_STATISTICSELEMENT_H
#define SYSTEMS2NDEXERCISE_STATISTICSELEMENT_H

#include <cstring>

namespace exercise3namespace {
    class StatisticsElement {
    public:
        int ageRanges[4];
        char* diseaseName;
        StatisticsElement(char* disease, int ranges[4]){
            diseaseName = new char[strlen(disease) + 1];
            strcpy(diseaseName, disease);
            for (int i = 0; i < 4; i++){
                ageRanges[i] = ranges[i];
            }
        }
        ~StatisticsElement(){
            delete[] diseaseName;
        }
    };
}

#endif //SYSTEMS2NDEXERCISE_STATISTICSELEMENT_H
