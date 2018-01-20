//
// Created by okleinfeld on 1/5/18.
//

#ifndef OS_COURSE_PCC_CLIENT_H
#define OS_COURSE_PCC_CLIENT_H

#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>

#define MAX_DNS_IP_RES 1025
#define MAX_READ_BUFFER_SIZE 1024

#define RANDOM_BYTE_GENERATOR "/dev/urandom"
#define PROGRAM_ARG_ERROR "Error: You must supply 3 argument - ip address, port number and number of bytes to send\n"
#define ARG_TRANS_ERROR "Error: Failed to translate your ip address or URL string. error reason is %s\n"
#define SOCKET_CREATE_ERROR "Error: Failed to create a socket due to the following error: %s\n"
#define CONNECT_ERROR "Error: Couldn't connect to server due to the following error: %s\n"
#define MEMORY_ALLOC_ERROR "Error: Memory allocation error. could not allocate a buffer\n"
#define OPEN_FILE_ERROR "Error: Could not open file %s due to the following error: %s\n"
#define READ_FILE_ERROR "Error: Reading from file returned the following error: %s\n"
#define CLOSE_FILE_ERROR "Error: Could not close file due to the following error %s\n"
#define WRITE_SOCKET_ERROR "Error: Failed writing data to server due to the following error %s\n"
#define READ_SOCKET_ERROR "Error: Failed to read answer from server due to the following error %s\n"
#define CLOSE_SOCKET_ERROR "Error: Could not close the socket at the end of program due to the following error %s\n"

#endif //OS_COURSE_PCC_CLIENT_H
