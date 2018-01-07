//
// Created by okleinfeld on 1/5/18.
//

#ifndef OS_COURSE_PCC_SERVER_H
#define OS_COURSE_PCC_SERVER_H


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>

#define NUM_PRINTABLE_CHARS 95
#define MAX_LISTEN_QUEUE 100
#define CLEANUP_FREQ 10
#define HEADER_LENGTH 4
#define PRINTABLE_OFFSET 32
#define PROGRAM_ARG_ERROR "Error: You must specify only one value between 0 and 65535, representing the desired port number\n"
#define SOCKET_CREATE_ERROR "Error: Failed to create a socket due to the following error: %s\n"
#define BIND_ERROR "Error: Couldn't bind socket due to the following error: %s\n"
#define LISTEN_ERROR "Error: Listen command failed for the socket due to the following error: %s\n"
#define ACCEPT_ERROR "Error: Failed to create a new connection due to the following error: %s\n"
#define MUTEX_INIT_ERROR "Error: Failed to initiate mutex due to the following error: %s\n"
#define MEMORY_ALLOC_ERROR "Error: Memory allocation error, connection could not be established\n"
#define THREAD_CREATE_ERROR "Error: Failed to create a new thread to handle connection due to the following error: %s\n"
#define LOCK_ERROR "Error: Could not acquire/release lock due to the following reason %s\n"
#define READ_SOCKET_ERROR "Error: Failed to read answer from server due to the following error %s\n"
#define WRITE_SOCKET_ERROR "Error: Failed to write answer to clien due to the following error %s\n"

typedef struct connection{
    int connectionFd;
    pthread_t threadId;
    pthread_mutex_t* sharedPccLock;
    pthread_mutex_t* sharedConnectionsLock;
    unsigned * sharedPCC;
    bool connectionIsOpen;
    struct connection* next;
} CONNECTION;

typedef struct connections_list{
    CONNECTION* head;
} CONNECTIONS_LIST;

CONNECTION* createConnection(int connectionFd, pthread_mutex_t* pccLock, pthread_mutex_t* connectionsLock, unsigned* sharedPPC);
void destroyConnection(CONNECTION* connection);
void addConnectionToList(CONNECTIONS_LIST* list, CONNECTION* connection);
void removeClosedConnectionFromList(CONNECTIONS_LIST *list);
bool isPrintableCharacter(char c);
int getPortNumber(char* string);
void* connectionResponse(void* threadAttributes);
unsigned parseHeader(void* header);
unsigned updateSharedPcc(unsigned N, const char *message, unsigned *sharedPcc);

#endif //OS_COURSE_PCC_SERVER_H
