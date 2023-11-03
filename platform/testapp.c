#include <stdio.h>
#include <fcntl.h>
#include <string.h>

int main(int argc, char** argv)
{
    if (argc < 1)
    {
        printf("ERROR: argc\n");
        return -1;
    }

    int fd = open("/dev/ledy", O_RDWR);
    if (fd < 0)
    {
        printf("ERROR: open\n");
        return -1;
    }

    unsigned char databuf[1];
    int ret = 0;

    databuf[0] = atoi(argv[1]);
    ret = write(fd, databuf, sizeof(databuf));
    if (fd < 0)
    {
        printf("ERROR: write\n");
        return -1;
    }

    close(fd);
    
    return 0;
}