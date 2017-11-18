//
// Created by okleinfeld on 11/18/17.
//

#include <stdio.h>
#include <unistd.h>

int main(int argc, char** argv){
    if (argc != 2){
        return -1;
    }
    for (int i = 0; i < 10; i++){
        printf("Your message is %s\n", argv[1]);
        sleep(2);
    }

}
