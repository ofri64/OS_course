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
    int ppc_total[95] = {0};

    // Create a new socket
    int listenFd = socket( AF_INET, SOCK_STREAM, 0 );
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
    int initError = pthread_mutex_init(&pccLock, NULL);
    if (initError != 0){
        printf(MUTEX_INIT_ERROR, strerror(errno));
        exit(-1);
    }

    // Create a connection list - data structure (linked list) for open connections
    CONNECTIONS_LIST list;
    list.head = NULL;

    // From now on - accept connections and manage every connection in a different thread
    while (1){
        // Create new socket for new connection (identified by four tuple ip-port ip-port)
        int connFd = accept(listenFd, NULL, NULL);
        if (connFd < 0){
            printf(ACCEPT_ERROR, strerror(errno));
            exit(-1);
        }

        // Prepare data for a new connection handled in a separate thread

        // Create a new connection object
        CONNECTION* connection = createConnection(connFd, &pccLock, ppc_total);
        if (connection == NULL){
            printf(MEMORY_ALLOC_ERROR);
            continue;
        }

        // Initiate thread data structure
        THREAD_ATTR* connectionThread = assignThreadAttributes(connection, &list);
        if (connectionThread == NULL){
            printf(MEMORY_ALLOC_ERROR);
            continue;
        }
        // Create new Thread
        int threadError = pthread_create(&connection->threadId, NULL, connectionResponse, (void *) connectionThread);
        if (threadError != 0){
            printf(THREAD_CREATE_ERROR, strerror(errno));
            continue;
        }

        break;
    }



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

CONNECTION* createConnection(int connectionFd, pthread_mutex_t* lock, int* sharedPPC){
    CONNECTION* connection = (CONNECTION*) malloc(sizeof(CONNECTION));
    if (connection == NULL){
        return NULL;
    }

    connection->connectionFd = connectionFd;
    connection->sharedLock = lock;
    connection->sharedPCC = sharedPPC;
    connection->threadId = 0;
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

void removeConnectionFromList(CONNECTIONS_LIST* list, CONNECTION* connection){
    if (list == NULL || connection == NULL){
        return;
    }

    CONNECTION* current = list->head;
    if (current == connection) { // special case - remove head of list
        list->head = connection->next;
        return;
    }

    while (current->next != NULL){
        if (current->next == connection){ // found the connection we want to remove
            current->next = connection->next;
            return;
        }
        else{
            current = current->next;
        }
    }
}

THREAD_ATTR* assignThreadAttributes(CONNECTION* connection, CONNECTIONS_LIST* connectionsList){
    THREAD_ATTR* threadAttribute = (THREAD_ATTR*) malloc(sizeof(THREAD_ATTR));
    if (threadAttribute == NULL){
        return NULL;
    }

    threadAttribute->connection = connection;
    threadAttribute->connectionsList = connectionsList;
    return threadAttribute;
}
void destroyThreadAttributes(THREAD_ATTR* threadAttributes){
    if (threadAttributes != NULL){
        free(threadAttributes);
    }
}