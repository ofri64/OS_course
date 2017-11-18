//
// Created by okleinfeld on 11/18/17.
//

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

int concatArgsToSting(int argc, char **argv, char **strPtr);

int main(int argc, char** argv){
    char* string;
    concatArgsToSting(argc, argv, &string);
    for (int i = 0; i < 4; i++){
        printf("Your message is: %s\n", string);
        sleep(10);
    }
}

int concatArgsToSting(int argc, char **argv, char **strPtr){
    size_t totalLen = 0;
    for (int i = 1; i < argc; i++){
        totalLen += strlen(argv[i]);
        if (i != argc-1){
            totalLen += 1; // for space char
        }
    }

    totalLen++; // for '\0' at the end

    char* str = (char*) malloc(totalLen * sizeof(char));

    char* originalPtr = str;

    for (int i = 1; i < argc; i++){
        long currentArgLen = strlen(argv[i]);

        for (int j = 0; j < currentArgLen; j++){
            *str = argv[i][j];
            str++;
        }

        if (i != argc-1) {
            *str = ' ';
            str++;
        }
    }
    *str = '\0';

    *strPtr = originalPtr;
    return 1;
}
