cmake_minimum_required(VERSION 3.13)
project(SystemsExercise3)

set(CMAKE_CXX_STANDARD 14)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(whoClient main.cpp Client.cpp Client.h SendReceiveProtocol.cpp SendReceiveProtocol.h)
target_link_libraries(whoClient Threads::Threads)

add_executable(whoServer Server.cpp Server.h mainServer.cpp PatientEntry.cpp PatientEntry.h
        RedBlackTrees.cpp RedBlackTrees.h DiseaseStatistics.cpp DiseaseStatistics.h
        StatisticsElement.cpp StatisticsElement.h GenericlLinkedList.cpp GenericlLinkedList.h SendReceiveProtocol.cpp SendReceiveProtocol.h)
target_link_libraries(whoServer Threads::Threads)

add_executable(Worker NamedPipeInOut.cpp NamedPipeInOut.h mainWorker.cpp GenericHashTable.cpp GenericHashTable.h
        Worker.cpp Worker.h GenericlLinkedList.h GenericlLinkedList.h GenericlLinkedList.cpp MyString.cpp MyString.h PatientEntry.h PatientEntry.cpp
        RedBlackTrees.cpp RedBlackTrees.h SendReceiveProtocol.cpp SendReceiveProtocol.h)
target_link_libraries(Worker Threads::Threads)

add_executable(Master mainMaster.cpp NamedPipeInOut.cpp NamedPipeInOut.h
        PipesHashTable.h PipesHashTable.cpp GenericlLinkedList.cpp GenericlLinkedList.h NamedPipe.cpp NamedPipe.h
        Master.cpp Master.h GenericHashTable.cpp GenericHashTable.h MyString.cpp MyString.h)