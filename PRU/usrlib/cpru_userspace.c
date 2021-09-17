#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/poll.h>
#include <stdint.h>

#define DEVICE_NAME "/dev/rpmsg_pru30"
#define MAX_BUFFER_SIZE 512

char readBuf[MAX_BUFFER_SIZE];

int8_t count_pru(uint32_t time_base, uint32_t *data) {
    struct pollfd pfd;
    int offset = 0;

    pfd.fd = open(DEVICE_NAME, O_RDWR);

    if (pfd.fd < 0)
    {
        printf("Failed to open %s: %d %d\n", DEVICE_NAME, sfd.fd, pfd.fd);
        return -1;
    }

    write(pfd.fd, "-", 2);
    usleep(time_base-400); // There is a 400 us offset
    write(pfd.fd, "-", 2);

    if (read(pfd.fd, readBuf, MAX_BUFFER_SIZE))
    {
        for (int i = 0; i < 4; i++) {
            data[i] = (readBuf[offset+3] << 24) | (readBuf[offset+2] << 16) | (readBuf[offset+1] << 8) | readBuf[offset];
            offset+=4;
        }
    }

    read(pfd.fd, readBuf, MAX_BUFFER_SIZE);
    close(pfd.fd);

    return 0;
}

