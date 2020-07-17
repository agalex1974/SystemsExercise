//
// Created by marios on 12/3/20.
//

#ifndef PATIENTENTRY_H
#define PATIENTENTRY_H

namespace exercise3namespace
{
    class PatientEntry {
        using uint = unsigned int;
        static const uint stringLength = 256;
    public:
        uint recordId;  //The record id of the patient in an unsigned int
        char firstName[stringLength];
        char lastName[stringLength];
        char diseaseId[stringLength];
        char country[stringLength];
        uint entryDate[3];
        uint exitDate[3];
        uint age;
        PatientEntry(){
            entryDate[0] = 0; entryDate[1] = 0; entryDate[2] = 0;
            exitDate[0]  = 0;  exitDate[1] = 0; exitDate[2]  = 0;
        };
        PatientEntry(const PatientEntry& patientEntry);
    };

}
#endif
