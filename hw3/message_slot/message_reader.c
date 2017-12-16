//
// Created by okleinfeld on 12/8/17.
//

#include <fcntl.h>      /* open */
#include <unistd.h>     /* exit */
#include <sys/ioctl.h>  /* ioctl */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "message_slot.h"
#define WRONG_ARGS "The program must have exactly 2 arguments: message slot file path and target message channel id\n"


int main(int argc, char** argv){
    if (argc != 3){
        printf (WRONG_ARGS);
        return;
    }

    char* deviceFile = argv[1];
    unsigned long channelId = (unsigned long) atoi(argv[2]);
    char buffer[BUF_LEN];

    int file_desc = open(deviceFile, O_RDONLY);
    if( file_desc < 0 )
    {
        printf ("Can't open device file: %s\n", deviceFile);
        fprintf(stderr, "file error is %s\n", strerror(errno));
        return -1;
    }

    int icotlReturn = ioctl( file_desc, MSG_SLOT_CHANNEL, channelId);
    if (icotlReturn < 0){
        printf("Can't operate ioctl for device: %s\n", deviceFile);
        fprintf(stderr, "file error is %s\n", strerror(errno));
        return -1;
    }

    long readReturn = read( file_desc, buffer , BUF_LEN);
    if (readReturn < 0){
        printf("Can't read device file: %s\n", deviceFile);
        fprintf(stderr, "file error is %s\n", strerror(errno));
        return -1;
    }

    close(file_desc);
    printf("Successfully read from device %s\n", deviceFile);
    printf("The message read is: %s\n", buffer);

    return 0;
}

