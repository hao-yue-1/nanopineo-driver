#include <stdio.h>
#include <fcntl.h>
#include <string.h>

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        printf("ERROR: argc\n");
        return -1;
    }

    int fd = open("/dev/hello", O_RDWR);
    if (fd < 0)
    {
        printf("ERROR: open\n");
        return -1;
    }

    int buf[256] = {0};
    int ret = 0;

    switch(argv[1][0])
    {
        case 'w':
        {
            ret = write(fd, argv[2], strlen(argv[2]));
            if (ret < 0)
            {
                printf("ERROR: write\n");
                close(fd);
                return -1;
            }
            printf("SUCCESS: write is %s\n", argv[2]);
            break;
        }
        case 'r':
        {
            ret = read(fd, buf, 256);
            if (ret < 0)
            {
                printf("ERROR: read\n");
                close(fd);
                return -1;
            }
            printf("SUCCESS: read is %s\n", buf);
            break;
        }
        default : printf("ERROR: argv\n");
    }

    close(fd);
    
    return 0;
}