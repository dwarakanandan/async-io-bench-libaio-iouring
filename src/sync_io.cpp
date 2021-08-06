#include "syncio.h"

using namespace std;

Result_t syncioRead(const RuntimeArgs_t& args, char* buffer, size_t buffer_size, bool isRand) {
    uint64_t ops = 0;

    double start = getTime();
    while (getTime() - start < RUN_TIME) {
        ssize_t readCount = pread(args.fd, buffer, buffer_size, getOffset(args.read_offset, buffer_size, ops, isRand));
        if(readCount < 0) {
            perror("Read error");
            return return_error();
        }
        ops++;
    }

    Result_t results;
    results.throughput = calculateThroughputGbps(ops, buffer_size);
    results.op_count = ops;

    return results;
}

Result_t syncioWrite(const RuntimeArgs_t& args, char* buffer, size_t buffer_size, bool isRand) {
    uint64_t ops = 0;

    double start = getTime();
    while (getTime() - start < RUN_TIME) {
        ssize_t writeCount = pwrite(args.fd, buffer, buffer_size, getOffset(args.read_offset, buffer_size, ops, isRand));
        if(writeCount < 0) {
            perror("Write error");
            return return_error();
        }
        ops++;
    }

    Result_t results;
    results.throughput = calculateThroughputGbps(ops, buffer_size);
    results.op_count = ops;

    return results;
}

Result_t syncio(const RuntimeArgs_t& args) {
    size_t buffer_size = 1024 * args.blk_size;
    bool isRead = (args.operation.compare(READ) == 0);
    bool isRand = (args.opmode.compare(RANDOM) == 0);

    char* buffer = (char *) aligned_alloc(1024, buffer_size);
    memset(buffer, '0', buffer_size);

    Result_t results;
    if (isRead) {
        results = syncioRead(args, buffer, buffer_size, isRand);
    } else {
        results = syncioWrite(args, buffer, buffer_size, isRand);
    }

    if (args.debugInfo) printStats(args, results);
    return results;
}
