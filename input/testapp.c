#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <linux/input.h>

static struct input_event inputevent;

int main(int argc, char** argv)
{
    if (argc < 1)
    {
        printf("ERROR: argc\n");
        return -1;
    }

    int fd = open("/dev/input/event1", O_RDWR);
    if (fd < 0)
    {
        printf("ERROR: open\n");
        return -1;
    }

    int err = 0;
    while (1)
    {
        err = read(fd, &inputevent, sizeof(inputevent));
        if (err < 0)
        {
            printf("ERROR: read\n");
            continue;
        }
        else
        {
            printf("SUCCESS: read\n");
        }
        switch (inputevent.type)
        {
            case EV_KEY:
            {
                printf("inputevent.code is EV_KEY\n");
                if (inputevent.code < BTN_MISC) 
                {
                    printf("key %d %s\r\n", inputevent.code, inputevent.value ? "press" : "release");
                }
                else
                {
                    printf("button %d %s\r\n", inputevent.code, inputevent.value ? "press" : "release");
                }
            }
            
            default:
            {
                printf("inputevent.code is invalid\n");
                break;
            }
        }
    }
    

    close(fd);
    
    return 0;
}
