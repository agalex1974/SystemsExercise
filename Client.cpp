//
// Created by marios on 18/6/20.
//

#include <pthread.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "MyString.h"
#include "SendReceiveProtocol.h"
#include "Client.h"

using namespace exercise3namespace;

pthread_mutex_t Client::data_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t Client::cond_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t Client::cvar = PTHREAD_COND_INITIALIZER;
pthread_mutex_t Client::cond_mtx_order_client = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t Client::cvar_order_client = PTHREAD_COND_INITIALIZER;
int Client::thread_counter = 0;
int Client::thread_total_count = 0;
int Client::is_father_signaled = 0;
Client::Client(uint nThreads, const char* fileName, uint portNumber, const char *dotInetAddress):
    QueryFileName(fileName),
    server(),
    portNumber(portNumber),
    threadCount(nThreads),
    sendReceiveProtocol(1024)
{
    thread_total_count = threadCount;
    server.sin_family = AF_INET;
    server.sin_port = htons(portNumber);
    server.sin_addr.s_addr = inet_addr(dotInetAddress);
}

void* Client::thread_query(void *argp)
{
    query_thread_input* input = (query_thread_input*) argp;

    int err = pthread_mutex_lock(&cond_mtx); //oc
    if (err){
        perror2("pthread_mutex_lock", err);
        exit(1);
    }

    err = pthread_mutex_lock(&cond_mtx_order_client); //of
    if (err){
        perror2("pthread_mutex_lock", err);
        exit(1);
    }
    thread_counter++;
    //elefthervnv ton patera mono otan eimai to teleftaio thread
    if (thread_counter == thread_total_count) {
        is_father_signaled = 1;
        pthread_cond_signal(&cvar_order_client);
    }

    err = pthread_mutex_unlock(&cond_mtx_order_client);

    if (err){
        perror2("pthread_mutex_unlock", err);
        exit(1);
    }

    //printf("I am thread %ld and I am waiting to execute command %s total threads %d\n", pthread_self(), input->command, thread_total_count);
    // O pateras tha perimenei to oc kai tha to pairnei mono otan to teleftaio paidi bei se anamonh
    pthread_cond_wait(&cvar, &cond_mtx);

    //printf("I am firing command: %s\n", input->command);

    err = pthread_mutex_unlock(&cond_mtx);
    if (err){
        perror2("pthread_mutex_unlock", err);
        exit(1);
    }
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0){
        perror("socket");
        exit(-1);
    }
    int res = connect(sock, (sockaddr *)input->server, sizeof(*(input->server)));
    if (res < 0){
        perror("connect");
        exit(-1);
    }
    input->sendReceiveProtocol->send(sock, input->command);
    char* result = input->sendReceiveProtocol->receive(sock);
    err = pthread_mutex_lock(&data_mtx); //oc
    if (err){
        perror2("pthread_mutex_lock", err);
        exit(1);
    }
    printf("output of command:%s\n%s\n", input->command, result);
    err = pthread_mutex_unlock(&data_mtx);
    if (err){
        perror2("pthread_mutex_unlock", err);
        exit(1);
    }
    delete[] result;
    close(sock);
    pthread_exit(NULL);
}

void Client::execute_query_file(){
    uint totalCommands = 0;
    FILE* file = fopen(QueryFileName.data, "r");
    char line[1024];
    while (fgets(line, sizeof(line), file))
    {
        totalCommands++;
    }
    fclose(file);

    printf("Processing %d commands\n", totalCommands);

    int err;
    pthread_t* threadPool = new pthread_t[threadCount];
    file = fopen(QueryFileName.data, "r");
    MyString* commands = new MyString[threadCount];
    query_thread_input* input = new query_thread_input[threadCount];

    uint counter = 0;
    uint processedCommands = 0;

    while (fgets(line, sizeof(line), file)) {
        if (!counter){
            is_father_signaled = 0;
            thread_total_count = MIN(totalCommands - processedCommands, threadCount);
        }
        //line[strlen(line)-1] = '\0';
        commands[counter] = MyString(line);
        input[counter].command = commands[counter].data;
        input[counter].server = &server;
        input[counter].sendReceiveProtocol = &sendReceiveProtocol;
        err = pthread_create(threadPool + counter, NULL, thread_query, (void*)&input[counter]);
        if (err){
            perror2("pthread_create", err);
            exit(1);
        }

        if (counter == threadCount - 1){
            /////////////////////////////////////////////////////////
            ///Bainw se katastash lock prwtos?
            ///Nai (is_father_signaled=0): Tote to paidi perimenei mexris otou o pateras na bei se ypno (pthread_cond_wait)
            //     bainontas se ypno elefthervnv to mutex kai afhnv to paidi na mpei kai na me eidopoihsei an einai
            //     to teleftaio. An me eidopoihsei tote pali tha perimenv san pateras mexri na elegthervsei to kleidi
            //     kai tha synexisv giati eimai svstos meta giati, exei to kleidi tou conditioner to paidi to teleftaio
            //     kai den borei o pateras na steilei shma energopoihseis para mono otan kai to teleftaio paidi bei
            ///    se anamonh.
            ///Oxi (is_father_signaled=1): Gia na ginei is_father_signal=1 shmainei oti to paidi to ekane afto shmainei
            ///    oti to paidi exei to conditioner lock ara tha synexisei o pateras kai tha perimenei kai to teleftaio
            //     paidi na bei se anamonh.
            int err = pthread_mutex_lock(&cond_mtx_order_client); //of
            if (err){
                perror2("pthread_mutex_lock", err);
                exit(1);
            }
            if (!is_father_signaled)
                pthread_cond_wait(&cvar_order_client, &cond_mtx_order_client);
            //printf("parent woken!\n");

            err = pthread_mutex_unlock(&cond_mtx_order_client);
            if (err){
                perror2("pthread_mutex_unlock", err);
                exit(1);
            }
            /////////////////////////////////////////////////////////

            ///////////////////////////////////////////////////////////////
            err = pthread_mutex_lock(&cond_mtx); //oc
            if (err){
                perror2("pthread_mutex_lock", err);
                exit(1);
            }
            pthread_cond_broadcast(&cvar);
            err = pthread_mutex_unlock(&cond_mtx);
            if (err){
                perror2("pthread_mutex_unlock", err);
                exit(1);
            }
            /////////////////////////////////////////////////////////////////

            for (int threadIndex = 0; threadIndex < threadCount; threadIndex++){
                err = pthread_join(threadPool[threadIndex], NULL);
                if (err){
                    perror2("pthread_join", err);
                    exit(1);
                }
            }
            thread_counter = 0;
        }

        processedCommands++;
        counter++;
        counter = counter % threadCount;
    }

    //printf("counter:%d %d\n", counter, thread_total_count);
    if (counter){

        /////////////////////////////////////////////////////////
        int err = pthread_mutex_lock(&cond_mtx_order_client);
        if (err){
            perror2("pthread_mutex_lock", err);
            exit(1);
        }
        if (!is_father_signaled)
            pthread_cond_wait(&cvar_order_client, &cond_mtx_order_client);
        //printf("parent woken!\n");

        err = pthread_mutex_unlock(&cond_mtx_order_client);
        if (err){
            perror2("pthread_mutex_unlock", err);
            exit(1);
        }
        /////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////
        err = pthread_mutex_lock(&cond_mtx);
        if (err){
            perror2("pthread_mutex_lock", err);
            exit(1);
        }
        pthread_cond_broadcast(&cvar);
        err = pthread_mutex_unlock(&cond_mtx);
        if (err){
            perror2("pthread_mutex_unlock", err);
            exit(1);
        }
        /////////////////////////////////////////////////////////////////

        for (int threadIndex = 0; threadIndex < counter; threadIndex++){
            err = pthread_join(threadPool[threadIndex], NULL);
            if (err){
                perror2("pthread_join", err);
                exit(1);
            }
        }
    }
    delete[] threadPool;
    delete[] commands;
    delete[] input;
    fclose(file);

}