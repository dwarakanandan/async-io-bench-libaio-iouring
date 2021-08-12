#include "sync_io.h"

using namespace std;

Result_t _syncio(const RuntimeArgs_t& args) {
    uint64_t ops_submitted = 0, ops_returned = 0, ops_failed = 0;
    size_t buffer_size = 1024 * args.blk_size;
    bool isRead = (args.operation == READ);
    bool isRand = (args.opmode == RANDOM);

    char* buffer = (char *) aligned_alloc(1024, buffer_size);
    memset(buffer, '0', buffer_size);

    double start = getTime();
    while (getTime() - start < RUN_TIME) {
        ssize_t opCount = isRead ?
            pread(args.fd, buffer, buffer_size, getOffset(args.max_offset, args.read_offset, buffer_size, ops_submitted, isRand)) :
            pwrite(args.fd, buffer, buffer_size, getOffset(args.max_offset, args.read_offset, buffer_size, ops_submitted, isRand));
        ops_submitted++;
        if(opCount < 0) {
            ops_failed++;
        }
        ops_returned++;
    }

    Result_t results;
    results.throughput = calculateThroughputGbps(ops_returned-ops_failed, buffer_size);
    results.op_count = ops_returned-ops_failed;
	results.ops_submitted = ops_submitted;
	results.ops_returned = ops_returned;
	results.ops_failed = ops_failed;
    return results;
}

Result_t syncio(const RuntimeArgs_t& args) {
    Result_t results = _syncio(args);
	if (args.debugInfo) printOpStats(args, results);
    return results;
}
