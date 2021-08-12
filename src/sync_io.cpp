#include "sync_io.h"

using namespace std;

Result_t syncio(const RuntimeArgs_t& args) {
    uint64_t ops = 0;
    size_t buffer_size = 1024 * args.blk_size;
    bool isRead = (args.operation == READ);
    bool isRand = (args.opmode == RANDOM);

    char* buffer = (char *) aligned_alloc(1024, buffer_size);
    memset(buffer, '0', buffer_size);

    double start = getTime();
    while (getTime() - start < RUN_TIME) {
        ssize_t opCount = isRead ?
            pread(args.fd, buffer, buffer_size, getOffset(args.read_offset, buffer_size, ops, isRand)) :
            pwrite(args.fd, buffer, buffer_size, getOffset(args.read_offset, buffer_size, ops, isRand));

        if(opCount < 0) {
            perror("IO error");
            return return_error();
        }

        ops++;
    }

    Result_t results;
    results.throughput = calculateThroughputGbps(ops, buffer_size);
    results.op_count = ops;

    if (args.debugInfo) printStats(args, results);
    return results;
}
