//
// Created by marios on 24/5/20.
//

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <cstring>
#include <climits>
#include <dirent.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>

#include "MyString.h"
#include "PatientEntry.h"
#include "GenericlLinkedList.h"
#include "GenericHashTable.h"
#include "NamedPipeInOut.h"
#include "RedBlackTrees.h"
#include "SendReceiveProtocol.h"
#include "Worker.h"

using namespace exercise3namespace;

pthread_mutex_t Worker::cond_worker_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t Worker::cvar_worker_non_empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t Worker::cvar_worker_non_full = PTHREAD_COND_INITIALIZER;

static bool lessOrEqualDate(const uint date1[3], const uint date2[3])
{
    if (date2[2] > date1[2]) return true;
    if (date2[2] < date1[2]) return false;
    if (date2[1] > date1[1]) return true;
    if (date2[1] < date1[1]) return false;
    return date2[0] >= date1[0];
}

bool Worker::readDate(const char* stringDate, uint* date)
{
    char day[3];
    char month[3];
    char year[5];
    enum date_format {DAY, MONTH, YEAR};
    date_format currentFormat = DAY;
    bool consistentDate = false;
    uint counter = 0;
    for (uint i = 0; i < strlen(stringDate); i++)
    {
        switch (currentFormat)
        {
            case DAY:
                if (stringDate[i] != '-')
                {
                    day[counter++] = stringDate[i];
                }
                else{
                    day[counter] = '\0';
                    uint dayToCheck = atoi(day);
                    if (dayToCheck < 1 || dayToCheck > 31) return false;
                    date[0] = dayToCheck;
                    currentFormat = MONTH;
                    counter = 0;
                }
                break;
            case MONTH:
                if (stringDate[i] != '-')
                {
                    month[counter++] = stringDate[i];
                }
                else{
                    month[counter] = '\0';
                    uint monthToCheck = atoi(month);
                    if (monthToCheck < 1 || monthToCheck > 12) return false;
                    date[1] = monthToCheck;
                    currentFormat = YEAR;
                    counter = 0;
                }
                break;
            case YEAR:
                year[counter++] = stringDate[i];
                if (i == strlen(stringDate) - 1){
                    year[counter] = '\0';
                    uint yearToCheck = atoi(year);
                    date[2] = yearToCheck;
                    consistentDate = true;
                }
        }
    }
    return consistentDate;
}

Worker::Worker(const char *initialization, NamedPipeInOut* namedPipeInOut)
    :
    patientHashTable(1111),
    patientsEntryHashTree(1111),
    patientsExitHashTree(1111),
    succeededCommands(0),
    failedCommands(0),
    totalCommands(0),
    sendReceiveProtocol(1024)
{
    //////////initialize the cyclic buffer/////////////////////////
    cyclicBuffer.elements = new int[20];
    cyclicBuffer.start = 0;
    cyclicBuffer.end = -1;
    cyclicBuffer.count = 0;
    cyclicBuffer.max_size = 20;
    ///////////////////////////////////////////////////////////////

    ///////////////Create the thread pool//////////////////
    threadPool = new pthread_t[10];
    for (uint counter = 0; counter < 10; counter++) {
        int err = pthread_create(threadPool + counter, NULL, threadConsumer, (void *) this);
        if (err) {
            perror2("pthread_create", err);
            exit(1);
        }
    }

    //printf("%s\n", initialization);
    //printf("Construting Worker!\n");
    this->namedPipeInOut = namedPipeInOut;

    // create the socket for the server side of the worker.
    socket_worker = socket(AF_INET, SOCK_STREAM, 0);
    //Create the internet address
    server_worker.sin_family = AF_INET;
    server_worker.sin_addr.s_addr = inet_addr(inet_address);
    server_worker.sin_port = 0;
    if (bind(socket_worker, (sockaddr*)(&server_worker), sizeof(server_worker)) < 0){
        perror("bind query");
        exit(1);
    }
    socklen_t length = sizeof(server_worker);
    getsockname(socket_worker, (sockaddr*)(&server_worker), &length);
    int serverWorkerPort = ntohs(server_worker.sin_port);

    printf("I have acquired port:%d\n", serverWorkerPort);

    if (listen(socket_worker, 55))
    {
        perror("listen query");
        exit(1);
    }

    const char* position_ptr = strchr(initialization, '\n');
    int position = position_ptr == NULL ? -1 : position_ptr - initialization;
    char numberOfBytesStr[40];
    for (int i = 0; i < position; i++)
    {
        numberOfBytesStr[i] = initialization[i];
    }
    numberOfBytesStr[position] = '\0';
    int byteCount = atoi(numberOfBytesStr);
    namedPipeInOut->setBufferSize(byteCount);
    position++;

    position_ptr = strchr(initialization + position, '\n');
    int positionNew = position_ptr - initialization;
    char updateStr[10];
    for (int i = position; i < positionNew; i++)
    {
        updateStr[i - position] = initialization[i];
    }
    updateStr[positionNew - position] = '\0';
    position = positionNew + 1;

    position_ptr = strchr(initialization + position, '\n');
    positionNew = position_ptr - initialization;
    char ipAddress[100];
    for (int i = position; i < positionNew; i++)
    {
        ipAddress[i - position] = initialization[i];
    }
    ipAddress[positionNew - position] = '\0';
    position = positionNew + 1;
    serverIPAddress = MyString(ipAddress);

    position_ptr = strchr(initialization + position, '\n');
    positionNew = position_ptr - initialization;
    char port[100];
    for (int i = position; i < positionNew; i++)
    {
        port[i - position] = initialization[i];
    }
    port[positionNew - position] = '\0';
    position = positionNew + 1;
    serverPort = atoi(port);

    position_ptr = strchr(initialization + position, '\n');
    // This is a bit risky for big loads
    MyString bufferToSendToAggregator(100);
    bufferToSendToAggregator.appendToString(inet_address);
    bufferToSendToAggregator.appendToString("\n");
    sprintf(port, "%d", serverWorkerPort);
    bufferToSendToAggregator.appendToString(port);
    bufferToSendToAggregator.appendToString("\n");
    do{
        int positionNew = position_ptr - initialization;
        char country[256];
        for (int i = position; i < positionNew; i++)
        {
            country[i - position] = initialization[i];
        }
        country[positionNew - position] = '\0';
        countryNames.Append(MyString(country));

        RedBlackTree<PatientEntry>* tree = new RedBlackTree<PatientEntry>(false);
        patientsEntryHashTree.InsertElement(country, tree);
        tree = new RedBlackTree<PatientEntry>(false);
        patientsExitHashTree.InsertElement(country, tree);

        position = positionNew + 1;
        char path[1024];
        position_ptr = strchr(initialization + position, '\n');
        positionNew = position_ptr - initialization;
        for (int i = position; i < positionNew; i++)
        {
            path[i - position] = initialization[i];
        }
        path[positionNew - position] = '\0';
        position = positionNew + 1;
        ReadFiles(country, path, &bufferToSendToAggregator);
        position_ptr = strchr(initialization + position, '\n');
    }while (position_ptr != nullptr);
    //printf("size of buffer %d\n", strlen(bufferToSendToAggregator.data));
    //printf("%s\n", bufferToSendToAggregator.data);
    //printf("%s\n", bufferToSendToAggregator.data);
    SendStatisticsBuffer(bufferToSendToAggregator.data);
}

void Worker::SendStatisticsBuffer(const char* statisticsBuffer){

    // The server to send the buffer
    sockaddr_in serverToSend;
    // Initiate the socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0){
        perror("socket");
        exit(-1);
    }
    printf("Attemting to connect to:%s on port %d\n", serverIPAddress.data, serverPort);
    serverToSend.sin_family = AF_INET;
    serverToSend.sin_port = htons(serverPort);
    serverToSend.sin_addr.s_addr = inet_addr(serverIPAddress.data);
    int res = connect(sock, (sockaddr *)&serverToSend, sizeof(serverToSend));
    if (res < 0){
        perror("connect");
        exit(-1);
    }
    sendReceiveProtocol.send(sock, statisticsBuffer);
    close(sock);
}

char* Worker::processCommand(char* buffer){
    char* position_ptr = nullptr;
    int position = 0;
    char command[256];
    while((position_ptr = strchr(buffer + position, '\n')) != nullptr) {
        int positionNew = position_ptr - buffer;
        for (int j = position; j < positionNew; j++) {
            command[j - position] = buffer[j];
        }
        command[positionNew - position] = '\0';
        position = positionNew + 1;
    }
    return readCommand(command);
}

char* Worker::readCommand(const char *commandString)
{
    char* commandFailed = new char[100];
    strcpy(commandFailed, "Command Failed!");

    char currentCommand[256];
    if (commandString[0] != '/'){
        return commandFailed;
    }

    int i = 0;
    for(; commandString[i] != ' ' && i < strlen(commandString); i++)
    {
        currentCommand[i] = commandString[i];
    }
    currentCommand[i] = '\0';
    while(commandString[i++] == ' ');
    i--;
    char restCommand[256];
    strcpy(restCommand, commandString + i);
    delete[] commandFailed;
    return executeCommand(currentCommand, restCommand);
}

char* Worker::executeCommand(const char* currentCommand, const char* parameters)
{
    char* commandFailed = new char[100];
    strcpy(commandFailed, "Command Failed!");
    if (!strcmp(currentCommand, commands[0]))
    {
        delete[] commandFailed;
        return SearchPatientRecordCommand(parameters);
    }
    else if (!strcmp(currentCommand, commands[1]))
    {
        delete[] commandFailed;
        return NumPatientAdmissionsCommand(parameters);
    }
    else if (!strcmp(currentCommand, commands[2]))
    {
        delete[] commandFailed;
        return NumPatientDischargesCommand(parameters);
    }
    return commandFailed;
}


void Worker::ReadFiles(const char *country, const char *path, MyString* buffer) {
    struct dirent *dent;
    DIR *srcdir = opendir(path);

    while ((dent = readdir(srcdir)) != NULL) {
        struct stat st;

        if (strcmp(dent->d_name, ".") == 0 || strcmp(dent->d_name, "..") == 0)
            continue;

        if (fstatat(dirfd(srcdir), dent->d_name, &st, 0) < 0) {
            continue;
        }

        if (S_ISREG(st.st_mode)) {
            GenericHashTable<AgeRanges> ageRangesHash(111);
            GenericLinkedList<MyString> diseaseIdList;
            char fullPath[1024];
            strcpy(fullPath, path);
            strcat(fullPath, "/");
            strcat(fullPath, dent->d_name);
            //printf("Reading File:%s.", fullPath);
            buffer->appendToString(dent->d_name);
            buffer->appendToString("\n");
            buffer->appendToString(country);
            buffer->appendToString("\n");
            bool status = ReadFile(country, dent->d_name, fullPath, &ageRangesHash, &diseaseIdList);
            fillBuffer(&ageRangesHash, &diseaseIdList, buffer);
        }
    }

    closedir(srcdir);
}

void Worker::fillBuffer(GenericHashTable<AgeRanges>* pAgesHash, GenericLinkedList<MyString>* pDiseaseList, MyString* buffer)
{
    for (GenericLinkedListIterator<MyString> diseaseIterator(*pDiseaseList); !diseaseIterator.End(); diseaseIterator++){
        MyString* pDiseaseId = diseaseIterator.GetElementOnIterator();
        buffer->appendToString(pDiseaseId->data);
        buffer->appendToString("\n");
        AgeRanges* ageRanges = pAgesHash->GetElement(pDiseaseId->data);
        for (int i = 0; i < 4; i++)
        {
            char countStr[10];
            sprintf(countStr, "%d", ageRanges->range[i]);
            buffer->appendToString(countStr);
            buffer->appendToString("\n");
        }
    }
    buffer->appendToString("END_DISEASE_LOG\n");
}

bool Worker::ReadFile(const char *country, const char* dateStr, const char *path,
                      GenericHashTable<AgeRanges>* pAgeRangesHash, GenericLinkedList<MyString>* pDiseaseIdList)
{
    FILE* file = fopen(path, "r");
    char recordIdString[stringLength];
    char firstNameString[stringLength];
    char lastNameString[stringLength];
    char diseasIDString[stringLength];
    char enterOrExit[stringLength];
    char ageString[stringLength];
    while (fscanf(file, "%s %s %s %s %s %s", recordIdString, enterOrExit, firstNameString, lastNameString,
                  diseasIDString, ageString) == 6)
    {
        //printf("%s %s %s %s %s %s\n", recordIdString, enterOrExit, firstNameString, lastNameString,
        //       diseasIDString, ageString);

        PatientEntry* pPatientEntry = patientHashTable.GetElement(recordIdString);
        RedBlackTree<PatientEntry>* treeEntry = patientsEntryHashTree.GetElement(country);
        RedBlackTree<PatientEntry>* treeExit = patientsExitHashTree.GetElement(country);
        uint date[3];
        if (!readDate(dateStr, date)) {
            printf("Upsa 1\n");
            exit(0);
            return false;
        }
        bool isExit = (strcmp(enterOrExit, "EXIT") == 0);
        if (isExit && pPatientEntry && pPatientEntry->entryDate[0] != 0){
            if (!lessOrEqualDate(pPatientEntry->entryDate, date)) {
                printf("Upsa 2\n");
                exit(0);
                return false;
            }
        }
        if(isExit && pPatientEntry && pPatientEntry->exitDate[0] != 0) {
            printf("Upsa 3\n");
            exit(0);
            return false;
        }
        if(isExit && pPatientEntry){
            for (uint i = 0; i < 3; i++) {
                pPatientEntry->exitDate[i] = date[i];
            }
            treeExit->InsertElement(date, pPatientEntry);
            continue;
        }

        if (!isExit && pPatientEntry && pPatientEntry->exitDate[0] != 0){
            if (!lessOrEqualDate(date, pPatientEntry->exitDate)) {
                printf("Upsa 4\n");
                exit(0);
                return false;
            }
        }
        if(!isExit && pPatientEntry && pPatientEntry->entryDate[0] != 0) {
            printf("Upsa 5\n");
            exit(0);
            return false;
        }
        if(!isExit && pPatientEntry){
            for (uint i = 0; i < 3; i++) {
                pPatientEntry->entryDate[i] = date[i];
            }
            AgeRanges* pAgeRange = pAgeRangesHash->GetElement(diseasIDString);
            if (pAgeRange != nullptr)
                pAgeRange->addAge(atoi(ageString));
            else {
                pAgeRange = new AgeRanges();
                pAgeRange->addAge(atoi(ageString));
                pAgeRangesHash->InsertElement(diseasIDString, pAgeRange);
                pDiseaseIdList->Append(MyString(diseasIDString));
            }
            treeEntry->InsertElement(date, pPatientEntry);
            continue;
        }

        PatientEntry* patientEntryNew = new PatientEntry;
        patientEntryNew->recordId = atoi(recordIdString);
        patientEntryNew->age = atoi(ageString);
        strcpy(patientEntryNew->country, country);
        strcpy(patientEntryNew->firstName, firstNameString);
        strcpy(patientEntryNew->lastName, lastNameString);
        strcpy(patientEntryNew->diseaseId, diseasIDString);
        for (uint i = 0; i < 3; i++)
        {
            if (isExit) {
                patientEntryNew->exitDate[i] = date[i];
            }
            else{
                patientEntryNew->entryDate[i] = date[i];
            }
        }

        if (!patients.Append(patientEntryNew)){
            printf("Upsa 6\n");
            printf("%d\n", patientEntryNew->recordId);
            exit(0);
            return false;
        }

        if (isExit)
            treeExit->InsertElement(date, patientEntryNew);
        else
            treeEntry->InsertElement(date, patientEntryNew);

        PatientEntry** listPatientEntry = GenericLinkedListIterator<PatientEntry*>::ReturnLast(patients);
        patientHashTable.InsertElement(recordIdString, *listPatientEntry, false);
        if (!isExit) {
            AgeRanges* pAgeRange = pAgeRangesHash->GetElement(diseasIDString);
            if (pAgeRange != nullptr)
                pAgeRange->addAge(patientEntryNew->age);
            else {
                pAgeRange = new AgeRanges();
                pAgeRange->addAge(patientEntryNew->age);
                pAgeRangesHash->InsertElement(diseasIDString, pAgeRange);
                pDiseaseIdList->Append(MyString(diseasIDString));
            }
        }
    }

    fclose(file);
    return true;
}

char* Worker::SearchPatientRecordCommand(const char *parameters)
{
    char* commandFailed = new char[100];
    strcpy(commandFailed, "Command Failed!");

    char parameterMatrix[1][100];
    uint parameterCount = 0;
    int i = 0;
    bool newParameter = false;
    uint count = 0;
    while (parameters[i] != ' ' && parameters[i] != '\0')
    {
        newParameter = true;
        parameterMatrix[parameterCount][i++] = parameters[count++];
    }
    if (newParameter) parameterMatrix[parameterCount++][count] = '\0';
    if (parameterCount == 1)
    {
        delete[] commandFailed;
        return SearchPatientRecord(parameterMatrix[0]);
    }
    else{
        return commandFailed;
    }
}

MyString Worker::DateToString(uint date[3]){
    if (date[0] == 0) return MyString("--");
    else{
        char tempString[20];
        MyString output;
        sprintf(tempString, "%d", date[0]);
        output.appendToString(tempString);
        output.appendToString("-");
        sprintf(tempString, "%d", date[1]);
        output.appendToString(tempString);
        output.appendToString("-");
        sprintf(tempString, "%d", date[2]);
        output.appendToString(tempString);
        return output;
    }
}

char* Worker::SearchPatientRecord(const char *recordIDString)
{
    MyString output;
    PatientEntry* pPatientEntry = patientHashTable.GetElement(recordIDString);
    if (pPatientEntry) {
        output.appendToString(recordIDString);
        output.appendToString(" ");
        output.appendToString(pPatientEntry->firstName);
        output.appendToString(" ");
        output.appendToString(pPatientEntry->lastName);
        output.appendToString(" ");
        output.appendToString(pPatientEntry->diseaseId);
        output.appendToString(" ");
        char numberString[12];
        sprintf(numberString, "%d", pPatientEntry->age);
        output.appendToString(numberString);
        output.appendToString(" ");
        output.appendToString(DateToString(pPatientEntry->entryDate).data);
        output.appendToString(" ");
        output.appendToString(DateToString(pPatientEntry->exitDate).data);
        output.appendToString("\n");
    }
    char* outputStr = new char[strlen(output.data) + 1];
    strcpy(outputStr, output.data);
    return outputStr;
}

char* Worker::NumPatientAdmissionsCommand(const char *parameters)
{
    char* commandFailed = new char[100];
    strcpy(commandFailed, "Command Failed!");
    char parameterMatrix[4][100];
    uint parameterCount = 0;
    int i = 0;
    bool newParameter = false;
    uint count = 0;
    while (parameters[i] != ' ' && parameters[i] != '\0')
    {
        newParameter = true;
        parameterMatrix[parameterCount][i++] = parameters[count++];
    }

    while (parameters[i++] == ' ');
    i--;
    if (newParameter) {
        parameterMatrix[parameterCount++][count] = '\0';
        newParameter = false;
        count = 0;
        while (parameters[i] != ' ' && parameters[i] != '\0') {
            newParameter = true;
            parameterMatrix[parameterCount][count++] = parameters[i++];
        }
        while (parameters[i++] == ' ');
        i--;
    }
    if (newParameter) {
        parameterMatrix[parameterCount++][count] = '\0';
        newParameter = false;
        count = 0;
        while (parameters[i] != ' ' && parameters[i] != '\0') {
            newParameter = true;
            parameterMatrix[parameterCount][count++] = parameters[i++];
        }
        while (parameters[i++] == ' ');
        i--;
    }
    if (newParameter) {
        parameterMatrix[parameterCount++][count] = '\0';
        newParameter = false;
        count = 0;
        while (parameters[i] != ' ' && parameters[i] != '\0') {
            newParameter = true;
            parameterMatrix[parameterCount][count++] = parameters[i++];
        }
        while (parameters[i++] == ' ');
        i--;
    }
    if (newParameter) parameterMatrix[parameterCount++][count] = '\0';
    if (parameterCount == 4)
    {
        delete[] commandFailed;
        return NumPatientAdmissions(parameterMatrix[0], parameterMatrix[1], parameterMatrix[2], parameterMatrix[3]);
    }
    if (parameterCount == 3)
    {
        delete[] commandFailed;
        return NumPatientAdmissions(parameterMatrix[0], parameterMatrix[1], parameterMatrix[2], "ALL");
    }
    else{
        return commandFailed;
    }
}

char* Worker::NumPatientAdmissions(const char* disease, const char* date1String, const char* date2String, const char* country)
{
    char* commandFailed = new char[100];
    strcpy(commandFailed, "Command Failed!");

    uint date1[3] = {1, 1 ,0};
    uint date2[3] = {1, 1, 3000};
    if (strcmp(date1String, "-")) {
        if (!readDate(date1String, date1))
            return commandFailed;
    }

    if (strcmp(date2String, "-"))
    {
        if (!readDate(date2String, date2))
            return commandFailed;
    }

    uint frequency = 0;
    GenericHashTable<int> countryHits(111);
    for (GenericLinkedListIterator<MyString> countryIterator(countryNames); !countryIterator.End(); countryIterator++)
    {
        MyString* countryVisited = (countryIterator.GetElementOnIterator());
        int* number = new int;
        *number = 0;
        countryHits.InsertElement(countryVisited->data, number);
        if (!strcmp(countryVisited->data, country) || !strcmp(country, "ALL"))
        {
            RedBlackTree<PatientEntry>* tree = patientsEntryHashTree.GetElement(countryVisited->data);
            GenericLinkedList<PatientEntry*>* statisticsList = tree->RangeGet(date1, date2);
            for (GenericLinkedListIterator<PatientEntry*> patientIterator(*statisticsList); !patientIterator.End(); patientIterator++){
                PatientEntry** ppPatient = patientIterator.GetElementOnIterator();
                if (!strcmp((*ppPatient)->diseaseId, disease))
                {
                    (*(countryHits.GetElement(countryVisited->data)))++;
                }
            }
            delete statisticsList;
        }
    }

    MyString output;
    for (GenericLinkedListIterator<MyString> countryIterator(countryNames); !countryIterator.End(); countryIterator++) {
        MyString *countryVisited = (countryIterator.GetElementOnIterator());
        if (*(countryHits.GetElement(countryVisited->data)) > 0){
            output.appendToString(countryVisited->data);
            output.appendToString(" ");
            char numberString[12];
            sprintf(numberString, "%d", *(countryHits.GetElement(countryVisited->data)));
            output.appendToString(numberString);
            output.appendToString("\n");
        }
    }

    char* outputStr = new char[strlen(output.data) + 1];
    strcpy(outputStr, output.data);
    delete[] commandFailed;
    return outputStr;
}

char* Worker::NumPatientDischargesCommand(const char *parameters)
{
    char* commandFailed = new char[100];
    strcpy(commandFailed, "Command Failed!");

    char parameterMatrix[4][100];
    uint parameterCount = 0;
    int i = 0;
    bool newParameter = false;
    uint count = 0;
    while (parameters[i] != ' ' && parameters[i] != '\0')
    {
        newParameter = true;
        parameterMatrix[parameterCount][i++] = parameters[count++];
    }

    while (parameters[i++] == ' ');
    i--;
    if (newParameter) {
        parameterMatrix[parameterCount++][count] = '\0';
        newParameter = false;
        count = 0;
        while (parameters[i] != ' ' && parameters[i] != '\0') {
            newParameter = true;
            parameterMatrix[parameterCount][count++] = parameters[i++];
        }
        while (parameters[i++] == ' ');
        i--;
    }
    if (newParameter) {
        parameterMatrix[parameterCount++][count] = '\0';
        newParameter = false;
        count = 0;
        while (parameters[i] != ' ' && parameters[i] != '\0') {
            newParameter = true;
            parameterMatrix[parameterCount][count++] = parameters[i++];
        }
        while (parameters[i++] == ' ');
        i--;
    }
    if (newParameter) {
        parameterMatrix[parameterCount++][count] = '\0';
        newParameter = false;
        count = 0;
        while (parameters[i] != ' ' && parameters[i] != '\0') {
            newParameter = true;
            parameterMatrix[parameterCount][count++] = parameters[i++];
        }
        while (parameters[i++] == ' ');
        i--;
    }
    if (newParameter) parameterMatrix[parameterCount++][count] = '\0';
    if (parameterCount == 4)
    {
        delete[] commandFailed;
        return NumPatientDischarges(parameterMatrix[0], parameterMatrix[1], parameterMatrix[2], parameterMatrix[3]);
    }
    if (parameterCount == 3)
    {
        delete[] commandFailed;
        return NumPatientDischarges(parameterMatrix[0], parameterMatrix[1], parameterMatrix[2], "ALL");
    }
    else{
        return commandFailed;
    }
}

char* Worker::NumPatientDischarges(const char* disease, const char* date1String, const char* date2String, const char* country){

    char* commandFailed = new char[100];
    strcpy(commandFailed, "Command Failed!");

    uint date1[3] = {1, 1 ,0};
    uint date2[3] = {1, 1, 3000};
    if (strcmp(date1String, "-")) {
        if (!readDate(date1String, date1))
            return commandFailed;
    }

    if (strcmp(date2String, "-"))
    {
        if (!readDate(date2String, date2))
            return commandFailed;
    }

    uint frequency = 0;
    GenericHashTable<int> countryHits(111);
    for (GenericLinkedListIterator<MyString> countryIterator(countryNames); !countryIterator.End(); countryIterator++)
    {
        MyString* countryVisited = (countryIterator.GetElementOnIterator());
        int* number = new int;
        *number = 0;
        countryHits.InsertElement(countryVisited->data, number);
        if (!strcmp(countryVisited->data, country) || !strcmp(country, "ALL"))
        {
            RedBlackTree<PatientEntry>* tree = patientsExitHashTree.GetElement(countryVisited->data);
            GenericLinkedList<PatientEntry*>* statisticsList = tree->RangeGet(date1, date2);
            for (GenericLinkedListIterator<PatientEntry*> patientIterator(*statisticsList); !patientIterator.End(); patientIterator++){
                PatientEntry** ppPatient = patientIterator.GetElementOnIterator();
                if (!strcmp((*ppPatient)->diseaseId, disease))
                {
                    (*(countryHits.GetElement(countryVisited->data)))++;
                }
            }
            delete statisticsList;
        }
    }

    MyString output;
    for (GenericLinkedListIterator<MyString> countryIterator(countryNames); !countryIterator.End(); countryIterator++) {
        MyString *countryVisited = (countryIterator.GetElementOnIterator());
        if (*(countryHits.GetElement(countryVisited->data)) > 0){
            output.appendToString(countryVisited->data);
            output.appendToString(" ");
            char numberString[12];
            sprintf(numberString, "%d", *(countryHits.GetElement(countryVisited->data)));
            output.appendToString(numberString);
            output.appendToString("\n");
        }
    }

    char* outputStr = new char[strlen(output.data) + 1];
    strcpy(outputStr, output.data);
    delete[] commandFailed;
    return outputStr;
}

void Worker::AcceptCommandsFromServer(){
    //get the sockets fpr query and statistics
    while(1) {
        int newsock = accept(socket_worker, NULL, NULL);
        if (newsock < 0) {
            perror("accept");
            exit(1);
        }
        int err = pthread_mutex_lock(&cond_worker_mtx);
        if (err) {
            perror2("pthread_mutex_lock", err);
            exit(1);
        }
        while (cyclicBuffer.count >= cyclicBuffer.max_size) {
            pthread_cond_wait(&cvar_worker_non_full, &cond_worker_mtx);
        }
        cyclicBuffer.end = (cyclicBuffer.end + 1) % cyclicBuffer.max_size;
        cyclicBuffer.count++;
        cyclicBuffer.elements[cyclicBuffer.end] = newsock;

        // It has to broadcast in the lock
        pthread_cond_broadcast(&cvar_worker_non_empty);
        err = pthread_mutex_unlock(&cond_worker_mtx);
        if (err) {
            perror2("pthread_mutex_unlock", err);
            exit(1);
        }
    }
}

void* Worker::threadConsumer(void *argp)
{
    const uint bufferSize = 1024;
    char buffer[bufferSize];
    Worker* worker = (Worker*)argp;
    while(1){
        pthread_mutex_lock(&cond_worker_mtx);
        while(worker->cyclicBuffer.count <= 0){
            pthread_cond_wait(&cvar_worker_non_empty, &cond_worker_mtx);
        }
        //printf("I have woken server:%d %d!\n", server->, server->cyclicBuffer.start);
        uint socket = worker->cyclicBuffer.elements[worker->cyclicBuffer.start];

        printf("worker socket:%d\n", socket);
        worker->cyclicBuffer.start = (worker->cyclicBuffer.start + 1) % worker->cyclicBuffer.max_size;
        worker->cyclicBuffer.count--;
        pthread_cond_signal(&cvar_worker_non_full);
        pthread_mutex_unlock(&cond_worker_mtx);

        char* command = worker->sendReceiveProtocol.receive(socket);
        printf("trying to execute command:%s\n", command);
        char* result = worker->processCommand(command);
        worker->sendReceiveProtocol.send(socket, result);

        delete[] command;
        delete[] result;
        close(socket);
    }
}