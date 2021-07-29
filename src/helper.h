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

#define _100MB (1024*1024*100)
#define _1GB (1024*1024*1024*1L)
#define _100GB (1024*1024*1024*100L)
#define MAX_READ_OFFSET (1024*1024*1024*3500L)
#define RUN_TIME 2

extern const std::string SEQUENTIAL;
extern const std::string RANDOM;

extern const std::string READ;
extern const std::string WRITE;

struct RuntimeArgs_t {
    std::string filename;
    int thread_id;
    int thread_count;
    int fd;
    int blk_size;
    off_t read_offset;
    bool debugInfo;
    std::string operation;
    std::string opmode;
};

struct Result_t {
  double throughput;
  uint64_t op_count;
};

static inline double getTime(void) {
  struct timeval now_tv;
  gettimeofday (&now_tv, NULL);
  return ((double)now_tv.tv_sec) + ((double)now_tv.tv_usec)/1000000.0;
}

void printStats(const RuntimeArgs_t& args, const Result_t results);

RuntimeArgs_t mapUserArgsToRuntimeArgs(int argc, char const *argv[]);

void fileOpen(RuntimeArgs_t *args);

void runBenchmark(RuntimeArgs_t& userArgs, Result_t (*benchmarkFunction)(const RuntimeArgs_t& args));