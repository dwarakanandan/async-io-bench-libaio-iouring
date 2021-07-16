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
#define _1GB (1024*1024*1024*1L)
#define _100GB (1024*1024*1024*100L)

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
    bool debug;
};

double syncioSequentialRead(const RuntimeArgs_t& args) {
    off_t initialOffset = _100GB * args.thread_id;

    std::stringstream rstats;
    rstats << "TID:" << args.thread_id
        << " read_offset: " << initialOffset / _1GB << " GB" << endl;
    if (args.debug) cout << rstats.str();

    size_t page_size  = 1024 * args.blk_size;
    char* buffer = (char *) aligned_alloc(1024, page_size);
    double start = gettime();
    uint64_t ops = 0;
    while (gettime() - start < args.runtime) {
        off_t readOffset = initialOffset + (ops * page_size) % _100GB;
        ssize_t readCount = pread(args.fd, buffer, page_size, readOffset);
        if (readCount <= 0) {
            perror("Read error");
            return -1;
        }
        ops++;
    }
    double throughput = ((ops * 1024 * args.blk_size)/(1024.0*1024*1024 * args.runtime));

    std::stringstream tstats;
    tstats << "TID:" << args.thread_id
        << " ops: " << throughput << " GB/s" << endl;
    if (args.debug) cout << tstats.str();
    return throughput;
}

int main(int argc, char const *argv[])
{
    if (argc < 3 ) {
        cout << "Usage: syncio -f <file> -t <threads> -rt <runtime> -blk <block_size_kB> -d(enables debuginfo)" << endl;
        exit(1);
    }
    const char* filename;
    int blockSize = 16;
    int threadCount = 1;
    int runtime = 1;
    bool debug = false;
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-f") == 0) {filename = argv[i+1];}
        if (strcmp(argv[i], "-blk") == 0) {blockSize = atoi(argv[i+1]);}
        if (strcmp(argv[i], "-t") == 0) {threadCount = atoi(argv[i+1]);}
        if (strcmp(argv[i], "-rt") == 0) {runtime = atoi(argv[i+1]);}
        if (strcmp(argv[i], "-d") == 0) {debug = true;}
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
        args.debug = debug;
        threads.push_back(std::async(syncioSequentialRead, args));
    }

    double aggregateStats = 0;
    for (auto& t : threads) {
        aggregateStats += t.get();
    }

    cout << "Total throughput = " << aggregateStats << " GB/s" << endl;
    return 0;
}
