//
// Created by okleinfeld on 1/5/18.
//

#include "pcc_client.h"

int main(int argc, char *argv[]) {
    if (argc != 4){
        printf(PROGRAM_ARG_ERROR);
        exit(-1);
    }

    char* serverAddressString = argv[1];
    unsigned short port = (unsigned short) atoi(argv[2]);
    unsigned length = (unsigned) atoi(argv[3]);

    // Prepare dataBuffer structures for tcp connection
    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress)); // initiate all bytes in structure to zero
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port); //htons for endiannes

    // Convert the server address from URL or dot-notation ip address to in_addr structure
    int invalidIpString;

    // Try to convert from ip address in a dot notation string

    invalidIpString = inet_aton(serverAddressString, &serverAddress.sin_addr);
    if (invalidIpString == 0) {
        // Try to translate from host name to ip address using DNS query

        struct addrinfo* dnsResult;
        int dnsError;

        // Resolve the domain name into a list of addresses */
        dnsError = getaddrinfo(serverAddressString, NULL, NULL, &dnsResult);
        if (dnsError != 0) {
            printf(ARG_TRANS_ERROR, gai_strerror(dnsError));
            exit(-1);
        }

        // Get the first DNS lookup result - output ip address as string
        char hostIpAddress[MAX_DNS_IP_RES];
        dnsError = getnameinfo(dnsResult->ai_addr, dnsResult->ai_addrlen, hostIpAddress, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
        if (dnsError != 0){
            freeaddrinfo(dnsResult);
            printf(ARG_TRANS_ERROR, gai_strerror(dnsError));
            exit(-1);
        }

        freeaddrinfo(dnsResult);
        printf("Host name is %s, and its ip address dot notation is: %s\n", argv[1], hostIpAddress);

        // Now that we got the ip in a dot notation string, convert it to the desired structure to use with connect

        invalidIpString = inet_aton(hostIpAddress, &serverAddress.sin_addr);
        if (invalidIpString == 0){
            printf(ARG_TRANS_ERROR, strerror(errno));
            exit(-1);
        }
    }

    // Create new socket
    int sockFd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockFd < 0){
        printf(SOCKET_CREATE_ERROR, strerror(errno));
        exit(-1);
    }

    // Initiate a tcp connection
    int connectStatus = connect(sockFd, (struct sockaddr*) &serverAddress, sizeof(serverAddress));
    if (connectStatus < 0){
        printf(CONNECT_ERROR, strerror(errno));
        exit(-1);
    }

    // Assign buffer to hold "protocol header" - 4 bytes to hold "N"

    unsigned int header = length;

    // Assign buffer to hold the data we wish to send to server
    char* dataBuffer = (char*) calloc(length, sizeof(char));
    if (dataBuffer == NULL){
        printf(MEMORY_ALLOC_ERROR);
        exit(-1);
    }

    // read the data from file
    char* fileName = RANDOM_BYTE_GENERATOR;
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

    // first write header
    unsigned long totalBytesHeader = sizeof(unsigned int);
    unsigned long numBytesHeaderSent = 0;
    while (numBytesHeaderSent < totalBytesHeader){
        long currentByteHeaderSent = write(sockFd, &header + numBytesHeaderSent, totalBytesHeader - numBytesHeaderSent);
        if (currentByteHeaderSent < 0){
            printf(WRITE_SOCKET_ERROR, strerror(errno));
            exit(-1);
        }
        numBytesHeaderSent += currentByteHeaderSent;
    }

//    printf("Wrote the header!\n");

    // then write message itself
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

//    printf("Wrote the data!\n");

    // read answer for server
    unsigned numPrintableChars;
    unsigned answerSizeToRead = sizeof(unsigned);
    unsigned totalAnsBytesRead = 0;
    while (totalAnsBytesRead < answerSizeToRead){
        long currentBytesRead = read(sockFd, &numPrintableChars + totalAnsBytesRead, answerSizeToRead - totalAnsBytesRead);
        if (currentBytesRead < 0){
            printf(READ_SOCKET_ERROR, strerror(errno));
            exit(-1);
        }
        totalAnsBytesRead += currentBytesRead;
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
