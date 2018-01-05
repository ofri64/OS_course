//
// Created by okleinfeld on 1/5/18.
//

#include "pcc_server.h"

int main(int argc, char *argv[]){
    if (argc != 2){
        printf(PROGRAM_ARG_ERROR);
        exit(-1);
    }
    int portCheck = getPortNumber(argv[1]);
    if (portCheck < 0){
        printf(PROGRAM_ARG_ERROR);
        exit(-1);
    }

    unsigned short port = (unsigned short) portCheck;
    int ppc_total[NUM_PRINTABLE_CHARS] = {0};

    // Create a new socket
    int listenFd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd < 0){
        printf(SOCKET_CREATE_ERROR, strerror(errno));
        exit(-1);
    }

    // Prepare the server protocol, ip, and port variables before binding it to the socket
    struct sockaddr_in serverAddress;
    socklen_t addressSize = sizeof(struct sockaddr_in);
    memset(&serverAddress, 0, addressSize); // fill the structure with 0 on all bytes
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_port = htons(port);

    // Bind the server values (including the given port number) to the socket
    int bindStatus = bind(listenFd, (struct sockaddr*) &serverAddress, addressSize);
    if (bindStatus < 0){
        printf(BIND_ERROR, strerror(errno));
        exit(-1);
    }

    // Mark this socket as a passive socket that will be used to accept connection
    int listenStatue = listen(listenFd, MAX_LISTEN_QUEUE);
    if (listenStatue < 0){
        printf(LISTEN_ERROR, strerror(errno));
        exit(-1);
    }

    // Preparation steps before accepting connections
    pthread_mutex_t pccLock;
    pthread_mutex_t connectionLock;
    int initPccLockError = pthread_mutex_init(&pccLock, NULL);
    int initCleanupLockError = pthread_mutex_init(&connectionLock, NULL);
    if (initPccLockError != 0 || initCleanupLockError !=0){
        printf(MUTEX_INIT_ERROR, strerror(errno));
        exit(-1);
    }

    // Create a connection list - data structure (linked list) for open connections
    CONNECTIONS_LIST list;
    list.head = NULL;
    int connectionsOpened = 0;

    // From now on - accept connections and manage every connection in a different thread
    while (1){
        // Create new socket for new connection (identified by four tuple ip-port ip-port)
        int connFd = accept(listenFd, NULL, NULL);
        if (connFd < 0){
            printf(ACCEPT_ERROR, strerror(errno));
            exit(-1);
        }

        connectionsOpened++; // increment, will be used for clean up logic

        // Prepare data for a new connection handled in a separate thread

        // Create a new connection object
        CONNECTION* connection = createConnection(connFd, &pccLock, &connectionLock, ppc_total);
        if (connection == NULL){
            printf(MEMORY_ALLOC_ERROR);
            continue;
        }

        // Add new connection to open connections list
        addConnectionToList(&list, connection);

        // Create new thread to handle server response
        int threadError = pthread_create(&connection->threadId, NULL, connectionResponse, (void *) connection);
        if (threadError != 0){
            printf(THREAD_CREATE_ERROR, strerror(errno));
            continue;
        }

        if (connectionsOpened % CLEANUP_FREQ == 0){ // clean up memory for connections that ended
            //TODO: Implement clean up closed connections logic. first acquire lock .
        }


        break;
    }


    //TODO: Make sure we clean up everything upon SIGINT
    // TODO: have to destroy 2 mutexes and close listening port
    pthread_mutex_destroy(&pccLock);
    close(listenFd);
}




bool isPrintableCharacter(char c){
    if (c >= 32 && c <= 126)
        return true;
    else
        return false;
}

int getPortNumber(char* string){
    int portNum = atoi(string);
    if (portNum == 0 && strcmp(string, "0") != 0){
        // Atoi returned 0 so the string doesn't represent a number
        // But still there is the special case when atoi returned 0 but it is valid because the string is "0"
        return -1;
    }

    if (portNum < 0 || portNum > 65535){
        // Negative number or positive but longer the unsigned short can hold
        return -1;
    }

    return portNum;
}

CONNECTION* createConnection(int connectionFd, pthread_mutex_t* lock,pthread_mutex_t* connectionsLock ,int* sharedPPC){
    CONNECTION* connection = (CONNECTION*) malloc(sizeof(CONNECTION));
    if (connection == NULL){
        return NULL;
    }

    connection->connectionFd = connectionFd;
    connection->sharedPccLock = lock;
    connection->sharedConnectionsLock = connectionsLock;
    connection->sharedPCC = sharedPPC;
    connection->threadId = 0;
    connection->connectionIsOpen = true;
    connection->next = NULL;
    return connection;
}

void destroyConnection(CONNECTION* connection){
    if (connection != NULL){
        free(connection);
    }
}

void addConnectionToList(CONNECTIONS_LIST* list, CONNECTION* connection){
    if (list == NULL || connection == NULL){
        return;
    }
    if (list->head == NULL){
        list->head = connection;
    } else{
        connection->next = list->head;
        list->head = connection;
    }
}

void removeClosedConnectionFromList(CONNECTIONS_LIST *list) {
    if (list == NULL) {
        return;
    }
    CONNECTION *head = list->head;
    CONNECTION *prev = list->head;
    CONNECTION *current = list->head->next;
    CONNECTION *connectionToRemove;
    // remove all closed connection - not included the list head
    while (current != NULL) {
        if (current->connectionIsOpen) {
            prev = current;
            current = current->next;
        } else {
            connectionToRemove = current;
            current = current->next;
            prev->next = current;
            destroyConnection(connectionToRemove);
        }
    }

    if (!head->connectionIsOpen) { // need to remove head of list
        list->head = head->next; // ok even if now head is NULL
    }
}
