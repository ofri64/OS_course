//
// Created by okleinfeld on 1/5/18.
//

#include "pcc_server.h"

CONNECTIONS_LIST list = {.head = NULL}; // global connection list
int listenFd; // global listing socket file descriptor
unsigned pcc_total[NUM_PRINTABLE_CHARS] = {0}; // global pcc_counter. has to be global in order to access to it from signal handler

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

    // Assign SIGINT handler function

    struct sigaction interruptAction;
    memset(&interruptAction, 0, sizeof(interruptAction));
    interruptAction.sa_sigaction = interruptHandler;
    interruptAction.sa_flags = SA_SIGINFO;
    if (sigaction(SIGINT, &interruptAction, NULL) != 0){
        printf(SIGNAL_ASSIGN_ERROR, strerror(errno));
        exit(-1);
    }

    // Create a new socket
    listenFd = socket(AF_INET, SOCK_STREAM, 0);
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
    int lockError;
    int initPccLockError = pthread_mutex_init(&pccLock, NULL);
    int initCleanupLockError = pthread_mutex_init(&connectionLock, NULL);
    if (initPccLockError != 0 || initCleanupLockError !=0){
        printf(MUTEX_INIT_ERROR, strerror(errno));
        exit(-1);
    }

    // Initiate variable to number of connection created - will be used for clean up logic
    unsigned connectionsOpened = 0;

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
        CONNECTION* connection = createConnection(connFd, &pccLock, &connectionLock, pcc_total);
        if (connection == NULL){
            printf(MEMORY_ALLOC_ERROR);
            continue;
        }

        // Create new thread to handle server response
        int threadError = pthread_create(&connection->threadId, NULL, connectionResponse, (void *) connection);
        if (threadError != 0){
            destroyConnection(connection);
            printf(THREAD_CREATE_ERROR, strerror(errno));
            continue;
        }

        // Add the new connection to open connections list
        if ((lockError = pthread_mutex_lock(&connectionLock)) != 0){
            printf(LOCK_ERROR, strerror(lockError));
            exit(-1);
        }

        addConnectionToList(&list, connection);

        if ((lockError = pthread_mutex_unlock(&connectionLock)) != 0){
            printf(LOCK_ERROR, strerror(lockError));
            exit(-1);
        }

        // clean up memory for connections that ended - check only once for 10 connection/threads
        if (connectionsOpened % CLEANUP_FREQ == 0){

            if ((lockError = pthread_mutex_lock(&connectionLock)) != 0){
                printf(LOCK_ERROR, strerror(lockError));
                exit(-1);
            }

            removeClosedConnectionFromList(&list);

            if ((lockError = pthread_mutex_unlock(&connectionLock)) != 0){
                printf(LOCK_ERROR, strerror(lockError));
                exit(-1);
            }
        }
    }
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

CONNECTION* createConnection(int connectionFd, pthread_mutex_t* lock,pthread_mutex_t* connectionsLock ,unsigned* sharedPPC){
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
//    printf("We are inside cleanup connection linked list\n");
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

void* connectionResponse(void* threadAttributes){
    CONNECTION* connection = (CONNECTION*) threadAttributes;
    int connFd = connection->connectionFd;
    unsigned totalPcc = 0;
    int lockError;

    // read packet header - 4 bytes length and convert it to unsigned int
    unsigned N;
    unsigned headerLengthBytes = sizeof(unsigned);
    unsigned totalHeaderBytesRead = 0;
    while (totalHeaderBytesRead < headerLengthBytes){
        long currentHeaderBytesRead = read(connFd, &N + totalHeaderBytesRead, headerLengthBytes - totalHeaderBytesRead);
        if (currentHeaderBytesRead < 0){
            printf(READ_SOCKET_ERROR, strerror(errno));
            exit(-1);
        }
        totalHeaderBytesRead += currentHeaderBytesRead;
    }

    printf("Length of data to come is %d\n", N);

    // Allocate space for data packet and initialize variables
    char message[MAX_SERVER_BUFFER_SIZE];
    unsigned totalMessageBytesRead = 0;

    while (totalMessageBytesRead < N){
        // initialize variables for current iteration. maybe required to read less than MAX_SERVER_BUFFER_SIZE
        unsigned iterationTotalMessageBytesRead = 0;

        unsigned numBytesToReadCurrentIter;
        if (totalMessageBytesRead + MAX_SERVER_BUFFER_SIZE > N){
            numBytesToReadCurrentIter = N - totalMessageBytesRead;
        } else{
            numBytesToReadCurrentIter = MAX_SERVER_BUFFER_SIZE;
        }

        // Read current iteration bytes
        while (iterationTotalMessageBytesRead < numBytesToReadCurrentIter){
            long currentMessageBytesRead = read(connFd, message + iterationTotalMessageBytesRead, numBytesToReadCurrentIter - iterationTotalMessageBytesRead);
            if (currentMessageBytesRead < 0){
                printf(READ_SOCKET_ERROR, strerror(errno));
                exit(-1);
            }
            iterationTotalMessageBytesRead += currentMessageBytesRead;
        }

        // Update to shared ppc array. Perform it atomically

        if ((lockError = pthread_mutex_lock(connection->sharedPccLock)) != 0){
            printf(LOCK_ERROR, strerror(lockError));
            exit(-1);
        }

        totalPcc += updateSharedPcc(iterationTotalMessageBytesRead, message, connection->sharedPCC);

        if ((lockError = pthread_mutex_unlock(connection->sharedPccLock)) != 0){
            printf(LOCK_ERROR, strerror(lockError));
            exit(-1);
        }

        printf("Finished iteration, read from client %u bytes and updated pcc counter\n", iterationTotalMessageBytesRead);


        // Update total bytes read
        totalMessageBytesRead += iterationTotalMessageBytesRead;
    }

    printf("Finished reading the entire message\n");


    // Send num of printable chars back to client

    unsigned numAnsBytesSent = 0;
    unsigned numAnsBytesToWrite = sizeof(unsigned);
    while (numAnsBytesSent < numAnsBytesToWrite){
        long currentAnsBytesWrote = write(connFd, &totalPcc + numAnsBytesSent, numAnsBytesToWrite - numAnsBytesSent);
        if (currentAnsBytesWrote < 0){
            printf(WRITE_SOCKET_ERROR, strerror(errno));
            exit(-1);
        }
        numAnsBytesSent += currentAnsBytesWrote;
    }

    printf("Sent answer back to client\n");

    // close the connection from server side
    close(connFd);

    // update this thread finished it's calculation - again atomically

    if ((lockError = pthread_mutex_lock(connection->sharedConnectionsLock)) != 0){
        printf(LOCK_ERROR, strerror(lockError));
        exit(-1);
    }

     connection->connectionIsOpen = false;

    if ((lockError = pthread_mutex_unlock(connection->sharedConnectionsLock)) != 0){
        printf(LOCK_ERROR, strerror(lockError));
        exit(-1);
    }

    printf("updated the connection is closed\n");
    pthread_exit(NULL);
}

unsigned updateSharedPcc(unsigned N, const char *message, unsigned *sharedPcc){
    bool printable;
    unsigned numPrintableChars = 0;
    for (unsigned i = 0; i < N; i++){
        char c = message[i];
        printable = isPrintableCharacter(c);
        if (printable){
            numPrintableChars++;
            int charDecimal = c - PRINTABLE_OFFSET;
            sharedPcc[charDecimal] += 1;
        }
    }
    return numPrintableChars;
}

void interruptHandler(int signum, siginfo_t* info, void* ptr){
    printf("I'm inside interrupt signal handler!\n");

    // First close the listing socket - stopping us from accepting new tcp connections
    close(listenFd);

    // Wait for all threads still on the list to add (some of the connections may already ended)
    CONNECTION* currentConnection = list.head;
    while (currentConnection != NULL){
        pthread_t threadNum = currentConnection->threadId;
        int errorStatus = pthread_join(threadNum, NULL);
        if (errorStatus != 0){
            printf(THREAD_JOIN_ERROR, strerror(errorStatus));
            exit(-1);
        }
        currentConnection = currentConnection->next;
    }

    // Now all threads finished we can call to remove closed connection without locking before
    removeClosedConnectionFromList(&list); // although it is not necessary we will free memory

    // print pcc_counter values
    for (int i = 0; i < NUM_PRINTABLE_CHARS; i++){
        int c = i + PRINTABLE_OFFSET;
        printf("char '%c': %u times\n", c, pcc_total[i]);
    }

    // not destroying locks but rest of the resources are freed
    exit(0);
}
