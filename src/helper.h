#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/time.h>
#include <iostream>
#include <unistd.h>
#include <sstream>

#define _100MB (1024 * 1024 * 100)
#define _1GB (1024 * 1024 * 1024 * 1L)
#define _100GB (1024 * 1024 * 1024 * 100L)

enum OPERATION
{
    READ,
    WRITE
};

enum OPMODE
{
    SEQUENTIAL,
    RANDOM
};

enum LIB
{
    SYNCIO,
    LIBAIO,
    IOURING
};

enum BENCHMARK_TYPE
{
    NORMAL,
    STRESS,
    POLL
};

struct RuntimeArgs_t
{
    std::string filename;
    int thread_id;
    int thread_count;
    int fd;
    int blk_size;
    int oio;
    int vec_size;
    int runtime;
    off_t read_offset;
    off_t max_offset;
    bool debugInfo;
    bool odirect;
    OPERATION operation;
    OPMODE opmode;
    LIB lib;
    BENCHMARK_TYPE benchmark_type;
};

struct Result_t
{
    double throughput;
    uint64_t op_count;
    uint64_t ops_submitted;
    uint64_t ops_returned;
    uint64_t ops_failed;
};

static inline double getTime(void)
{
    struct timeval now_tv;
    gettimeofday(&now_tv, NULL);
    return ((double)now_tv.tv_sec) + ((double)now_tv.tv_usec) / 1000000.0;
}

void printStats(const RuntimeArgs_t &args, const Result_t results);

void printOpStats(const RuntimeArgs_t &args, const Result_t results);

void printIterationStats(const RuntimeArgs_t &args, uint64_t iteration, const Result_t results);

const char *getErrorMessageWithTid(const RuntimeArgs_t &args, std::string error);

void fileOpen(RuntimeArgs_t *args);

double calculateThroughputGbps(uint64_t ops, size_t buffer_size, int runtime);

off_t getOffset(off_t maxOffset, off_t initialOffset, size_t buffer_size, uint64_t iteration, bool isRand);

Result_t return_error();

std::string operationToString(OPERATION operation);

std::string opmodeToString(OPMODE opmode);

std::string libToString(LIB lib);