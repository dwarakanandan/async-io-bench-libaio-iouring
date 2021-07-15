#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <iostream>
#include <unistd.h>
#include <thread>
#include <vector>
#include <sstream>

using namespace std;
#define RUNTIME 1

static inline double gettime(void) {
  struct timeval now_tv;
  gettimeofday (&now_tv, NULL);
  return ((double)now_tv.tv_sec) + ((double)now_tv.tv_usec)/1000000.0;
}

struct RuntimeArgs_t {
    int thread_id;
    int fd;
    int blk_size;
};

void sequentialRead(const RuntimeArgs_t& args) {
    char* buffer = (char *) aligned_alloc(1024, 1024 * args.blk_size);
    double start = gettime();
    uint64_t ops = 0;
    while (gettime() - start < RUNTIME) {
        read(args.fd, buffer, 1024 * args.blk_size);
        ops++;
    }
    double throughput = ((ops * 1024 * args.blk_size)/(1024.0*1024*1024 * RUNTIME));

    std::stringstream stats;
    stats << "TID:" << args.thread_id
        << " bsize: " << args.blk_size << "kB"
        << " ops: " << throughput << " GB/s" << endl;
    cout << stats.str();
}

int main(int argc, char const *argv[])
{
    if (argc <3 ) {
        cout << "Usage: syncio <file_name> <block_size_kB> <thread_count>" << endl;
        exit(1);
    }
    const char* filename = argv[1];
    const int blockSize = atoi(argv[2]);
    const int threadCount = atoi(argv[3]);

    int fd;

    fd = open(filename, O_RDWR | O_CREAT | O_DIRECT, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        perror("Open error");
        return -1;
    }

    std::vector<std::thread> threads;
    for (int i = 0; i < threadCount; ++i) {
        RuntimeArgs_t args;
        args.thread_id = i;
        args.blk_size = blockSize;
        args.fd = fd;
        threads.push_back(std::thread(sequentialRead, args));
    }

    for (auto& t : threads) {
        t.join();
    }
    return 0;
}
