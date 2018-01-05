//
// Created by okleinfeld on 1/5/18.
//

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

#define PROGRAM_ARG_ERROR "Error: You must specify only one value between 0 and 65535, representing the desired port number\n"
#define SOCKET_CREATE_ERROR "Error: Failed to create a socket due to following error: %s\n"

bool isPrintableCharacter(char);
int getPortNumber(char* string);


pthread_mutex_t pccLock;

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

    struct sockaddr_in serverAddress;
    socklen_t addressSize = sizeof(struct sockaddr_in);
    memset(&serverAddress, 0, addressSize); // fill the structure with 0 on all bytes
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_port = htons(port);



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