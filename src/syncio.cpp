#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <iostream>
#include <unistd.h>

using namespace std;

static inline double gettime(void) {
  struct timeval now_tv;
  gettimeofday (&now_tv, NULL);
  return ((double)now_tv.tv_sec) + ((double)now_tv.tv_usec)/1000000.0;
}

void sequentialRead(int fd) {
    char buffer[1024*16];
    double start = gettime();
    while (gettime() - start < 1) {
        size_t rsize = read(fd, buffer, 1024*16);
        cout << rsize << std::endl;
    }
}

int main(int argc, char const *argv[])
{
    const char* filename = argv[1];
    int fd;

    fd = open(filename, O_RDWR | O_CREAT | O_DIRECT, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        perror("Open error");
        return -1;
    }
    sequentialRead(fd);
    return 0;
}
