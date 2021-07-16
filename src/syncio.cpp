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
#define MAX_OPS 200000
#define RUN_TIME 2

const char* SEQUENTIAL = "SEQUENTIAL";
const char* RANDOM = "RANDOM";

const char* READ = "READ";
const char* WRITE = "WRITE";

static inline double gettime(void) {
  struct timeval now_tv;
  gettimeofday (&now_tv, NULL);
  return ((double)now_tv.tv_sec) + ((double)now_tv.tv_usec)/1000000.0;
}

struct RuntimeArgs_t {
    int thread_id;
    int thread_count;
    int fd;
    int blk_size;
    off_t read_offset;
    bool debugInfo;
    const char* operation;
    const char* opmode;
};

void printStats(const RuntimeArgs_t& args, double throughput, uint64_t ops) {
    std::stringstream stats;
    stats << "TID:" << args.thread_id
        << " offset:" << args.read_offset / _1GB << "GB"
        << " ops:" << ops
        << " throughput:" << throughput << " GB/s" << endl;
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

void syncioWrite(double start, int fd, char* buffer, size_t buffer_size, off_t offsets[], uint64_t* ops) {
    while (gettime() - start < RUN_TIME) {
        ssize_t writeCount = pwrite(fd, buffer, buffer_size, offsets[*ops]);
        if(writeCount < 0) {
            perror("Write error");
            return;
        }
        (*ops)++;
    }
}

double syncio(const RuntimeArgs_t& args) {
    size_t buffer_size = 1024 * args.blk_size;;
    uint64_t ops = 0;
    off_t offsets[MAX_OPS];
    
    if (strcmp(args.opmode, SEQUENTIAL) == 0) {
        for(int i=0; i < MAX_OPS; i++) {
            offsets[i] = args.read_offset + (i * buffer_size) % _100GB;
        }
    } else {
        for(int i=0; i < MAX_OPS; i++) {
            offsets[i] = args.read_offset + (rand() * buffer_size) % _100GB;
        }
    }

    char* buffer = (char *) aligned_alloc(1024, buffer_size);
    memset(buffer, '1', buffer_size);

    double start = gettime();
    if (strcmp(args.operation, READ) == 0) {
        syncioRead(start, args.fd, buffer, buffer_size, offsets, &ops);
    } else {
        syncioWrite(start, args.fd, buffer, buffer_size, offsets, &ops);
    }
    double throughput = ((ops * buffer_size)/(1024.0*1024*1024 * RUN_TIME));

    printStats(args, throughput, ops);
    return throughput;
}

void runBenchmark(const RuntimeArgs_t& userArgs, const char* operation, const char* mode) {
    std::vector<std::future<double>> threads;
    for (int i = 0; i < userArgs.thread_count; ++i) {
        RuntimeArgs_t args;
        args.thread_id = i;
        args.blk_size = (strcmp(mode, SEQUENTIAL) == 0) ? 102400: userArgs.blk_size;
        args.fd = userArgs.fd;
        args.debugInfo = userArgs.debugInfo;
        args.read_offset = (_100GB * i) % MAX_READ_OFFSET;
        args.operation = operation;
        args.opmode = mode;
        threads.push_back(std::async(syncio, args));
    }
    double totalThroughput = 0;
    for (auto& t : threads) {
        totalThroughput += t.get();
    }
    cout << operation << " " <<  mode << " BLK_SIZE: " << userArgs.blk_size <<
            " kB   Throughput = " << totalThroughput << " GB/s" << endl << endl;
}

int main(int argc, char const *argv[])
{
    if (argc < 3 ) {
        cout << "syncio -f <file> -t <threads> -blk <block_size_kB> -d(enables debuginfo)" << endl;
        exit(1);
    }

    srand(time(NULL));

    const char* filename;
    RuntimeArgs_t args;
    args.thread_count = 1;
    args.blk_size = 16;
    args.debugInfo = false;
    
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-f") == 0) {filename = argv[i+1];}
        if (strcmp(argv[i], "-blk") == 0) {args.blk_size = atoi(argv[i+1]);}
        if (strcmp(argv[i], "-t") == 0) {args.thread_count = atoi(argv[i+1]);}
        if (strcmp(argv[i], "-d") == 0) {args.debugInfo = true;}
    }

    args.fd = open(filename, O_RDWR | O_CREAT | O_DIRECT, S_IRUSR | S_IWUSR);
    if (args.fd < 0) {
        perror("Open error");
        return -1;
    }

    runBenchmark(args, READ, SEQUENTIAL);
    runBenchmark(args, WRITE, SEQUENTIAL);

    runBenchmark(args, READ, RANDOM);
    runBenchmark(args, WRITE, RANDOM);

    return 0;
}
