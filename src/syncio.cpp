#include "syncio.h"

using namespace std;

void syncioRead(const RuntimeArgs_t& args, char* buffer, size_t buffer_size, uint64_t* ops, bool isRand) {
    double start = getTime();
    while (getTime() - start < RUN_TIME) {
        ssize_t readCount = pread(args.fd, buffer, buffer_size, getOffset(args.read_offset, args.blk_size, (*ops), isRand));
        if(readCount < 0) {
            perror("Read error");
            return;
        }
        (*ops)++;
    }
}

void syncioWrite(const RuntimeArgs_t& args, char* buffer, size_t buffer_size, uint64_t* ops, bool isRand) {
    double start = getTime();
    while (getTime() - start < RUN_TIME) {
        ssize_t writeCount = pwrite(args.fd, buffer, buffer_size, getOffset(args.read_offset, args.blk_size, (*ops), isRand));
        if(writeCount < 0) {
            perror("Write error");
            return;
        }
        (*ops)++;
    }
}

Result_t syncio(const RuntimeArgs_t& args) {
    size_t buffer_size = 1024 * args.blk_size;
    uint64_t ops = 0;
    bool isRead = (args.operation.compare(READ) == 0);
    bool isRand = (args.opmode.compare(RANDOM) == 0);

    char* buffer = (char *) aligned_alloc(1024, buffer_size);
    memset(buffer, '0', buffer_size);

    if (isRead) {
        syncioRead(args, buffer, buffer_size, &ops, isRand);
    } else {
        syncioWrite(args, buffer, buffer_size, &ops, isRand);
    }

    Result_t results;
    results.throughput = calculateThroughputGbps(ops, buffer_size);
    results.op_count = ops;

    if (args.debugInfo) printStats(args, results);
    return results;
}
