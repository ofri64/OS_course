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
#define WRONG_ARGS "The program must have exactly 3 arguments: message slot file path, target channel id and message to write\n"


int main(int argc, char** argv){
    if (argc != 4){
        printf (WRONG_ARGS);
    }

    char* deviceFile = argv[1];
    int channelId = atoi(argv[2]);
    char *theMessage = argv[3];
    size_t messageLen = strlen(theMessage);

    int file_desc = open(deviceFile, O_RDWR);
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

    long writeReturn = write( file_desc,theMessage , messageLen);
    if (writeReturn < 0){
        printf("Can't write device file: %s\n", deviceFile);
        fprintf(stderr, "file error is %s\n", strerror(errno));
        return -1;
    }

    close(file_desc);
    printf("Successfully wrote to device %s, the value %s\n", deviceFile, theMessage);

    return 0;
}
