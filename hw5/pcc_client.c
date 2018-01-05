//
// Created by okleinfeld on 1/5/18.
//

#include "pcc_client.h"

int main(int argc, char *argv[]) {
    if (argc != 4){
        printf(PROGRAM_ARG_ERROR);
        exit(-1);
    }

    char* localHost = "127.0.0.1";
    unsigned short port = (unsigned short) atoi(argv[2]);
    unsigned length = (unsigned) atoi(argv[3]);

    // Create new socket
    int sockFd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockFd < 0){
        printf(SOCKET_CREATE_ERROR, strerror(errno));
        exit(-1);
    }

    // Prepare dataBuffer structures for tcp connection
    struct sockaddr_in serverAddress;
//    socklen_t addressSize = sizeof(struct sockaddr_in);
    memset(&serverAddress, 0, sizeof(serverAddress)); // initiate all bytes in structure to zero
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port); //htons for endiannes
    serverAddress.sin_addr.s_addr = inet_addr(localHost);

    // Initiate a tcp connection
    int connectStatus = connect(sockFd, (struct sockaddr*) &serverAddress, sizeof(serverAddress));
    if (connectStatus < 0){
        printf(CONNECT_ERROR, strerror(errno));
        exit(-1);
    }

    // Assign buffer to hold the data we wish to send to server
    char* dataBuffer = (char*) calloc(length, sizeof(char));
    if (dataBuffer == NULL){
        printf(MEMORY_ALLOC_ERROR);
        exit(-1);
    }

    // read the data from file
    char* fileName = "simple_file.txt";
    int dataFd = open(fileName, O_RDONLY);
    if (dataFd < 0){
        printf(OPEN_FILE_ERROR, fileName, strerror(errno));
        exit(-1);
    }

    unsigned totalBytesRead = 0;
    while (totalBytesRead < length){
        long currentBytesRead = read(dataFd, dataBuffer + totalBytesRead, length - totalBytesRead);
        if (currentBytesRead < 0){
            printf(READ_FILE_ERROR, strerror(errno));
            exit(-1);
        }
        totalBytesRead += currentBytesRead;
    }

    if (close(dataFd) < 0){  // close the data file - we don't need it anymore in this program
        printf(CLOSE_FILE_ERROR, strerror(errno));
        exit(-1);
    };

    // write data to server
    int totalBytesSent = 0;
    while (totalBytesSent < length){
        long currentBytesSent = write(sockFd, dataBuffer + totalBytesSent, length - totalBytesSent);
        if (currentBytesSent < 0){
            printf(WRITE_SOCKET_ERROR, strerror(errno));
            exit(-1);
        }
        totalBytesSent += currentBytesSent;
    }

    free(dataBuffer); // send our message already - can free the data buffer

    // read answer for server
    unsigned numPrintableChars;
    long bytesRead = read(sockFd, &numPrintableChars, 1);
    if (bytesRead < 0){
        printf(READ_SOCKET_ERROR, strerror(errno));
        exit(-1);
    }

    // print the answer
    printf("# of printable characters: %u\n", numPrintableChars);

    // close the connection from client side (only) and exit
    if (close(sockFd) < 0){
        printf(CLOSE_SOCKET_ERROR, strerror(errno));
        exit(-1);
    };

    return 0;
}
