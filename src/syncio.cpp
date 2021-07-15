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
#define RUNTIME 1

static inline double gettime(void) {
  struct timeval now_tv;
  gettimeofday (&now_tv, NULL);
  return ((double)now_tv.tv_sec) + ((double)now_tv.tv_usec)/1000000.0;
}

void sequentialRead(int fd, int chunk_size) {
    char* buffer = (char *) aligned_alloc(1024, 1024 * chunk_size);
    double start = gettime();
    uint64_t ops = 0;
    while (gettime() - start < RUNTIME) {
        ssize_t rbytes = read(fd, buffer, 1024 * chunk_size);
        if (rbytes == 0) {
            cout << "Reached eof" << endl;
        }
        ops++;
    }
    cout << "Chunk size: " << chunk_size << endl;
    cout << "Number of ops: " << ops << endl;
    cout << "Throughput: " << ((ops * 1024 * chunk_size)/(1024.0*1024*1024 * RUNTIME)) << " GB/s" << endl << endl;
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
    sequentialRead(fd, 2);
    sequentialRead(fd, 4);
    sequentialRead(fd, 8);
    sequentialRead(fd, 16);
    sequentialRead(fd, 32);
    sequentialRead(fd, 64);
    return 0;
}
