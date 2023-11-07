#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <linux/input.h>

#define CLOSE_CMD		(_IO(0xEF, 0x1))
#define OPEN_CMD		(_IO(0xEF, 0x2))
#define SETPERIOD_CMD	(_IO(0xEF, 0x3))

int main(int argc, char** argv)
{
    if (argc < 1)
    {
        printf("ERROR: argc\n");
        return -1;
    }

    int fd = open("/dev/timery", O_RDWR);
    if (fd < 0)
    {
        printf("ERROR: open\n");
        return -1;
    }

    int ret = 0;
    unsigned int cmd;
    unsigned char str[100];
    unsigned int arg;
    while (1)
    {
        printf("Input CMD = ");
        ret = scanf("%d", &cmd);
        if (ret != 1)
        {
            gets(str);
        }

        switch (cmd)
        {
            case 1:
            {
                printf("CMD is CLOSE\n");
                cmd = CLOSE_CMD;
                break;
            }
            case 2:
            {
                printf("CMD is OPEN\n");
                cmd = OPEN_CMD;
                break;
            }
            case 3:
            {
                printf("CMD is SETPERIOD\n");
                cmd = SETPERIOD_CMD;
                printf("Input Timer Period = ");
                ret = scanf("%d", &arg);
                if (ret != 1)
                {
                    gets(str);
                }   
                break;
            }
            default: printf("CMD is invalid\n"); break;
        }
        
        ioctl(fd, cmd, arg);   
    }
    
    close(fd);
    
    return 0;
}
