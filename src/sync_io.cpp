#include "sync_io.h"

using namespace std;

Result_t syncio(const RuntimeArgs_t& args) {
    uint64_t ops = 0;
    size_t buffer_size = 1024 * args.blk_size;
    bool isRead = (args.operation == READ);
    bool isRand = (args.opmode == RANDOM);

    char* buffer[args.oio];

    struct iovec *iovecs;
    iovecs = (iovec*) calloc(args.oio, sizeof(struct iovec));
    for (int i = 0; i < args.oio; i++)
    {
        buffer[i] = (char *) aligned_alloc(1024, buffer_size);
	    memset(buffer[i], '0', buffer_size);
        iovecs[i].iov_base = buffer[i];
        iovecs[i].iov_len = buffer_size;
    }

    double start = getTime();
    while (getTime() - start < RUN_TIME) {
        ssize_t opCount = isRead ?
            preadv(args.fd, iovecs, args.oio, getOffset(args.read_offset, buffer_size, ops, isRand)) :
            pwritev(args.fd, iovecs, args.oio, getOffset(args.read_offset, buffer_size, ops, isRand));

        if(opCount < 0) {
            perror("IO error");
            return return_error();
        }

        ops+= args.oio;
    }

    Result_t results;
    results.throughput = calculateThroughputGbps(ops, buffer_size);
    results.op_count = ops;

    if (args.debugInfo) printStats(args, results);
    return results;
}
