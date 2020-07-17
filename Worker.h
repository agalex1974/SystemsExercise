//
// Created by marios on 24/5/20.
//

#ifndef SYSTEMS2NDEXERCISE_WORKER_H
#define SYSTEMS2NDEXERCISE_WORKER_H

#define perror2(s, e) fprintf(stderr, "%s: %s\n", s, strerror(e))

namespace exercise3namespace
{
    class Worker {
    private:
        static pthread_mutex_t cond_worker_mtx;
        static pthread_cond_t  cvar_worker_non_full;
        static pthread_mutex_t data_worker_mtx;
        static pthread_cond_t  cvar_worker_non_empty;
        static pthread_mutex_t listener_cond_mtx;
        static pthread_cond_t  listener_cvar;

        const char commands[3][100] = {
                "/searchPatientRecord",
                "/numPatientAdmissions",
                "/numPatientDischarges"
        };
        static const uint stringLength = 256;
        struct AgeRanges{
            int range[4];
            AgeRanges(){
                range[0] = 0;
                range[1] = 0;
                range[2] = 0;
                range[3] = 0;
            }
            void addAge(int age)
            {
                if (age < 21) range[0]++;
                else if (age < 41) range[1]++;
                else if (age < 61) range[2]++;
                else range[3]++;
            }
        };
        GenericHashTable<RedBlackTree<PatientEntry>> patientsEntryHashTree;
        GenericHashTable<RedBlackTree<PatientEntry>> patientsExitHashTree;
        NamedPipeInOut* namedPipeInOut;
        void ReadFiles(const char* country, const char* path, MyString* buffer);
        bool ReadFile(const char *country, const char* dateStr, const char *path,
                      GenericHashTable<AgeRanges>* pAgeRangesHash, GenericLinkedList<MyString>* pDiseaseIdList);
        GenericLinkedList<PatientEntry*> patients;
        GenericHashTable<PatientEntry> patientHashTable;
        GenericLinkedList<MyString> countryNames;
        bool readDate(const char*, uint*);
        void fillBuffer(GenericHashTable<AgeRanges>* pAgesHash, GenericLinkedList<MyString>* pDiseaseList, MyString* buffer);

        char* readCommand(const char *commandString);
        char* executeCommand(const char* currentCommand, const char* parameters);

        char* NumPatientAdmissionsCommand(const char* parameters);
        char* NumPatientAdmissions(const char* disease, const char* date1String, const char* date2String, const char* country);
        char* NumPatientDischargesCommand(const char *parameters);
        char* NumPatientDischarges(const char* disease, const char* date1String, const char* date2String, const char* country);
        char* SearchPatientRecordCommand(const char *parameters);
        char* SearchPatientRecord(const char* recordIDString);

        MyString DateToString(uint date[3]);
        int succeededCommands;
        int failedCommands;
        int totalCommands;

        SendReceiveProtocol sendReceiveProtocol;
        const char* inet_address = "192.168.1.23";
        sockaddr_in server_worker;

        // The IP address of who server
        MyString serverIPAddress;
        // The port of who server
        int serverPort;

        // The socket on which the server of the worker is listening
        int socket_worker;

        void SendStatisticsBuffer(const char* statisticsBuffer);

        struct InputToThreadWorkerServer{
            Worker* worker;
            int socket;
        };

        struct cyclic_buffer{
            int* elements;
            int start;
            int end;
            int count;
            int max_size;
        };

        pthread_t* threadPool;
        static void* threadConsumer(void* argp);
    public:
        cyclic_buffer cyclicBuffer;
        Worker(const char* initialization, NamedPipeInOut* namedPipeInOut);
        char* processCommand(char* buffer);
        void AcceptCommandsFromServer();
        ~Worker(){
            patients.ErasePatientEntries();
            delete[] cyclicBuffer.elements;
            delete[] threadPool;
        }
    };
}

#endif //SYSTEMS2NDEXERCISE_WORKER_H
