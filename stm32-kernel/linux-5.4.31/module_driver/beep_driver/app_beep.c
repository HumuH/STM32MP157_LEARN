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
    char *filename;
    unsigned char databuf[1] = {0};
    if (argc != 3){
        printf("Error Usage!\r\n");
        return -1;
    }

    filename = argv[1];

    fd = open(filename, O_RDWR);
    if (fd < 0){
        printf("beep open failed\r\n");
        return -1;
    }

    databuf[0] = atoi(argv[2]);
    ret = write(fd, databuf, sizeof(databuf));

    if (ret < 0){
        printf("beep write failed\r\n");
        close(fd);
        return -1;
    }

    ret = close(fd);
    if (ret < 0){
        printf("beep close failed\r\n");
        return -1;
    }

    return 0;
}