//
// Created by marios on 29/5/20.
//

#ifndef SYSTEMS2NDEXERCISE_DISEASESTATISTICS_H
#define SYSTEMS2NDEXERCISE_DISEASESTATISTICS_H

namespace exercise3namespace
{
    using uint = unsigned int;
    class DiseaseStatistics {
    public:
        DiseaseStatistics(const char* diseaseName, const uint ageRanges[4]);
        ~DiseaseStatistics();
        char* DiseaseName;
        uint AgeRanges[4];
    };
}

#endif //SYSTEMS2NDEXERCISE_DISEASESTATISTICS_H
