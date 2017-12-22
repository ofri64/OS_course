//
// Created by okleinfeld on 12/22/17.
//

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define CHUNK_SIZE 1024
#define CREATE_PERMISSIONS 0666
#define NUM_ARGS_ERROR "Error: You must supply at least 2 arguments - one output file and at least 1 input file\n"
#define MEMORY_ERROR "Error: Mermory allocation faild\n"
#define OPEN_ERROR "Error: Cannot open file from path %s\n"
#define READ_ERROR "Error: Cannot read from file %s\n"

ssize_t xorChuckInputFile(char* inputFile, char* sharedBuffer, int offset);
ssize_t getMaxSize(const ssize_t* sizesArray, ssize_t arrayLength);
void resetBuffer(char *buffer, int bufferSize);

int main (int argc, char *argv[]) {
    if (argc < 3) {
        printf(NUM_ARGS_ERROR);
        exit(-1);
    }

    char* outputFile = argv[1];
    int numInputFiles = argc-2;
    char sharedBuffer[CHUNK_SIZE] = {0};

    ssize_t * bytesReadForChunk = (ssize_t *) calloc((size_t ) numInputFiles, sizeof(ssize_t));
    if (bytesReadForChunk == NULL){
        printf(MEMORY_ERROR);
        exit(-1);
    }

    int outputFd = open(outputFile, O_CREAT | O_WRONLY | O_TRUNC, CREATE_PERMISSIONS);
    if (outputFd < 0){
        printf(OPEN_ERROR, outputFile);
        perror(strerror(errno));
        exit(-1);
    }

    ssize_t maxBytesReadFromFiles;
    int outputByteLocation = 0;

    do {
        for (int i = 0; i < numInputFiles; i++){
            bytesReadForChunk[i] = xorChuckInputFile(argv[i+2], sharedBuffer, outputByteLocation);
        }

        maxBytesReadFromFiles = getMaxSize(bytesReadForChunk, numInputFiles);
        ssize_t byteWritten = write(outputFd, sharedBuffer, (size_t ) maxBytesReadFromFiles);
        outputByteLocation += CHUNK_SIZE;
        resetBuffer(sharedBuffer, CHUNK_SIZE);
    }
    while (maxBytesReadFromFiles == CHUNK_SIZE);

    close(outputFd);
    free(bytesReadForChunk);
}

ssize_t xorChuckInputFile(char* inputFile, char* sharedBuffer, int offset){
    int fd = open(inputFile, O_RDONLY);
    lseek(fd, offset, SEEK_SET);

    if (fd < 0){
        printf(OPEN_ERROR, inputFile);
        exit(-1);
    }

    char inputBuffer[CHUNK_SIZE];
    ssize_t bytesRead;
    bytesRead = read(fd, inputBuffer, CHUNK_SIZE);
    if (bytesRead < 0){
        printf(READ_ERROR, inputFile);
        exit(-1);
    }

    close(fd);

    for (int i = 0; i < bytesRead; i++){
        sharedBuffer[i] = sharedBuffer[i] ^ inputBuffer[i];
    }


    return bytesRead;
}

ssize_t getMaxSize(const ssize_t* sizesArray, ssize_t arrayLength){
    ssize_t max = sizesArray[0];
    for (int i = 1; i < arrayLength; i++){
        ssize_t currentSize = sizesArray[i];
        if (currentSize > max){
            max = currentSize;
        }
    }

    return max;
}

void resetBuffer(char *buffer, int bufferSize){
    for (int i = 0; i < bufferSize; i++){
        buffer[i] = 0;
    }
}
