#include <fcntl.h>      /* open */
#include <unistd.h>     /* exit */
#include <sys/ioctl.h>  /* ioctl */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

int main()
{
    int file_desc;
    long ret_val;

    file_desc = open( "/dev/simple_char_dev", O_RDWR);
    if( file_desc < 0 )
    {
        printf ("Can't open device file: %s\n", "simple_char_dev");
        fprintf(stderr, "file error is%s\n", strerror(errno));
        exit(-1);
    }

    printf("file descriptor num is %d\n", file_desc);
    char buff[256];
    ret_val = write( file_desc, "Hello", 5);
    if (ret_val < 0){
        printf("Can't write device file: %s\n", "simple_char_dev");
        fprintf(stderr, "file error is%s\n", strerror(errno));
    }
    ret_val = read( file_desc, buff, 5 );
    if (ret_val < 0){
        printf("Can't read device file: %s\n", "simple_char_dev");
        fprintf(stderr, "file error is%s\n", strerror(errno));
    }
    else{
        printf("read into buffer the string %s\n", buff);
    }


    close(file_desc);
    return 0;
}
