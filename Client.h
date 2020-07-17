//
// Created by marios on 18/6/20.
//

#ifndef SYSTEMSEXERCISE3_CLIENT_H
#define SYSTEMSEXERCISE3_CLIENT_H

#define perror2(s, e) fprintf(stderr, "%s: %s\n", s, strerror(e))
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

namespace exercise3namespace{
    class Client{
        //We will use only pure C members
        struct query_thread_input{
            char* command;
            sockaddr_in* server;
            SendReceiveProtocol* sendReceiveProtocol;
        };
        static int thread_counter;
        static int thread_total_count;
        static int is_father_signaled;
        using uint = unsigned int;
        static pthread_mutex_t data_mtx;
        static pthread_mutex_t cond_mtx;
        static pthread_cond_t cvar;
        static pthread_mutex_t cond_mtx_order_client;
        static pthread_cond_t cvar_order_client;
        sockaddr_in server;
        MyString QueryFileName;
        static void* thread_query(void *argp);
        uint portNumber;
        uint threadCount;
        SendReceiveProtocol sendReceiveProtocol;
    public:
        void execute_query_file();
        Client(uint nThreads, const char* fileName, uint portNumber, const char *dotInetAddress);
    };
}


#endif //SYSTEMSEXERCISE3_CLIENT_H
