#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <stdio.h>
#include <memory.h>
int main(int argc, char** argv) {
    int fd = open("/dev/top30", O_WRONLY);
    write(fd, argv[1], strlen(argv[1]));
    return 0;
}
