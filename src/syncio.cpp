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
#include <future>

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
    int runtime;
};

double sequentialRead(const RuntimeArgs_t& args) {
    char* buffer = (char *) aligned_alloc(1024, 1024 * args.blk_size);
    double start = gettime();
    uint64_t ops = 0;
    while (gettime() - start < args.runtime) {
        read(args.fd, buffer, 1024 * args.blk_size);
        ops++;
    }
    double throughput = ((ops * 1024 * args.blk_size)/(1024.0*1024*1024 * args.runtime));

    std::stringstream stats;
    stats << "TID:" << args.thread_id
        << " bsize: " << args.blk_size << "kB"
        << " ops: " << throughput << " GB/s" << endl;
    cout << stats.str();
    return throughput;
}

int main(int argc, char const *argv[])
{
    if (argc < 3 ) {
        cout << "Usage: syncio -f <file> -t <threads> -rt <runtime> -blk <block_size_kB>" << endl;
        exit(1);
    }
    const char* filename;
    int blockSize = 16;
    int threadCount = 1;
    int runtime = 1;
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-f") == 0) {filename = argv[i+1];}
        if (strcmp(argv[i], "-blk") == 0) {blockSize = atoi(argv[i+1]);}
        if (strcmp(argv[i], "-t") == 0) {threadCount = atoi(argv[i+1]);}
        if (strcmp(argv[i], "-rt") == 0) {runtime = atoi(argv[i+1]);}
    }

    int fd;

    fd = open(filename, O_RDWR | O_CREAT | O_DIRECT, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        perror("Open error");
        return -1;
    }

    std::vector<std::future<double>> threads;
    for (int i = 0; i < threadCount; ++i) {
        RuntimeArgs_t args;
        args.thread_id = i;
        args.blk_size = blockSize;
        args.fd = fd;
        args.runtime = runtime;
        threads.push_back(std::async(sequentialRead, args));
    }

    double aggregateStats = 0;
    for (auto& t : threads) {
        aggregateStats += t.get();
    }

    cout << "Total throughput = " << aggregateStats << " GB/s" << endl;
    return 0;
}
