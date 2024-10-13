#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#define LEDOFF 0
#define LEDON  1


int main(int argc, char *argv[]){
    int fd = 0, ret = 0;
    char *filename;
    unsigned char databuf[256] = {0};
    if (argc > 2){
        printf("Error Usage!\r\n");
        return -1;
    }

    filename = argv[1];

    fd = open(filename, O_RDWR);
    if (fd < 0){
        printf("mpu open failed\r\n");
        return -1;
    }

    ret = read(fd, databuf, sizeof(databuf));
    if (ret < 0){
        printf("mpu read failed\r\n");
        close(fd);
        return -1;
    }
    
    printf("mpu: %s \r\n", databuf);


    ret = close(fd);
    if (ret < 0){
        printf("mpu close failed\r\n");
        return -1;
    }

    return 0;
}