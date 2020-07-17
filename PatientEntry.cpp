//
// Created by marios on 12/3/20.
//

#include <cstring>
#include "PatientEntry.h"

using namespace exercise3namespace;

PatientEntry::PatientEntry(const PatientEntry &patientEntry) {
    recordId = patientEntry.recordId;
    strcpy(country, patientEntry.country);
    strcpy(firstName, patientEntry.firstName);
    strcpy(lastName, patientEntry.lastName);
    strcpy(diseaseId, patientEntry.diseaseId);
    for (uint i = 0; i < 3; i++) {
        entryDate[i] = patientEntry.entryDate[i];
        exitDate[i] = patientEntry.exitDate[i];
    }
}