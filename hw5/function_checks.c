//
// Created by okleinfeld on 1/7/18.
//

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>

#define PRINTABLE_OFFSET 32

int updateSharedPcc(unsigned N, const char* message, int* sharedPcc);
bool isPrintableCharacter(char c);

int main(int argc, char *argv[]) {
    char message[] = {'1', '2', 'a', 'b', 'c'};
    int pcc[95] = {};
    updateSharedPcc(5, message, pcc);
    for (int i = 0; i < 95; i++){
        int c = i + PRINTABLE_OFFSET;
        printf("char '%c': %u times\n", c, pcc[i]);
    }
}

bool isPrintableCharacter(char c){
    if (c >= 32 && c <= 126)
        return true;
    else
        return false;
}


int updateSharedPcc(unsigned N, const char* message, int* sharedPcc){
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