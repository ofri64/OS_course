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

    ret_val = write( file_desc, "Hello", 5);
    ret_val = read(  file_desc, NULL, 100 );

    close(file_desc);
    return 0;
}
