//
// Created by okleinfeld on 11/18/17.
//

#include <unistd.h>
#include <stdio.h>

int main(int argc, char** argv){
    write(1, "hello world\n", 12);
    close(1);
}
