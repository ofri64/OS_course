//
// Created by okleinfeld on 11/19/17.
//

#include <unistd.h>
#include <stdio.h>

int main(){
    char data[512];
    long status = read(0, data, 521);
    if (status < 0){
        write(2, "Error\n", 6);
    }
    else{
        printf("You directed me with: %s", data);
    }
}
