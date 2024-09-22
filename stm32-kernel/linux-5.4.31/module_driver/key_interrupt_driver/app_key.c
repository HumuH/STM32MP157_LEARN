#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>


#define BEEPOFF 1
#define BEEPON  0


int main(int argc, char *argv[]){
    int fd = 0, ret = 0;
    int status = 2;
    char *filename;
    unsigned char databuf[1] = {0};
    int last_status = 2;

    filename = argv[1];

    fd = open(filename, O_RDONLY);
    if (fd < 0){
        printf("button open failed\r\n");
        return -1;
    }

    


    for (;;){
        ret = read(fd, databuf, sizeof(databuf));
        if (ret < 0){
            printf("button read failed\r\n");
            close(fd);
            return -1;
        }

        status = databuf[0];
        if (status != last_status){
            printf("button status:%d\n", databuf[0]);
            last_status = status;
        }        
    }

    ret = close(fd);
    if (ret < 0){
        printf("beep close failed\r\n");
        return -1;
    }

    return 0;
}