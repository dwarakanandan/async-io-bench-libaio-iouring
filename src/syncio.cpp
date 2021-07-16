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
#define MAX_READ_OFFSET (1024*1024*1024*3500L)

const char* SEQUENTIAL = "Sequential";
const char* RANDOM = "Random";

static inline double gettime(void) {
  struct timeval now_tv;
  gettimeofday (&now_tv, NULL);
  return ((double)now_tv.tv_sec) + ((double)now_tv.tv_usec)/1000000.0;
}

struct RuntimeArgs_t {
    int thread_id;
    int fd;
    int blk_size;
    off_t read_offset;
    bool debugInfo;
    const char* readMode;
};

void printStats(const RuntimeArgs_t& args, double throughput) {
    std::stringstream stats;
    stats << "TID:" << args.thread_id
        << " read_offset: " << args.read_offset / _1GB << " GB"
        << " throughput: " << throughput << " GB/s" << endl;
    if (args.debugInfo) cout << stats.str();
}

double syncioRead(const RuntimeArgs_t& args) {
    size_t page_size  = 1024 * args.blk_size;
    char* buffer = (char *) aligned_alloc(1024, page_size);
    uint64_t ops = 0;

    off_t offsets[1000000];
    if (strcmp(args.readMode, SEQUENTIAL) == 0) {
        for(int i=0; i<1000000; i++) {
            offsets[i] = args.read_offset + (i * page_size) % _100GB;
        }
    } else {
        for(int i=0; i<1000000; i++) {
            offsets[i] = args.read_offset + (rand() * page_size) % _100GB;
        }
    }

    double start = gettime();
    while (gettime() - start < 1) {
        pread(args.fd, buffer, page_size, offsets[ops]);
        ops++;
    }

    double throughput = ((ops * 1024 * args.blk_size)/(1024.0*1024*1024));
    printStats(args, throughput);
    return throughput;
}

void runReadBenchmark(const RuntimeArgs_t& userArgs, int threadCount, const char* readMode) {
    std::vector<std::future<double>> threads;
    for (int i = 0; i < threadCount; ++i) {
        RuntimeArgs_t args;
        args.thread_id = i;
        args.blk_size = userArgs.blk_size;
        args.fd = userArgs.fd;
        args.debugInfo = userArgs.debugInfo;
        args.read_offset = (_100GB * i) % MAX_READ_OFFSET;
        args.readMode = readMode;
        threads.push_back(std::async(syncioRead, args));
    }
    double totalThroughput = 0;
    for (auto& t : threads) {
        totalThroughput += t.get();
    }
    cout << readMode << " : Total throughput = " << totalThroughput << " GB/s" << endl;
}

int main(int argc, char const *argv[])
{
    if (argc < 3 ) {
        cout << "Usage: syncio -f <file> -t <threads> -rt <runtime> -blk <block_size_kB> -d(enables debuginfo)" << endl;
        exit(1);
    }

    srand(time(NULL));

    const char* filename;
    int threadCount = 1;
    RuntimeArgs_t args;
    args.blk_size = 16;
    args.debugInfo = false;
    
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-f") == 0) {filename = argv[i+1];}
        if (strcmp(argv[i], "-blk") == 0) {args.blk_size = atoi(argv[i+1]);}
        if (strcmp(argv[i], "-t") == 0) {threadCount = atoi(argv[i+1]);}
        if (strcmp(argv[i], "-d") == 0) {args.debugInfo = true;}
    }

    args.fd = open(filename, O_RDWR | O_CREAT | O_DIRECT, S_IRUSR | S_IWUSR);
    if (args.fd < 0) {
        perror("Open error");
        return -1;
    }

    runReadBenchmark(args, threadCount, SEQUENTIAL);
    runReadBenchmark(args, threadCount, RANDOM);
    return 0;
}
