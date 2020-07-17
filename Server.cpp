//
// Created by marios on 20/6/20.
//

#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>

#include "MyString.h"
#include "StatisticsElement.h"
#include "PatientEntry.h"
#include "GenericlLinkedList.h"
#include "GenericHashTable.h"
#include "RedBlackTrees.h"
#include "SendReceiveProtocol.h"
#include "Server.h"
#include "cctype"

using namespace exercise3namespace;

pthread_mutex_t Server::cond_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t Server::cvar_non_empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t Server::cvar_non_full = PTHREAD_COND_INITIALIZER;
pthread_mutex_t Server::listener_cond_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t Server::listener_cvar = PTHREAD_COND_INITIALIZER;
pthread_mutex_t Server::data_mtx = PTHREAD_MUTEX_INITIALIZER;

int Server::is_listener_signaled = 0;

char *trimwhitespace(char *str)
{
    char *end;

    // Trim leading space
    while(isspace((unsigned char)*str)) str++;

    if(*str == 0)  // All spaces?
        return str;

    // Trim trailing space
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;

    // Write new null terminator character
    end[1] = '\0';

    return str;
}

int Server::compare(const void * a, const void * b)
{
    AgeRange* a1 = (AgeRange*)a;
    AgeRange* b1 = (AgeRange*)b;
    return ( b1->ageRange - a1->ageRange );
}

bool Server::readDate(const char* stringDate, uint* date)
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

Server::~Server()
{
    delete[] cyclicBuffer.elements;
    delete[] threadPool;
}

Server::Server(uint queryPortNumber, uint statisticsPortNumber, uint threadCount, uint bufferSize):
    server_query(),
    server_statistics(),
    queryPortNumber(queryPortNumber),
    statisticsPortNumber(statisticsPortNumber),
    buffer_size(bufferSize),
    statistics(1111),
    workerServerHashTable(1111),
    sendReceiveProtocol(1024)
{
    //////////initialize the cyclic buffer/////////////////////////
    cyclicBuffer.elements = new cyclic_buffer_element[buffer_size];
    cyclicBuffer.start = 0;
    cyclicBuffer.end = -1;
    cyclicBuffer.count = 0;
    cyclicBuffer.max_size = buffer_size;
    ///////////////////////////////////////////////////////////////

    ///////////////Create the thread pool//////////////////
    threadPool = new pthread_t[threadCount];
    for (uint counter = 0; counter < threadCount; counter++) {
        int err = pthread_create(threadPool + counter, NULL, threadConsumer, (void *) this);
        if (err) {
            perror2("pthread_create", err);
            exit(1);
        }
    }

    //get the sockets fpr query and statistics
    sock_query = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_query < 0){
        perror("socket error!");
        exit(-1);
    }
    sock_statistics = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_statistics < 0){
        perror("socket error!");
        exit(-1);
    }

    server_query.sin_family = AF_INET;
    server_query.sin_addr.s_addr = inet_addr(inet_address);
    server_query.sin_port = htons(queryPortNumber);

    server_statistics.sin_family = AF_INET;
    server_statistics.sin_addr.s_addr = inet_addr(inet_address);
    server_statistics.sin_port = htons(statisticsPortNumber);

    if (bind(sock_query, (sockaddr*)(&server_query), sizeof(server_query)) < 0){
        perror("bind query");
        exit(1);
    }

    if (bind(sock_statistics, (sockaddr*)(&server_statistics), sizeof(server_statistics)) < 0){
        perror("bind statistics");
        exit(1);
    }

    if (listen(sock_query, 55))
    {
        perror("listen query");
        exit(1);
    }

    if (listen(sock_statistics, 55))
    {
        perror("listen statistics");
        exit(1);
    }

    printf("Listening for query connections to port %d\n", queryPortNumber);
    printf("Listening for statistics connections to port %d\n", statisticsPortNumber);
}

void* Server::threadConsumer(void *argp)
{
    const uint bufferSize = 1024;
    char buffer[bufferSize];
    Server* server = (Server*)argp;
    while(1){
        pthread_mutex_lock(&cond_mtx);
        while(server->cyclicBuffer.count <= 0){
            pthread_cond_wait(&cvar_non_empty, &cond_mtx);
        }
        //printf("I have woken server:%d %d!\n", server->, server->cyclicBuffer.start);
        uint socket = server->cyclicBuffer.elements[server->cyclicBuffer.start].socket;
        TypeOfConnection type = server->cyclicBuffer.elements[server->cyclicBuffer.start].type;
        printf("server socket:%d type %d\n", socket, type);
        server->cyclicBuffer.start = (server->cyclicBuffer.start + 1) % server->cyclicBuffer.max_size;
        server->cyclicBuffer.count--;
        pthread_cond_signal(&cvar_non_full);
        pthread_mutex_unlock(&cond_mtx);
        if (type == TypeOfConnection::WORKER){;
            char* buffer = server->sendReceiveProtocol.receive(socket);
            //printf("Read this buffer:%s\n", buffer);
            server->GetStatistics(buffer);
            //usleep(500000);
            printf("socket : %d done Statistics\n", socket);
            delete[] buffer;
        }
        else{
            char* command = server->sendReceiveProtocol.receive(socket);
            char currentCommand[256];
            char* result = server->readCommand(command, currentCommand);
            server->sendReceiveProtocol.send(socket, result);
            delete[] result;
        }
        close(socket);
    }
}

void* Server::ListenToQueryRequests(void *args) {
    data_for_listener* data = (data_for_listener*) args;
    while(1){
        int newsock = accept(data->socket, NULL, NULL);
        if (newsock < 0){
            perror("accept");
            exit(1);
        }

        /*
        /////////////////////////////////////////////////////////
        int err = pthread_mutex_lock(&listener_cond_mtx);
        if (err){
            perror2("pthread_mutex_lock", err);
            exit(1);
        }
        if (!is_listener_signaled)
            pthread_cond_wait(&listener_cvar, &listener_cond_mtx);
        is_listener_signaled = 0;
        ///////////////////////////////////////////////////////////////
        */

        int err = pthread_mutex_lock(&cond_mtx);
        if (err){
            perror2("pthread_mutex_lock", err);
            exit(1);
        }
        while(data->cyclicBuffer->count >= data->cyclicBuffer->max_size){
            pthread_cond_wait(&cvar_non_full, &cond_mtx);
        }
        data->cyclicBuffer->end = (data->cyclicBuffer->end + 1) % data->cyclicBuffer->max_size;
        data->cyclicBuffer->count++;
        data->cyclicBuffer->elements[data->cyclicBuffer->end].socket = newsock;
        data->cyclicBuffer->elements[data->cyclicBuffer->end].type = TypeOfConnection::CLIENT;
        // It has to broadcast in the lock
        pthread_cond_broadcast(&cvar_non_empty);
        err = pthread_mutex_unlock(&cond_mtx);
        if (err){
            perror2("pthread_mutex_unlock", err);
            exit(1);
        }
        /////////////////////////////////////////////////////////////////


    }
    pthread_exit(NULL);
}

void* Server::ListenToStatisticsRequests(void *args)  {
    data_for_listener* data = (data_for_listener*) args;
    while(1){

        int newsock = accept(data->socket, NULL, NULL);
        printf("accepted connection!\n");
        if (newsock < 0){
            perror("accept");
            exit(1);
        }

        int err = pthread_mutex_lock(&cond_mtx);
        if (err){
            perror2("pthread_mutex_lock", err);
            exit(1);
        }

        while(data->cyclicBuffer->count >= data->cyclicBuffer->max_size){
            pthread_cond_wait(&cvar_non_full, &cond_mtx);
        }

        data->cyclicBuffer->end = (data->cyclicBuffer->end + 1) % data->cyclicBuffer->max_size;
        data->cyclicBuffer->count++;
        data->cyclicBuffer->elements[data->cyclicBuffer->end].socket = newsock;
        data->cyclicBuffer->elements[data->cyclicBuffer->end].type = TypeOfConnection::WORKER;
        /*printf("Elements:%d %d %d\n", data->cyclicBuffer->elements[data->cyclicBuffer->end].socket,
                data->cyclicBuffer->elements[data->cyclicBuffer->end].type,
               data->cyclicBuffer->end);*/

        pthread_cond_broadcast(&cvar_non_empty);
        err = pthread_mutex_unlock(&cond_mtx);
        if (err){
            perror2("pthread_mutex_unlock", err);
            exit(1);
        }

    }
    pthread_exit(NULL);
}

void Server::Listen()
{
    pthread_t thread_query_listener, thread_statistics_listener;

    data_for_listener queryListenerData;
    queryListenerData.cyclicBuffer = &cyclicBuffer;
    queryListenerData.pServer = &server_query;
    queryListenerData.socket = sock_query;

    int err = pthread_create(&thread_query_listener, NULL, ListenToQueryRequests, (void*)&queryListenerData);
    if (err){
        perror2("pthread_create", err);
        exit(1);
    }

    data_for_listener statisticsListenerData;
    statisticsListenerData.cyclicBuffer = &cyclicBuffer;
    statisticsListenerData.pServer = &server_statistics;
    statisticsListenerData.socket = sock_statistics;

    err = pthread_create(&thread_statistics_listener, NULL, ListenToStatisticsRequests, (void*)&statisticsListenerData);
    if (err){
        perror2("pthread_create", err);
        exit(1);
    }

    pthread_join(thread_statistics_listener, NULL);
    pthread_join(thread_query_listener, NULL);
}

char* Server::readCommand(const char *commandString, char* currentCommand)
{
    char* commandFailed = new char[100];
    strcpy(commandFailed, "Command Failed!");

    if (commandString[0] != '/')
    {
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
    strcat(restCommand, " ");
    delete[] commandFailed;
    trimwhitespace(restCommand);
    //printf("current command:%s parameters:%s here...\n", currentCommand, restCommand);
    return executeCommand(currentCommand, restCommand);
}

char* Server::executeDiseaseFrequency(const char* parameters)
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
    //printf("Parameter count:%d\n", parameterCount);

    if (parameterCount == 4)
    {
        delete[] commandFailed;
        //printf("parameter1:%s parameter2:%s parameter3:%s parameter4:%s\n", parameterMatrix[0], parameterMatrix[1], parameterMatrix[2], parameterMatrix[3]);
        return DiseaseFrequency(parameterMatrix[0], parameterMatrix[1], parameterMatrix[2], parameterMatrix[3]);
    }
    if (parameterCount == 3)
    {
        //printf("HERE!\n");
        delete[] commandFailed;
        return DiseaseFrequency(parameterMatrix[0], parameterMatrix[1], parameterMatrix[2], "ALL");
    }
    else{
        printf("error in %s input!\n", commands[1]);
        return commandFailed;
    }
}

char* Server::DiseaseFrequency(const char* virusName, const char* date1String, const char* date2String, const char* country){
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
    for (GenericLinkedListIterator<MyString> countryIterator(countryNames); !countryIterator.End(); countryIterator++)
    {
        MyString* countryVisited = (countryIterator.GetElementOnIterator());
        if (!strcmp(countryVisited->data, country) || !strcmp(country, "ALL"))
        {
            RedBlackTree<StatisticsElement>* tree = statistics.GetElement(countryVisited->data);
            GenericLinkedList<StatisticsElement*>* statisticsList = tree->RangeGet(date1, date2);
            for (GenericLinkedListIterator<StatisticsElement*> statisticsIterator(*statisticsList); !statisticsIterator.End(); statisticsIterator++){
                StatisticsElement** ppStatisticsElement = statisticsIterator.GetElementOnIterator();
                if (!strcmp((*ppStatisticsElement)->diseaseName, virusName))
                {
                    for (uint i = 0; i < 4; i++){
                        frequency += (*ppStatisticsElement)->ageRanges[i];
                    }
                }
            }
            delete statisticsList;
        }
    }

    char* outputStr = new char[100];
    sprintf(outputStr, "%d", frequency);
    printf("%s %s %s %s %s\n", virusName, date1String, date2String, country, outputStr);
    delete[] commandFailed;
    return outputStr;
}

char* Server::executeTopKAgeRanges(const char *parameters)
{
    char* commandFailed = new char[100];
    strcpy(commandFailed, "Command Failed!");

    char parameterMatrix[5][100];
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
    if (parameterCount == 5)
    {
        delete[] commandFailed;
        return TopKAgeRanges(parameterMatrix[0], parameterMatrix[1], parameterMatrix[2], parameterMatrix[3], parameterMatrix[4]);
    }
    else{
        printf("error in %s input!\n", commands[2]);
        return commandFailed;
    }
}

char* Server::TopKAgeRanges(const char* kstring, const char* country, const char* virusName, const char* date1String, const char* date2String){
    char* commandFailed = new char[100];
    strcpy(commandFailed, "Command Failed!");

    uint k = atoi(kstring);
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

    int frequency = 0;
    int frequencyPerAgeRange[4] = {0, 0, 0, 0};
    for (GenericLinkedListIterator<MyString> countryIterator(countryNames); !countryIterator.End(); countryIterator++)
    {
        MyString* countryVisited = (countryIterator.GetElementOnIterator());
        if (!strcmp(countryVisited->data, country))
        {
            RedBlackTree<StatisticsElement>* tree = statistics.GetElement(countryVisited->data);
            GenericLinkedList<StatisticsElement*>* statisticsList = tree->RangeGet(date1, date2);
            for (GenericLinkedListIterator<StatisticsElement*> statisticsIterator(*statisticsList); !statisticsIterator.End(); statisticsIterator++){
                StatisticsElement** ppStatisticsElement = statisticsIterator.GetElementOnIterator();
                if (!strcmp((*ppStatisticsElement)->diseaseName, virusName))
                {
                    for (uint i = 0; i < 4; i++){
                        frequencyPerAgeRange[i] += (*ppStatisticsElement)->ageRanges[i];
                        frequency += (*ppStatisticsElement)->ageRanges[i];
                    }
                }
            }
            delete statisticsList;
        }
    }
    AgeRange ageRange[4];
    for (uint i = 0; i < 4; i++){
        ageRange[i].index = i;
        ageRange[i].ageRange = frequencyPerAgeRange[i];
    }
    qsort(ageRange, 4, sizeof(AgeRange), compare);
    MyString output;
    for (uint i = 0; i < k; i++)
    {
        int number = frequency == 0 ? 0 : (int)(100.0 * ageRange[i].ageRange/ frequency);
        char frequencyStr[100];
        switch (ageRange[i].index){
            case 0:
                sprintf(frequencyStr, "0-20: %d%%\n", number);
                output.appendToString(frequencyStr);
                break;
            case 1:
                sprintf(frequencyStr, "21-40: %d%%\n", number);
                output.appendToString(frequencyStr);
                break;
            case 2:
                sprintf(frequencyStr,"41-60: %d%%\n", number);
                output.appendToString(frequencyStr);
                break;
            default:
                sprintf(frequencyStr,"60+: %d%%\n", number);
                output.appendToString(frequencyStr);
                break;
        }
    }
    delete[] commandFailed;
    char* outputStr = new char[strlen(output.data) + 1];
    strcpy(outputStr, output.data);
    return outputStr;
}

char* Server::executeSearchPatientRecord(const char *parameters)
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
    printf("Parameter count:%d\n", parameterCount);
    if (parameterCount >= 1)
    {
        delete[] commandFailed;
        printf("Parameter Matrix:%s\n", parameterMatrix[0]);
        return SearchPatientRecord(parameterMatrix[0]);
    }
    else{
        printf("error in %s input!\n", commands[3]);
        return commandFailed;
    }
}

char* Server::SearchPatientRecord(const char* recordId){

    MyString command;
    command.appendToString("/searchPatientRecord ");
    command.appendToString(recordId);
    command.appendToString("\n");
    GenericHashTable<int> processedPorts(111);
    MyString output;
    for (GenericLinkedListIterator<MyString> countryIterator(countryNames); !countryIterator.End(); countryIterator++) {
        char* currentCountry = (*(countryIterator.GetElementOnIterator())).data;
        workerServerInfo* wrkServerInfo = workerServerHashTable.GetElement(currentCountry);
        uint port = wrkServerInfo->port;
        char portStr[10];
        sprintf(portStr, "%d", port);
        int* portGet = processedPorts.GetElement(portStr);
        if (!portGet){
            int* pPort = new int;
            *pPort = port;
            processedPorts.InsertElement(portStr, pPort);

            // The server to send the buffer
            sockaddr_in serverToSend;
            // Initiate the socket
            int sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock < 0){
                perror("socket");
                exit(-1);
            }
            printf("Attemting to connect to:%s on port %d\n", wrkServerInfo->ipAddress.data, port);
            serverToSend.sin_family = AF_INET;
            serverToSend.sin_port = htons(port);
            serverToSend.sin_addr.s_addr = inet_addr(wrkServerInfo->ipAddress.data);
            int res = connect(sock, (sockaddr *)&serverToSend, sizeof(serverToSend));
            if (res < 0){
                perror("connect");
                exit(-1);
            }
            sendReceiveProtocol.send(sock, command.data);
            char* buffer = sendReceiveProtocol.receive(sock);
            output.appendToString(buffer);
            close(sock);
        }
    }

    char* outputStr = new char[strlen(output.data) + 1];
    strcpy(outputStr, output.data);
    return outputStr;
}

char* Server::executeNumPatientAdmissions(const char* parameters){
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
    if (parameterCount == 3)
    {
        strcpy(parameterMatrix[3], "ALL");
    }
    if (parameterCount >= 3)
    {
        delete[] commandFailed;
        return NumPatientAdmissions(parameterMatrix[0], parameterMatrix[1], parameterMatrix[2], parameterMatrix[3]);
    }
    else{
        return commandFailed;
    }
}

char* Server::NumPatientAdmissions(const char *virusName, const char *date1, const char *date2, const char *country)
{
    MyString command;
    command.appendToString("/numPatientAdmissions ");
    command.appendToString(virusName);
    command.appendToString(" ");
    command.appendToString(date1);
    command.appendToString(" ");
    command.appendToString(date2);
    command.appendToString(" ");
    command.appendToString(country);
    command.appendToString("\n");
    GenericHashTable<int> processedPorts(111);
    MyString output;
    for (GenericLinkedListIterator<MyString> countryIterator(countryNames); !countryIterator.End(); countryIterator++) {
        char* currentCountry = (*(countryIterator.GetElementOnIterator())).data;
        workerServerInfo* wrkServerInfo = workerServerHashTable.GetElement(currentCountry);
        uint port = wrkServerInfo->port;
        char portStr[10];
        sprintf(portStr, "%d", port);
        int* portGet = processedPorts.GetElement(portStr);
        if (!portGet){
            int* pPort = new int;
            *pPort = port;
            processedPorts.InsertElement(portStr, pPort);

            // The server to send the buffer
            sockaddr_in serverToSend;
            // Initiate the socket
            int sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock < 0){
                perror("socket");
                exit(-1);
            }
            printf("Attemting to connect to:%s on port %d\n", wrkServerInfo->ipAddress.data, port);
            serverToSend.sin_family = AF_INET;
            serverToSend.sin_port = htons(port);
            serverToSend.sin_addr.s_addr = inet_addr(wrkServerInfo->ipAddress.data);
            int res = connect(sock, (sockaddr *)&serverToSend, sizeof(serverToSend));
            if (res < 0){
                perror("connect");
                exit(-1);
            }
            sendReceiveProtocol.send(sock, command.data);
            char* buffer = sendReceiveProtocol.receive(sock);
            output.appendToString(buffer);
            close(sock);
        }
    }

    char* outputStr = new char[strlen(output.data) + 1];
    strcpy(outputStr, output.data);
    return outputStr;
}

char* Server::executeNumPatientsDischarges(const char *parameters){
    char* commandFailed = new char[100];
    strcpy(commandFailed, "Command Failed!");

    char parameterMatrix[4][100];
    uint parameterCount = 0;
    int i = 0;
    bool newParameter = false;
    uint count = 0;
    while (parameters[i] != ' ' && parameters[i] != '\0') {
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
    if (parameterCount == 3) {
        strcpy(parameterMatrix[3], "ALL");
    }
    if (parameterCount >= 3) {
        delete[] commandFailed;
        return NumPatientDischarges(parameterMatrix[0], parameterMatrix[1], parameterMatrix[2],
                                                       parameterMatrix[3]);
    } else {
        //printf("error in %s input!\n", commands[1]);
        return commandFailed;
    }
}

char* Server::NumPatientDischarges(const char *virusName, const char *date1, const char *date2, const char *country)
{
    MyString command;
    command.appendToString("/numPatientDischarges ");
    command.appendToString(virusName);
    command.appendToString(" ");
    command.appendToString(date1);
    command.appendToString(" ");
    command.appendToString(date2);
    command.appendToString(" ");
    command.appendToString(country);
    command.appendToString("\n");
    GenericHashTable<int> processedPorts(111);
    MyString output;
    for (GenericLinkedListIterator<MyString> countryIterator(countryNames); !countryIterator.End(); countryIterator++) {
        char* currentCountry = (*(countryIterator.GetElementOnIterator())).data;
        workerServerInfo* wrkServerInfo = workerServerHashTable.GetElement(currentCountry);
        uint port = wrkServerInfo->port;
        char portStr[10];
        sprintf(portStr, "%d", port);
        int* portGet = processedPorts.GetElement(portStr);
        if (!portGet){
            int* pPort = new int;
            *pPort = port;
            processedPorts.InsertElement(portStr, pPort);

            // The server to send the buffer
            sockaddr_in serverToSend;
            // Initiate the socket
            int sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock < 0){
                perror("socket");
                exit(-1);
            }
            printf("Attemting to connect to:%s on port %d\n", wrkServerInfo->ipAddress.data, port);
            serverToSend.sin_family = AF_INET;
            serverToSend.sin_port = htons(port);
            serverToSend.sin_addr.s_addr = inet_addr(wrkServerInfo->ipAddress.data);
            int res = connect(sock, (sockaddr *)&serverToSend, sizeof(serverToSend));
            if (res < 0){
                perror("connect");
                exit(-1);
            }
            sendReceiveProtocol.send(sock, command.data);
            char* buffer = sendReceiveProtocol.receive(sock);
            output.appendToString(buffer);
            close(sock);
        }
    }

    char* outputStr = new char[strlen(output.data) + 1];
    strcpy(outputStr, output.data);
    return outputStr;
}

char* Server::executeCommand(const char* currentCommand, const char* parameters)
{
    if (!strcmp(currentCommand, commands[0]))
    {
        return executeDiseaseFrequency(parameters);
    }
    else if (!strcmp(currentCommand, commands[1]))
    {
        return executeTopKAgeRanges(parameters);
    }
    else if (!strcmp(currentCommand, commands[2]))
    {
        return executeSearchPatientRecord(parameters);
    }
    else if (!strcmp(currentCommand, commands[3]))
    {
        return executeNumPatientAdmissions(parameters);
    }
    else if (!strcmp(currentCommand, commands[4]))
    {
        return executeNumPatientsDischarges(parameters);
    }
    else{
        char* commandNotFound = new char[100];
        strcpy(commandNotFound, "Command Not found!\n");
        return commandNotFound;
    }
}

void Server::GetStatistics(char* buffer)
{
    char* position_ptr = nullptr;
    int position = 0;
    position_ptr = strchr(buffer + position, '\n');
    int positionNew = position_ptr - buffer;
    char workerIP[200];
    for (int j = position; j < positionNew; j++)
    {
        workerIP[j - position] = buffer[j];
    }
    workerIP[positionNew - position] = '\0';
    position = positionNew + 1;

    position_ptr = strchr(buffer + position, '\n');
    positionNew = position_ptr - buffer;
    char workerPortString[200];
    for (int j = position; j < positionNew; j++)
    {
        workerPortString[j - position] = buffer[j];
    }
    workerPortString[positionNew - position] = '\0';
    position = positionNew + 1;
    int workerPort = atoi(workerPortString);

    //printf("Got IP:%s on port: %d\n", workerIP, workerPort);

    bool isFirtsElement = true;
    while((position_ptr = strchr(buffer + position, '\n')) != nullptr){
        int positionNew = position_ptr - buffer;
        char dateStr[256];
        for (int j = position; j < positionNew; j++)
        {
            dateStr[j - position] = buffer[j];
        }
        dateStr[positionNew - position] = '\0';
        //printf("date:%s\n", dateStr);
        position = positionNew + 1;

        uint date[4];
        readDate(dateStr, date);

        //printf("Continuing...1\n");
        char country[256];
        position_ptr = strchr(buffer + position, '\n');
        positionNew = position_ptr - buffer;
        for (int j = position; j < positionNew; j++)
        {
            country[j - position] = buffer[j];
        }
        country[positionNew - position] = '\0';
        position = positionNew + 1;

        RedBlackTree<StatisticsElement>* rbTree = statistics.GetElement(country);
        if (isFirtsElement)
        {
            //printf("%d %d\n", isFirtsElement, rbTree);
            // This means that the server already have the info
            // The worker server must have dropped and is resending the statistics.
            if (rbTree){
                // Update all elements pointing to the same port.
                workerServerInfo* wrkServerInfo = workerServerHashTable.GetElement(country);
                int previousPort = wrkServerInfo->port;
                // Lock until update
                int err = pthread_mutex_lock(&data_mtx);
                if (err){
                    perror2("pthread_mutex_lock", err);
                    exit(1);
                }
                for (GenericLinkedListIterator<MyString> countryIterator(countryNames); !countryIterator.End(); countryIterator++) {
                    char* currentCountry = (*(countryIterator.GetElementOnIterator())).data;
                    workerServerInfo* wrkServerInfo = workerServerHashTable.GetElement(currentCountry);
                    if (wrkServerInfo->port == previousPort){
                        wrkServerInfo->port = workerPort;
                    }
                }
                err = pthread_mutex_unlock(&data_mtx);
                if (err){
                    perror2("pthread_mutex_unlock", err);
                    exit(1);
                }
                // Break from the loop the data structures should not be updated also.
                break;
            }
            isFirtsElement = false;
        }
        //printf("Continuing...2\n");
        if (!rbTree){
            rbTree = new RedBlackTree<StatisticsElement>();
            int err = pthread_mutex_lock(&data_mtx);
            if (err){
                perror2("pthread_mutex_lock", err);
                exit(1);
            }

            statistics.InsertElement(country, rbTree);
            countryNames.Append(MyString(country));
            workerServerInfo* wsi = new workerServerInfo();
            wsi->port = workerPort;
            wsi->ipAddress = MyString(workerIP);
            workerServerHashTable.InsertElement(country, wsi);

            err = pthread_mutex_unlock(&data_mtx);
            if (err){
                perror2("pthread_mutex_unlock", err);
                exit(1);
            }
        }
        //printf("Continuing...3\n");
        char disease[256];
        position_ptr = strchr(buffer + position, '\n');
        positionNew = position_ptr - buffer;
        for (int j = position; j < positionNew; j++)
        {
            disease[j - position] = buffer[j];
        }
        disease[positionNew - position] = '\0';
        position = positionNew + 1;

        while(strcmp(disease, "END_DISEASE_LOG") != 0){
            int ageRange[4];
            char range[256];
            for (int k = 0; k < 4; k++)
            {
                position_ptr = strchr(buffer + position, '\n');
                positionNew = position_ptr - buffer;
                for (int j = position; j < positionNew; j++) {
                    range[j - position] = buffer[j];
                }
                range[positionNew - position] = '\0';
                position = positionNew + 1;
                ageRange[k] = atoi(range);
            }

            StatisticsElement* statisticsElement = new StatisticsElement(disease, ageRange);

            int err = pthread_mutex_lock(&data_mtx);
            if (err){
                perror2("pthread_mutex_lock", err);
                exit(1);
            }
            rbTree->InsertElement(date, statisticsElement);
            err = pthread_mutex_unlock(&data_mtx);
            if (err){
                perror2("pthread_mutex_unlock", err);
                exit(1);
            }

            position_ptr = strchr(buffer + position, '\n');
            positionNew = position_ptr - buffer;
            for (int j = position; j < positionNew; j++)
            {
                disease[j - position] = buffer[j];
            }
            disease[positionNew - position] = '\0';
            position = positionNew + 1;
        }
    }
}


























