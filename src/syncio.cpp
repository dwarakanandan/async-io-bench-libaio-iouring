#include <stdio.h>
#include <stdlib.h>
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

void sequentialRead(int fd, int chunk_size) {
    char* buffer = (char *) aligned_alloc(1024, 1024 * chunk_size);
    double start = gettime();
    uint64_t ops = 0;
    while (gettime() - start < 10) {
        read(fd, buffer, 1024 * chunk_size);
        ops++;
    }
    cout << ((ops * 1024 * chunk_size)/(1024.0*1024*1024*10)) << " GB/s" << endl;
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
    sequentialRead(fd, 16);
    return 0;
}
