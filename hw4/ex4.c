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

#define CHUNK_SIZE 2
#define CREATE_PERMISSIONS 0666
#define NUM_ARGS_ERROR "Error: You must supply at least 2 arguments - one output file and at least 1 input file\n"
#define MEMORY_ERROR "Error: Memory allocation failed\n"
#define OPEN_ERROR "Error: Cannot open file from path %s\n"
#define READ_ERROR "Error: Cannot read from file %s\n"
#define WRITE_ERROR "Error: Could not write to output file\n"
#define WELCOME_MESSAGE "Hello, creating %s from %d input files\n"
#define CREATE_THREAD_ERROR "Error: Could not create thread for input file %d with the following error %s\n"
#define LOCK_ERROR "Error: An error has occurred trying to acquire shared buffer lock: %s\n"
#define UNLOCK_ERROR "Error: An error has occurred trying to unlock shared buffer lock: %s\n"
#define JOIN_ERROR "Error: Could not join thread, the error is %s\n"

void* xorChuckInputFile(void*);
void resetBuffer(char *buffer, int bufferSize);
void updateSharedInfoOfFiles(bool fileEnded, ssize_t fileBytesRead);
void writeToOutputBuffer();
void resetSharedVarsForChunk();


int outputFd;
int numInputFiles;
char sharedBuffer[CHUNK_SIZE] = {0};
ssize_t maxBytesReadForCurrentChunk = -1;
int numFileFinishedChunk = 0;
int numFilesEnded = 0;
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
        printf("Main: creating thread for input file %d\n", i);
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
    printf ("Main(): Waited on %d threads. Done.\n", numInputFiles);

    // clean up and exit
    close(outputFd);
    free(threads);
    pthread_mutex_destroy(&outBufferLock);
    pthread_mutex_destroy(&filesProgressLock);
    pthread_cond_destroy(&endChunkCondVar);
    pthread_exit(NULL);
}

void* xorChuckInputFile(void* inputFilePath){
    int threadId = (int )pthread_self();
    char* inputFile = (char *) inputFilePath;
    int errorStatus;
    bool fileEnded = false;

    // open file only for reading and initialize local input buffer
    int fd = open(inputFile, O_RDONLY);
    if (fd < 0) {
        printf(OPEN_ERROR, inputFile);
        exit(-1);
    }
    printf("Thread for input file %s with file descriptor %d is starting\n",inputFile, fd);
    char inputBuffer[CHUNK_SIZE];

    while (!fileEnded) {

        // read chunk size (or less) data from the file
        ssize_t bytesRead = read(fd, inputBuffer, CHUNK_SIZE);
        if (bytesRead < 0) {
            printf(READ_ERROR, inputFile);
            exit(-1);
        }

        // Acquire lock before updating the shared output buffer data
        errorStatus = pthread_mutex_lock(&outBufferLock);
        if (errorStatus) {
            printf(LOCK_ERROR, strerror(errorStatus));
            exit(-1);
        }

        for (int i = 0; i < bytesRead; ++i) {
            sharedBuffer[i] = sharedBuffer[i] ^ inputBuffer[i];
        }

        errorStatus = pthread_mutex_unlock(&outBufferLock);
        if (errorStatus) {
            printf(UNLOCK_ERROR, strerror(errorStatus));
            exit(-1);
        }

        printf("Thread %d has finished writing its content to shared buffer\n", fd);

        // reset thread input buffer and conclude if thread file has ended
        resetBuffer(inputBuffer, CHUNK_SIZE);
        fileEnded = bytesRead < CHUNK_SIZE;

        printf("Thread %d reset input buffer and conclude if file ended\n", fd);

        // Acquire lock before updating shared info about files
        printf("Thread %d is tyring to acquire lock for updating files progress\n", fd);
        errorStatus = pthread_mutex_lock(&filesProgressLock);
        if (errorStatus) {
            printf(LOCK_ERROR, strerror(errorStatus));
            exit(-1);
        }

        updateSharedInfoOfFiles(fileEnded, bytesRead);

        // If you are not the last thread updating the shared output buffer - sleep and wait for the chunk to end
        if (numFileFinishedChunk + numFilesEnded < numInputFiles) {
            printf("Thread for file %d, is now meditating and waiting for other threads\n", fd);
            pthread_cond_wait(&endChunkCondVar, &filesProgressLock);
            printf("Signal condition received in thread %d\n", fd);
        }

        // If you are the last thread to finish - write from output buffer to output file,
        // also reset shared buffer and variables and finally release lock and signal event
        if (numFileFinishedChunk == numInputFiles) {
            writeToOutputBuffer();
            resetSharedVarsForChunk();
            printf("Sending end of chunk signal from thread %d\n", fd);
            pthread_cond_broadcast(&endChunkCondVar);
        }
        printf("Thread for file %d is now releasing files progress lock\n", fd);
        errorStatus = pthread_mutex_unlock(&filesProgressLock);
        if (errorStatus) {
            printf(UNLOCK_ERROR, strerror(errorStatus));
            exit(-1);
        }
    }

    printf("Thread %d has finished its file and ended\n", fd);
    close(fd);
    pthread_exit(NULL);
}


void resetBuffer(char *buffer, int bufferSize){
    for (int i = 0; i < bufferSize; i++){
        buffer[i] = 0;
    }
}

void updateSharedInfoOfFiles(bool fileEnded, ssize_t fileBytesRead){
    numFileFinishedChunk++;
    if (fileBytesRead > maxBytesReadForCurrentChunk){
        maxBytesReadForCurrentChunk = fileBytesRead;
    }
    if (fileEnded){
        numFilesEnded++;
    }
}

void writeToOutputBuffer(){
        ssize_t byteWritten = write(outputFd, sharedBuffer, (size_t ) maxBytesReadForCurrentChunk);
        if (byteWritten != maxBytesReadForCurrentChunk){
            printf(WRITE_ERROR);
            perror(strerror(errno));
            exit(-1);
    }
}

void resetSharedVarsForChunk(){
    resetBuffer(sharedBuffer, CHUNK_SIZE);
    maxBytesReadForCurrentChunk = -1;
    numFileFinishedChunk = 0;
}