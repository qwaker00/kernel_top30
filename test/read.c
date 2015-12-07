#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <stdio.h>
#include <memory.h>
int main(int argc, char** argv) {
    char buf[8192];
    int to_read = sizeof(buf);
    if (argc > 1) {
        to_read = atoi(argv[1]);
    }
    int fd = open("/dev/top30", O_RDONLY);
    size_t done = read(fd, buf, to_read), i = 0;
    fwrite(buf, 1, done, stdout);
    return 0;
}
