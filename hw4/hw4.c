//
// Created by okleinfeld on 12/22/17.
//

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>

#define CHUNK_SIZE 1048576
#define CREATE_PERMISSIONS 0777
#define WELCOME_MESSAGE "Hello, creating %s from %d input files\n"
#define TERMINATION_MESSAGE "Created %s with size %d bytes\n"
#define NUM_ARGS_ERROR "Error: You must supply at least 2 arguments - one output file and at least 1 input file\n"
#define MEMORY_ERROR "Error: Memory allocation failed\n"
#define OPEN_ERROR "Error: Cannot open file from path %s\n"
#define READ_ERROR "Error: Cannot read from file %s\n"
#define WRITE_ERROR "Error: Could not write to output file\n"
#define CREATE_THREAD_ERROR "Error: Could not create thread for input file %d with the following error %s\n"
#define LOCK_ERROR "Error: An error has occurred trying to acquire shared buffer lock: %s\n"
#define UNLOCK_ERROR "Error: An error has occurred trying to unlock shared buffer lock: %s\n"
#define JOIN_ERROR "Error: Could not join thread, the error is %s\n"

void* xorChuckInputFile(void*);
void resetBuffer(char *buffer, int bufferSize);
void updateSharedInfoOfFiles(bool fileEnded, ssize_t fileBytesRead);
void writeToOutputBuffer();
void updateAndResetSharedVarsForChunk();


int outputFd;
int numInputFiles;
char sharedBuffer[CHUNK_SIZE] = {0};
ssize_t maxBytesReadForCurrentChunk = -1;
int numFileFinishedChunk = 0;
int numFilesEnded = 0;
int totalBytesWritten = 0;
pthread_mutex_t outBufferLock;
pthread_mutex_t filesProgressLock;
pthread_cond_t  endChunkCondVar;

int main (int argc, char *argv[]) {
    if (argc < 3) {
        printf(NUM_ARGS_ERROR);
        exit(-1);
    }

    // Welcome Message and initialization of locks and cv

    char *outputFile = argv[1];
    numInputFiles = argc - 2;
    int errorStatus;
    printf(WELCOME_MESSAGE, outputFile, numInputFiles);

    pthread_mutex_init(&outBufferLock, NULL);
    pthread_mutex_init(&filesProgressLock, NULL);
    pthread_cond_init(&endChunkCondVar, NULL);

    // Initialize memory for threads object

    pthread_t* threads = (pthread_t*) calloc((size_t )numInputFiles, sizeof(pthread_t));
    if (threads == NULL){
        printf(MEMORY_ERROR);
        perror(strerror(errno));
        exit(-1);
    }

    // Open the output file - create or truncate if exists

    outputFd = open(outputFile, O_CREAT | O_WRONLY | O_TRUNC, CREATE_PERMISSIONS);
    if (outputFd < 0) {
        printf(OPEN_ERROR, outputFile);
        perror(strerror(errno));
        exit(-1);
    }

    // Lunch a thread for each input file

    for (int i = 0; i < numInputFiles; ++i) {
        errorStatus = pthread_create(&threads[i], NULL, xorChuckInputFile, (void *) argv[i+2]);
        if(errorStatus) {
            printf(CREATE_THREAD_ERROR, i, strerror(errorStatus));
            exit( -1 );
        }
    }

    // Wait for threads to finish
    for( int i = 0; i < numInputFiles; ++i) {
        pthread_t threadNum = threads[i];
        errorStatus = pthread_join(threadNum, NULL);
        if (errorStatus){
            printf(JOIN_ERROR, strerror(errorStatus));
        }
    }

    // Print termination message
    printf(TERMINATION_MESSAGE, outputFile, totalBytesWritten);

    // clean up and exit
    close(outputFd);
    free(threads);
    pthread_mutex_destroy(&outBufferLock);
    pthread_mutex_destroy(&filesProgressLock);
    pthread_cond_destroy(&endChunkCondVar);
    pthread_exit(NULL);
}

void* xorChuckInputFile(void* inputFilePath){
    char* inputFile = (char *) inputFilePath;
    int errorStatus;
    size_t numBytesReadDuringIteration = 0;
    bool fileEnded = false;

    // open file only for reading and initialize local input buffer
    int fd = open(inputFile, O_RDONLY);
    if (fd < 0) {
        printf(OPEN_ERROR, inputFile);
        exit(-1);
    }
    char inputBuffer[CHUNK_SIZE];

    while (!fileEnded) {

        // read chunk size (or less) data from the file
        while (numBytesReadDuringIteration < CHUNK_SIZE){
            ssize_t bytesRead = read(fd, inputBuffer + numBytesReadDuringIteration, CHUNK_SIZE - numBytesReadDuringIteration);
            if (bytesRead < 0) {
                printf(READ_ERROR, inputFile);
                exit(-1);
            }
            if (bytesRead == 0){ // reached end of file
                break;
            }
            numBytesReadDuringIteration += bytesRead;
        }

        // Acquire lock before updating the shared output buffer data
        errorStatus = pthread_mutex_lock(&outBufferLock);
        if (errorStatus) {
            printf(LOCK_ERROR, strerror(errorStatus));
            exit(-1);
        }

        for (int i = 0; i < numBytesReadDuringIteration; ++i) {
            sharedBuffer[i] = sharedBuffer[i] ^ inputBuffer[i];
        }

        errorStatus = pthread_mutex_unlock(&outBufferLock);
        if (errorStatus) {
            printf(UNLOCK_ERROR, strerror(errorStatus));
            exit(-1);
        }

        // reset thread input buffer and conclude if thread file has ended
        fileEnded = numBytesReadDuringIteration < CHUNK_SIZE;
        resetBuffer(inputBuffer, CHUNK_SIZE);
        size_t bytesReadFromSpecificFile = numBytesReadDuringIteration;
        numBytesReadDuringIteration = 0;

        // Acquire lock before updating shared info about files
        errorStatus = pthread_mutex_lock(&filesProgressLock);
        if (errorStatus) {
            printf(LOCK_ERROR, strerror(errorStatus));
            exit(-1);
        }

        updateSharedInfoOfFiles(fileEnded, bytesReadFromSpecificFile);

        // If you are not the last thread updating the shared output buffer - sleep and wait for the chunk to end
        if (numFileFinishedChunk + numFilesEnded != numInputFiles) {
            pthread_cond_wait(&endChunkCondVar, &filesProgressLock);
        }

        else{
            // If you are the last thread to finish - write from output buffer to output file,
            // also reset shared buffer and variables and finally release lock and signal event
            writeToOutputBuffer();
            updateAndResetSharedVarsForChunk();
            pthread_cond_broadcast(&endChunkCondVar);
        }

        errorStatus = pthread_mutex_unlock(&filesProgressLock);
        if (errorStatus) {
            printf(UNLOCK_ERROR, strerror(errorStatus));
            exit(-1);
        }
    }

    close(fd);
    pthread_exit(NULL);
}


void resetBuffer(char *buffer, int bufferSize){
    for (int i = 0; i < bufferSize; i++){
        buffer[i] = 0;
    }
}

void updateSharedInfoOfFiles(bool fileEnded, ssize_t fileBytesRead){
    if (fileBytesRead > maxBytesReadForCurrentChunk){
        maxBytesReadForCurrentChunk = fileBytesRead;
    }
    if (fileEnded){
        numFilesEnded++;
    } else{
        numFileFinishedChunk++;
    }
}

void writeToOutputBuffer(){
    size_t totalBytesWrittenInCurrentIteration = 0;
    while (totalBytesWrittenInCurrentIteration < maxBytesReadForCurrentChunk){
        ssize_t byteWritten = write(outputFd, sharedBuffer + totalBytesWrittenInCurrentIteration, (size_t ) maxBytesReadForCurrentChunk - totalBytesWrittenInCurrentIteration);
        if (byteWritten < 0){
            printf(WRITE_ERROR);
            perror(strerror(errno));
            exit(-1);
        }
        totalBytesWrittenInCurrentIteration += byteWritten;
    }
}

void updateAndResetSharedVarsForChunk(){
    resetBuffer(sharedBuffer, CHUNK_SIZE);
    totalBytesWritten += maxBytesReadForCurrentChunk;
    maxBytesReadForCurrentChunk = -1;
    numFileFinishedChunk = 0;
}