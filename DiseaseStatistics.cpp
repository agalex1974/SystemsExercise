//
// Created by marios on 29/5/20.
//

#include <cstring>
#include "DiseaseStatistics.h"

using namespace exercise3namespace;

DiseaseStatistics::DiseaseStatistics(const char *diseaseName, const uint *ageRanges)
{
    DiseaseName = new char[strlen(diseaseName) + 1];
    strcpy(DiseaseName, diseaseName);
    for (uint i = 0; i < 4; i++)
        AgeRanges[i] = ageRanges[i];
}

DiseaseStatistics::~DiseaseStatistics()
{
    delete[] DiseaseName;
}