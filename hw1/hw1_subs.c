//#include <stdio.h>
//#include <string.h>
//#include <stdlib.h>
//#include <fcntl.h>
//#include <unistd.h>
//#include <sys/types.h>
//#include <sys/stat.h>
//#include <errno.h>
//
//#define NO_ENVIRONMENT_VARIABLES_ERROR "Error: At least one of the Environment Variables 'HW1DIR', 'HW1TF' does not exist\n"
//#define ARGUMENT_MISMATCH_ERROR "Error: You have to supply two argument variables to the program. But you only supplied %d\n"
//#define MEMORY_ERROR "Error: Memory Allocation Error occurred\n"
//#define FILE_NOT_OPEN_ERROR "Error: Failed to open file %s\n"
//#define CANNOT_READ_FILE_ERROR "Error: Failed to read file\n"
//#define FILE_OPERATION_FAILED "Error: Failed to substitute between words in file content\n"
//#define WRITE_TO_OUTPUT_ERROR "Error: Failed to write to output the new content %s\n"
//
//char* concatDirAndPath(char* dirPath, char* fileName);
//char* replaceStringContent(char* originalStr, char* replaceStr, char* newStr);
//int countNumReplacement(char* originalStr, char* replaceStr);
//int writeStrToOutput(char* string);
//char* readFile(int fileDescriptor);
//
//
//int main(int argc, char** argv) {
//    char* HW1DIR = getenv("HW1DIR");
//    char* HW1TF = getenv("HW1TF");
//    int exit_status =  0;
//
//    if (HW1DIR == NULL || HW1TF == NULL){
//        printf(NO_ENVIRONMENT_VARIABLES_ERROR);
//        exit_status = 1;
//        return exit_status;
//    }
//
//    if (argc != 3){
//        printf(ARGUMENT_MISMATCH_ERROR, argc-1);
//        exit_status = 1;
//        return exit_status;
//    }
//
//    char* filePath = concatDirAndPath(HW1DIR, HW1TF);
//    if (filePath == NULL){
//        printf(MEMORY_ERROR);
//        exit_status = 1;
//        return exit_status;
//    }
//
//
//    int fd = open(filePath, O_RDWR);
//    if (fd < 0){
//        printf(FILE_NOT_OPEN_ERROR, strerror(errno));
//        free(filePath);
//        exit_status = 1;
//        return exit_status;
//    }
//
//    free(filePath);
//
//    char* fileContent = readFile(fd);
//    if (fileContent == NULL){
//        printf(CANNOT_READ_FILE_ERROR);
//        close(fd);
//        exit_status = 1;
//        return exit_status;
//    }
//
//    close(fd);
//
//    char* replaceWord = argv[1];
//    char* replaceWith = argv[2];
//
//    char* newFile = replaceStringContent(fileContent, replaceWord, replaceWith);
//    if (newFile == NULL){
//        free(fileContent);
//        printf(FILE_OPERATION_FAILED);
//        exit_status = 1;
//        return exit_status;
//    }
//    free(fileContent);
//
//    int writeStatus = writeStrToOutput(newFile);
//    if (writeStatus < 0){
//        free(newFile);
//        printf(WRITE_TO_OUTPUT_ERROR, strerror(errno));
//        exit_status = 1;
//        return exit_status;
//    }
//
//    free(newFile);
//    return exit_status;
//}
//
//
//char* concatDirAndPath(char* dirPath, char* fileName){
//    size_t dirPathLength = strlen(dirPath);
//    size_t fileNameLength = strlen(fileName);
//    char* filePath = (char*) calloc(dirPathLength + fileNameLength + 2, sizeof(char));
//    if (filePath == NULL){
//        return NULL;
//    } else{
//        strcat(filePath, dirPath);
//        strcat(filePath, "/");
//        strcat(filePath, fileName);
//        return filePath;
//    }
//}
//
//char* readFile(int fileDescriptor){
//    struct stat buffer;
//    int fileStat = fstat(fileDescriptor, &buffer);
//    if (fileStat < 0){
//        return NULL;
//    }
//    long size = buffer.st_size;
//    if (size <= 0){
//        return NULL;
//    }
//    char* outString = calloc((size_t) size+1, sizeof(char));
//    if (outString == NULL){
//        printf(MEMORY_ERROR);
//        return NULL;
//    }
//    char* currentStringLocation = outString;
//    ssize_t totalBytesRead = 0;
//    ssize_t currentBytesRead;
//    while (totalBytesRead != size){
//        currentBytesRead = read(fileDescriptor, currentStringLocation, (size_t) size - totalBytesRead);
//        if (currentBytesRead < 0){
//            free(outString);
//            printf(MEMORY_ERROR);
//            return NULL;
//        } else{
//            totalBytesRead += currentBytesRead;
//            currentStringLocation += currentBytesRead;
//        }
//    }
//    outString[size] = '\0';
//    return outString;
//}
//
//
//char* replaceStringContent(char* originalStr, char* replaceStr, char* newStr){
//    size_t replaceLength = strlen(replaceStr);
//    size_t newLength = strlen(newStr);
//    int numReplacements = countNumReplacement(originalStr, replaceStr);
//    size_t newFileLength = strlen(originalStr) + (newLength - replaceLength) * numReplacements;
//
//    char* newFile = calloc(newFileLength+1, sizeof(char));
//    char* currentPlaceInNewFile = newFile;
//    if (newFile == NULL){
//        printf(MEMORY_ERROR);
//        return NULL;
//    }
//
//    char* replaceLocation = strstr(originalStr, replaceStr);
//    while (replaceLocation != NULL){ // we still have place to replace the words
//        while (replaceLocation != originalStr){
//            *currentPlaceInNewFile = *originalStr;
//            currentPlaceInNewFile++;
//            originalStr++;
//        }
//        for (int i = 0; i < (int) newLength; i++){
//            *currentPlaceInNewFile = newStr[i];
//            currentPlaceInNewFile++;
//        }
//        originalStr += replaceLength;
//        replaceLocation = strstr(originalStr, replaceStr);
//
//    }
//    while (*originalStr != '\0') { // no more words to replace - just copy the rest of the string
//        *currentPlaceInNewFile = *originalStr;
//        currentPlaceInNewFile++;
//        originalStr++;
//
//    }
//    *currentPlaceInNewFile = '\0';
//    return newFile;
//}
//
//
//
//int countNumReplacement(char* originalStr, char* replaceStr){
//    int counter = 0;
//    char* currentPtr = originalStr;
//    size_t replaceStrLength = strlen(replaceStr);
//    while (1){
//        char* replaceLocation = strstr(currentPtr, replaceStr);
//        if (replaceLocation == NULL){
//            break;
//        }
//        currentPtr = replaceLocation + replaceStrLength;
//        counter++;
//    }
//    return counter;
//}
//
//
//int writeStrToOutput(char* string){
//    if (string == NULL){
//        return -1;
//    }
//    size_t stringLength = strlen(string);
//    size_t totalWrittenBytes = 0;
//    while (totalWrittenBytes < stringLength){
//        ssize_t writeBytes = write(1, string, stringLength - totalWrittenBytes);
//        if (writeBytes < 0){
//            printf(WRITE_TO_OUTPUT_ERROR, strerror(errno));
//            return -1;
//        } else{
//            totalWrittenBytes += writeBytes;
//            string += writeBytes;
//        }
//    }
//    return 0;
//}