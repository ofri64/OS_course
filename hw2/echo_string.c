//
// Created by okleinfeld on 11/18/17.
//

#include <stdio.h>

int main(int argc, char** argv){
    if (argc != 2){
        return -1;
    }
    printf("%s", argv[1]);
}
