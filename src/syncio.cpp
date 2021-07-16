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
#define _100MB (1024*1024*100)
#define _1GB (1024*1024*1024*1L)
#define _100GB (1024*1024*1024*100L)
#define MAX_READ_OFFSET (1024*1024*1024*3500L)
#define MAX_OPS 100000
#define RUN_TIME 1

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
};

void printStats(const RuntimeArgs_t& args, double throughput, uint64_t ops) {
    std::stringstream stats;
    stats << "TID:" << args.thread_id
        << " offset:" << args.read_offset / _1GB << "GB"
        << " ops:" << ops
        << " throughput:" << throughput << "GB/s" << endl;
    if (args.debugInfo) cout << stats.str();
}

void syncioRead(double start, int fd, char* buffer, size_t buffer_size, off_t offsets[], uint64_t* ops) {
    while (gettime() - start < RUN_TIME) {
        ssize_t readCount = pread(fd, buffer, buffer_size, offsets[*ops]);
        if(readCount < 0) {
            perror("Read error");
            return;
        }
        (*ops)++;
    }
}

double syncioSequential(const RuntimeArgs_t& args) {
    size_t buffer_size  = _100MB;
    char* buffer = (char *) aligned_alloc(1024, buffer_size);
    uint64_t ops = 0;

    off_t offsets[MAX_OPS];
    for(int i=0; i < MAX_OPS; i++) {
        offsets[i] = args.read_offset + (i * buffer_size) % _100GB;
    }

    double start = gettime();
    
    syncioRead(start, args.fd, buffer, buffer_size, offsets, &ops);

    double throughput = ((ops * buffer_size)/(1024.0*1024*1024 * RUN_TIME));
    printStats(args, throughput, ops);
    return throughput;
}

double syncioRandom(const RuntimeArgs_t& args) {
    size_t buffer_size  = 1024 * args.blk_size;
    char* buffer = (char *) aligned_alloc(1024, buffer_size);
    uint64_t ops = 0;

    off_t offsets[MAX_OPS];
    for(int i=0; i < MAX_OPS; i++) {
        offsets[i] = args.read_offset + (rand() * buffer_size) % _100GB;
    }

    double start = gettime();
    syncioRead(start, args.fd, buffer, buffer_size, offsets, &ops);

    double throughput = ((ops * buffer_size)/(1024.0*1024*1024 * RUN_TIME));
    printStats(args, throughput, ops);
    return throughput;
}

void runBenchmark(const RuntimeArgs_t& userArgs, int threadCount, const char* readMode) {
    std::vector<std::future<double>> threads;
    for (int i = 0; i < threadCount; ++i) {
        RuntimeArgs_t args;
        args.thread_id = i;
        args.blk_size = userArgs.blk_size;
        args.fd = userArgs.fd;
        args.debugInfo = userArgs.debugInfo;
        args.read_offset = (_100GB * i) % MAX_READ_OFFSET;
        if (strcmp(readMode, SEQUENTIAL) == 0) {
            threads.push_back(std::async(syncioSequential, args));
        } else {
            threads.push_back(std::async(syncioRandom, args));
        }
    }
    double totalThroughput = 0;
    for (auto& t : threads) {
        totalThroughput += t.get();
    }
    if (strcmp(readMode, SEQUENTIAL) == 0) {
        cout << SEQUENTIAL << " Block-size: " << _100MB/(1024) << " kB  Throughput = " << totalThroughput << " GB/s" << endl;
    } else {
        cout << RANDOM << " Block-size: " << userArgs.blk_size << " kB  Throughput = " << totalThroughput << " GB/s" << endl;
    }
}

int main(int argc, char const *argv[])
{
    if (argc < 3 ) {
        cout << "Usage: syncio -f <file> -t <threads> -blk <block_size_kB> -d(enables debuginfo)" << endl;
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

    runBenchmark(args, threadCount, SEQUENTIAL);
    runBenchmark(args, threadCount, RANDOM);
    return 0;
}
