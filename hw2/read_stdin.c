//
// Created by okleinfeld on 11/19/17.
//

#include <unistd.h>
#include <stdio.h>

int main(){
    char data[512];
    long status;
    while ((status = read(0, data, 512)) <= 0){
        printf("couldn't read");
    }
    data[status] = '\0';
    printf("You directed me with: %s\n", data);
}
