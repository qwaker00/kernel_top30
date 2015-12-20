#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <stdio.h>
#include <memory.h>
#include <stdlib.h>

int min(int x, int y) {
    if (x < y)
        return x;
    return y;
}

int main(int argc, char** argv) {
    char buf[8192];
    int fd = open("/dev/top30", O_RDONLY);
    size_t offset = 0, to_read = 0, done = 0;
    while (1) {
        to_read = min(sizeof(buf) - offset, rand() % 20 + 5);
        done = read(fd, buf + offset, to_read);
        if (!done) {
            break;
        }
        offset += done;
    }
    fwrite(buf, 1, offset, stdout);
    return 0;
}
