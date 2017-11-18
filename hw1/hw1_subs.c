#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#define bufferSize 1024
#define NO_ENVIRONMENT_VARIABLES_ERROR "Error: At least one of the Environment Variables 'HW1DIR', 'HW1TF' does not exist\n"
#define ARGUMENT_MISMATCH_ERROR "Error: You have to supply two argument variables to the program. But you only supplied %d\n"
#define MEMORY_ERROR "Error: Memory Allocation Error occurred\n"
#define FILE_NOT_OPEN_ERROR "Error: Failed to open file %s\n"
#define CANNOT_READ_FILE_ERROR "Error: Failed to read file\n"
#define FILE_OPERATION_FAILED "Error: Failed to substitute between words in file content\n"
#define WRITE_TO_OUTPUT_ERROR "Error: Failed to write to output the new content %s\n"

char* concatDirAndPath(char* dirPath, char* fileName);
char* readTextFileToBuffer(int fileDescriptor);
char* replaceStringContent(char* originalStr, char* replaceStr, char* newStr);
int countNumReplacement(char* originalStr, char* replaceStr);

int main(int argc, char** argv) {
    char* HW1DIR = getenv("HW1DIR");
    char* HW1TF = getenv("HW1TF");
    int exit_status =  0;

    if (HW1DIR == NULL || HW1TF == NULL){
        printf(NO_ENVIRONMENT_VARIABLES_ERROR);
        exit_status = 1;
        return exit_status;
    }

    if (argc != 3){
        printf(ARGUMENT_MISMATCH_ERROR, argc-1);
        exit_status = 1;
        return exit_status;
    }

    char* filePath = concatDirAndPath(HW1DIR, HW1TF);
    if (filePath == NULL){
        printf(MEMORY_ERROR);
        exit_status = 1;
        return exit_status;
    }


    int fd = open(filePath, O_RDWR);
    if (fd < 0){
        printf(FILE_NOT_OPEN_ERROR, strerror(errno));
        exit_status = 1;
        return exit_status;
    }

    free(filePath);

    char* fileContent = readTextFileToBuffer(fd);
    if (fileContent == NULL){
        printf(CANNOT_READ_FILE_ERROR);
        close(fd);
        exit_status = 1;
        return exit_status;
    }

    close(fd);

    char* replaceWord = argv[1];
    char* replaceWith = argv[2];

    char* newFile = replaceStringContent(fileContent, replaceWord, replaceWith);
    if (newFile == NULL){
        free(fileContent);
        printf(FILE_OPERATION_FAILED);
        exit_status = 1;
        return exit_status;
    }
    free(fileContent);

    size_t newFileLength = strlen(newFile);
    ssize_t writeBytes = write(1, newFile, newFileLength);
    if (writeBytes != (int) newFileLength){
        free(newFile);
        printf(WRITE_TO_OUTPUT_ERROR, strerror(errno));
        exit_status = 1;
        return exit_status;
    }

    free(newFile);
    return exit_status;
}


char* concatDirAndPath(char* dirPath, char* fileName){
    size_t dirPathLength = strlen(dirPath);
    size_t fileNameLength = strlen(fileName);
    char* filePath = (char*) calloc(dirPathLength + fileNameLength + 2, sizeof(char));
    if (filePath == NULL){
        return NULL;
    }
    strcat(filePath, dirPath);
    strcat(filePath, "/");
    strcat(filePath, fileName);
    return filePath;
}

char* readTextFileToBuffer(int fileDescriptor){
    char* buffer = (char*) calloc(bufferSize+1, sizeof(char));
    if (buffer == NULL){
        printf(MEMORY_ERROR);
        return NULL;
    }
    ssize_t bytesRead = read(fileDescriptor, buffer, bufferSize);
    if (bytesRead < 0){
        free(buffer);
        return NULL;
    }
    if (bytesRead < bufferSize){ // read the entire file
        buffer[bufferSize] = '\0';
        return buffer;
    }
    int bufferCurrentSize = bufferSize;
    int bufferNewSize = bufferCurrentSize * 2;
    int firstRoundIndicator = 1;
    while (bytesRead == bufferCurrentSize){
        if (firstRoundIndicator == 0){
            bufferCurrentSize = bufferNewSize;
            bufferNewSize *= 2;
        }
        char* newBuffer = (char*) calloc((size_t ) bufferNewSize+1, sizeof(char));
        if (newBuffer == NULL){
            printf(MEMORY_ERROR);
            free(buffer);
            return NULL;
        }
        strcat(newBuffer, buffer);
        free(buffer);
        buffer = newBuffer;
        bytesRead = read(fileDescriptor, buffer + bufferCurrentSize, (size_t) bufferCurrentSize);
        if (bytesRead < 0){
            free(buffer);
            return NULL;
        }
        firstRoundIndicator = 0;
    }
    buffer[bufferCurrentSize+bytesRead] = '\0';
    return buffer;
}

char* replaceStringContent(char* originalStr, char* replaceStr, char* newStr){
    size_t replaceLength = strlen(replaceStr);
    size_t newLength = strlen(newStr);
    int numReplacements = countNumReplacement(originalStr, replaceStr);
    size_t newFileLength = strlen(originalStr) + (newLength - replaceLength) * numReplacements;

    char* newFile = calloc(newFileLength+1, sizeof(char));
    char* currentPlaceInNewFile = newFile;
    if (newFile == NULL){
        printf(MEMORY_ERROR);
        return NULL;
    }

    char* replaceLocation = strstr(originalStr, replaceStr);
    while (replaceLocation != NULL){ // we still have place to replace the words
        while (replaceLocation != originalStr){
            *currentPlaceInNewFile = *originalStr;
            currentPlaceInNewFile++;
            originalStr++;
        }
        for (int i = 0; i < (int) newLength; i++){
            *currentPlaceInNewFile = newStr[i];
            currentPlaceInNewFile++;
        }
        originalStr += replaceLength;
        replaceLocation = strstr(originalStr, replaceStr);

    }
    while (*originalStr != '\0') { // no more words to replace - just copy the rest of the string
        *currentPlaceInNewFile = *originalStr;
        currentPlaceInNewFile++;
        originalStr++;

    }
    *currentPlaceInNewFile = '\0';
    return newFile;
}



int countNumReplacement(char* originalStr, char* replaceStr){
    int counter = 0;
    char* currentPtr = originalStr;
    size_t replaceStrLength = strlen(replaceStr);
    while (1){
        char* replaceLocation = strstr(currentPtr, replaceStr);
        if (replaceLocation == NULL){
            break;
        }
        currentPtr = replaceLocation + replaceStrLength;
        counter++;
    }
    return counter;
}
