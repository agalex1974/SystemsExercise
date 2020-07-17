//
// Created by marios on 20/6/20.
//

#ifndef SYSTEMSEXERCISE3_SERVER_H
#define SYSTEMSEXERCISE3_SERVER_H

#define perror2(s, e) fprintf(stderr, "%s: %s\n", s, strerror(e))

namespace exercise3namespace
{
    class Server {
        using uint = unsigned int;
        const char commands[5][100] = {
                "/diseaseFrequency",       //handled by server
                "/topk-AgeRanges",         //handled by server
                "/searchPatientRecord",    //handled by worker
                "/numPatientAdmissions",   //handled by worker
                "/numPatientDischarges",   //handled by worker
        };

        //
        static pthread_mutex_t cond_mtx;
        static pthread_cond_t cvar_non_full;
        static pthread_mutex_t data_mtx;
        static pthread_cond_t cvar_non_empty;
        static pthread_mutex_t listener_cond_mtx;
        static pthread_cond_t listener_cvar;
        static int is_listener_signaled;
        // Enumeration of the type of connection
        enum TypeOfConnection{
            WORKER,
            CLIENT
        };

        // to socket den einai tipota apo enas integer pou anathetete
        // otan ginetai connection
        struct cyclic_buffer_element{
            // The socket to which the server will communicate with the child
            int socket;
            // The type of connection either from Worker or Client
            TypeOfConnection type;
        };

        // kyklikos buffer pou exyphretei ton client kai ton worker
        struct cyclic_buffer{
            cyclic_buffer_element* elements;
            int start;
            int end;
            int count;
            int max_size;
        };

        struct data_for_listener{
            sockaddr_in* pServer;
            uint socket;
            cyclic_buffer* cyclicBuffer;
        };

        SendReceiveProtocol sendReceiveProtocol;

        // These are the server and client socket addresses
        sockaddr_in server_query, server_statistics;
        int sock_query, sock_statistics;
        uint queryPortNumber;
        uint statisticsPortNumber;
        const char* inet_address = "192.168.1.23";
        uint buffer_size;

        // The thread pool
        pthread_t* threadPool;
        static void* threadConsumer(void* argp);
        static void* ListenToQueryRequests(void* args);
        static void* ListenToStatisticsRequests(void* args);
        char* executeCommand(const char* currentCommand, const char* parameters);
        char* executeDiseaseFrequency(const char* parameters);
        char* DiseaseFrequency(const char* virusName, const char* date1String, const char* date2String, const char* country);
        char* executeTopKAgeRanges(const char *parameters);
        char* TopKAgeRanges(const char* kstring, const char* country, const char* virusName, const char* date1String, const char* date2String);
        char* executeSearchPatientRecord(const char *parameters);
        char* SearchPatientRecord(const char* recordId);
        char* executeNumPatientAdmissions(const char* parameters);
        char* NumPatientAdmissions(const char *virusName, const char *date1, const char *date2, const char *country);
        char* executeNumPatientsDischarges(const char *parameters);
        char* NumPatientDischarges(const char *virusName, const char *date1, const char *date2, const char *country);

        // The worker server info
        struct workerServerInfo{
            MyString ipAddress;
            uint port;
        };

        // HashTable where each country point to a worker server info
        GenericHashTable<workerServerInfo> workerServerHashTable;

        // These are the country names
        GenericLinkedList<MyString> countryNames;

        // The statistics red black tree
        GenericHashTable<RedBlackTree<StatisticsElement>> statistics;
        bool readDate(const char* stringDate, uint* date);
        struct AgeRange{
            int index;
            int ageRange;
        };
        static int compare(const void * a, const void * b);
    public:
        // The cyclic buffer
        cyclic_buffer cyclicBuffer;
        Server(uint queryPortNumber, uint statisticsPortNumber, uint threadCount, uint bufferSize);
        char* readCommand(const char *commandString, char* currentCommand);
        void Listen();
        void GetStatistics(char* buffer);
        ~Server();
    };
}
#endif //SYSTEMSEXERCISE3_SERVER_H
