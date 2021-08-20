#include "sync_io.h"

using namespace std;

Result_t _syncio(const RuntimeArgs_t &args)
{
    uint64_t ops_submitted = 0, ops_returned = 0, ops_failed = 0;
    size_t buffer_size = 1024 * args.blk_size;
    bool isRead = (args.operation == READ);
    bool isRand = (args.opmode == RANDOM);

    char *buffer = (char *)aligned_alloc(1024, buffer_size);
    memset(buffer, '0', buffer_size);

    double start = getTime();
    while (getTime() - start < args.runtime)
    {
        ssize_t opCount = isRead ? pread(args.fd, buffer, buffer_size, getOffset(args.max_offset, args.read_offset, buffer_size, ops_submitted, isRand)) : pwrite(args.fd, buffer, buffer_size, getOffset(args.max_offset, args.read_offset, buffer_size, ops_submitted, isRand));
        ops_submitted++;
        if (opCount < 0)
        {
            ops_failed++;
        }
        ops_returned++;
    }

    Result_t results;
    results.throughput = calculateThroughputGbps(ops_returned - ops_failed, buffer_size, args.runtime);
    results.op_count = ops_returned - ops_failed;
    results.ops_submitted = ops_submitted;
    results.ops_returned = ops_returned;
    results.ops_failed = ops_failed;
    return results;
}

Result_t _syncio_vectored(const RuntimeArgs_t &args)
{
    uint64_t ops_submitted = 0, ops_returned = 0, ops_failed = 0;
    size_t buffer_size = 1024 * args.blk_size;
    bool isRead = (args.operation == READ);
    bool isRand = (args.opmode == RANDOM);

    struct iovec *iovecs;
    iovecs = (iovec *)calloc(args.vec_size, sizeof(struct iovec));
    for (int j = 0; j < args.vec_size; j++)
    {
        iovecs[j].iov_base = (char *)aligned_alloc(1024, buffer_size);
        iovecs[j].iov_len = buffer_size;
    }

    double start = getTime();
    while (getTime() - start < args.runtime)
    {
        ssize_t opCount = isRead ? preadv(args.fd, iovecs, args.vec_size, getOffset(args.max_offset, args.read_offset, buffer_size, ops_submitted, isRand)) : pwritev(args.fd, iovecs, args.vec_size, getOffset(args.max_offset, args.read_offset, buffer_size, ops_submitted, isRand));
        ops_submitted += args.vec_size;
        if (opCount < 0)
        {
            ops_failed += args.vec_size;
        }
        ops_returned += args.vec_size;
    }

    Result_t results;
    results.throughput = calculateThroughputGbps(ops_returned - ops_failed, buffer_size, args.runtime);
    results.op_count = ops_returned - ops_failed;
    results.ops_submitted = ops_submitted;
    results.ops_returned = ops_returned;
    results.ops_failed = ops_failed;
    return results;
}

Result_t _syncio_stress(const RuntimeArgs_t &args)
{
    uint64_t ops_submitted = 0, ops_returned = 0, ops_failed = 0;
    int benchmark_iteration = 1;
    double benchmark_throughput = 0;
    uint64_t benchmark_opcount = 0;
    size_t buffer_size = 1024 * args.blk_size;
    bool isRead = (args.operation == READ);
    bool isRand = (args.opmode == RANDOM);

    char *buffer = (char *)aligned_alloc(1024, buffer_size);
    memset(buffer, '0', buffer_size);

    while (benchmark_iteration <= args.runtime)
    {
        ops_submitted = 0;
        ops_returned = 0;
        ops_failed = 0;

        double start_iteration = getTime();
        while (getTime() - start_iteration < 1)
        {
            ssize_t opCount = isRead ? pread(args.fd, buffer, buffer_size, getOffset(args.max_offset, args.read_offset, buffer_size, ops_submitted, isRand)) : pwrite(args.fd, buffer, buffer_size, getOffset(args.max_offset, args.read_offset, buffer_size, ops_submitted, isRand));
            ops_submitted++;
            if (opCount < 0)
            {
                ops_failed++;
            }
            ops_returned++;
        }

        Result_t iteration_results;
        iteration_results.throughput = calculateThroughputGbps(ops_returned - ops_failed, buffer_size, 1);
        iteration_results.op_count = ops_returned - ops_failed;
        benchmark_throughput += iteration_results.throughput;
        benchmark_opcount += iteration_results.op_count;

        printIterationStats(args, benchmark_iteration, iteration_results);
        benchmark_iteration++;
    }

    Result_t results;
    results.throughput = benchmark_throughput / args.runtime;
    results.op_count = benchmark_opcount / args.runtime;

    return results;
}

Result_t syncio(const RuntimeArgs_t &args)
{
    Result_t results;
    switch (args.benchmark_type)
    {
    case NORMAL:
        results = (args.vec_size > 0) ? _syncio_vectored(args) : _syncio(args);
        break;
    case STRESS:
        results = _syncio_stress(args);
        break;
    case POLL:
        /* code */
        break;
    default:
        break;
    }

    if (args.debugInfo)
        printOpStats(args, results);
    return results;
}
