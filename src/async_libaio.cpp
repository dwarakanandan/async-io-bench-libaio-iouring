#include "async_libaio.h"

using namespace std;

Result_t _async_libaio(const RuntimeArgs_t &args)
{
    size_t buffer_size = 1024 * args.blk_size;
    uint64_t ops_submitted = 0, ops_returned = 0, ops_failed = 0;
    bool isRead = (args.operation == READ);
    bool isRand = (args.opmode == RANDOM);

    aio_context_t ctx = 0;
    struct iocb cb[args.oio];
    struct iocb *cbs[args.oio];
    struct io_event events[args.oio];
    timespec timeout;
    char *buffer[args.oio];
    off_t offsets[args.oio];
    int ret;

    for (int i = 0; i < args.oio; i++)
    {
        buffer[i] = (char *)aligned_alloc(1024, buffer_size);
        memset(buffer[i], '0', buffer_size);

        /* Pre-calculate first set of offsets */
        offsets[i] = getOffset(args.max_offset, args.read_offset, buffer_size, i, isRand);

        memset(&(cb[i]), 0, sizeof(cb[i]));
        cb[i].aio_fildes = args.fd;
        cb[i].aio_buf = (uint64_t)buffer[i];
        cb[i].aio_nbytes = buffer_size;
        cbs[i] = &(cb[i]);
    }

    /* Initialize libaio */
    ret = io_setup(args.oio, &ctx);
    if (ret < 0)
    {
        perror(getErrorMessageWithTid(args, "io_setup"));
        return return_error();
    }

    double start = getTime();
    while (getTime() - start < args.runtime)
    {
        /* Prepare args.oio requests */
        for (int i = 0; i < args.oio; i++)
        {
            cb[i].aio_offset = offsets[i];
            cb[i].aio_lio_opcode = isRead ? IOCB_CMD_PREAD : IOCB_CMD_PWRITE;
        }

        /* Submit args.oio requests */
        ret = io_submit(ctx, args.oio, cbs);
        if (ret < 0)
        {
            perror(getErrorMessageWithTid(args, "io_submit"));
            return return_error();
        }
        ops_submitted += ret;

        /* Pre-calculate next set of offsets */
        for (int i = 0; i < args.oio; i++)
        {
            offsets[i] = getOffset(args.max_offset, args.read_offset, buffer_size, ops_submitted + i, isRand);
        }

        /* Wait for args.oio requests to complete */
        timeout.tv_sec = 1;
        ret = io_getevents(ctx, args.oio, args.oio, events, &timeout);
        if (ret < 0)
        {
            fprintf(stderr, "io_getevents failed with code: %d\n", ret);
            return return_error();
        }
        ops_returned += ret;

        /* Check completion event result code */
        for (int i = 0; i < ret; i++)
        {
            if (events[i].res < 0)
            {
                ops_failed++;
            }
        }
    }

    /* Cleanup libaio */
    ret = io_destroy(ctx);
    if (ret < 0)
    {
        perror(getErrorMessageWithTid(args, "io_destroy"));
        return return_error();
    }

    Result_t results;
    results.throughput = calculateThroughputGbps(ops_returned - ops_failed, buffer_size, args.runtime);
    results.op_count = ops_returned - ops_failed;
    results.ops_submitted = ops_submitted;
    results.ops_returned = ops_returned;
    results.ops_failed = ops_failed;
    return results;
}

Result_t _async_libaio_vectored(const RuntimeArgs_t &args)
{
    size_t buffer_size = 1024 * args.blk_size;
    uint64_t ops_submitted = 0, ops_returned = 0, ops_failed = 0;
    bool isRead = (args.operation == READ);
    bool isRand = (args.opmode == RANDOM);

    aio_context_t ctx = 0;
    struct iocb cb[args.oio];
    struct iocb *cbs[args.oio];
    struct io_event events[args.oio];
    timespec timeout;
    struct iovec *iovecs_oio[args.oio];
    off_t offsets[args.oio];
    int ret;

    for (int i = 0; i < args.oio; i++)
    {
        iovecs_oio[i] = (iovec *)calloc(args.vec_size, sizeof(struct iovec));
        for (int j = 0; j < args.vec_size; j++)
        {
            iovecs_oio[i][j].iov_base = (char *)aligned_alloc(1024, buffer_size);
            iovecs_oio[i][j].iov_len = buffer_size;
        }

        /* Pre-calculate first set of offsets */
        offsets[i] = getOffset(args.max_offset, args.read_offset, buffer_size, i * args.vec_size, isRand);

        memset(&(cb[i]), 0, sizeof(cb[i]));
        cb[i].aio_fildes = args.fd;
        cb[i].aio_buf = (uint64_t)iovecs_oio[i];
        cb[i].aio_nbytes = args.vec_size;
        cbs[i] = &(cb[i]);
    }

    /* Initialize libaio */
    ret = io_setup(args.oio, &ctx);
    if (ret < 0)
    {
        perror(getErrorMessageWithTid(args, "io_setup"));
        return return_error();
    }

    double start = getTime();
    while (getTime() - start < args.runtime)
    {
        /* Prepare args.oio requests */
        for (int i = 0; i < args.oio; i++)
        {
            cb[i].aio_offset = offsets[i];
            cb[i].aio_lio_opcode = isRead ? IOCB_CMD_PREADV : IOCB_CMD_PWRITEV;
        }

        /* Submit args.oio requests */
        ret = io_submit(ctx, args.oio, cbs);
        if (ret < 0)
        {
            perror(getErrorMessageWithTid(args, "io_submit"));
            return return_error();
        }
        ops_submitted += (args.oio * args.vec_size);

        /* Pre-calculate next set of offsets */
        for (int i = 0; i < args.oio; i++)
        {
            offsets[i] = getOffset(args.max_offset, args.read_offset, buffer_size, ops_submitted + (i * args.vec_size), isRand);
        }

        /* Wait for args.oio requests to complete */
        timeout.tv_sec = 1;
        ret = io_getevents(ctx, args.oio, args.oio, events, &timeout);
        if (ret < 0)
        {
            fprintf(stderr, "io_getevents failed with code: %d\n", ret);
            return return_error();
        }
        ops_returned += (args.oio * args.vec_size);

        /* Check completion event result code */
        for (int i = 0; i < ret; i++)
        {
            if (events[i].res < 0)
            {
                ops_failed += args.vec_size;
            }
        }
    }

    /* Cleanup libaio */
    ret = io_destroy(ctx);
    if (ret < 0)
    {
        perror(getErrorMessageWithTid(args, "io_destroy"));
        return return_error();
    }

    Result_t results;
    results.throughput = calculateThroughputGbps(ops_returned - ops_failed, buffer_size, args.runtime);
    results.op_count = ops_returned - ops_failed;
    results.ops_submitted = ops_submitted;
    results.ops_returned = ops_returned;
    results.ops_failed = ops_failed;
    return results;
}

Result_t _async_libaio_stress(const RuntimeArgs_t &args)
{
    uint64_t ops_submitted = 0, ops_returned = 0, ops_failed = 0;
    int benchmark_iteration = 0;
    double benchmark_throughput = 0;
    uint64_t benchmark_opcount = 0;
    off_t benchmark_offset = 0;
    size_t buffer_size = 1024 * args.blk_size;
    bool isRead = (args.operation == READ);
    bool isRand = (args.opmode == RANDOM);

    aio_context_t ctx = 0;
    struct iocb cb[args.oio];
    struct iocb *cbs[args.oio];
    struct io_event events[args.oio];
    timespec timeout;
    char *buffer[args.oio];
    off_t offsets[args.oio];
    int ret;

    for (int i = 0; i < args.oio; i++)
    {
        buffer[i] = (char *)aligned_alloc(1024, buffer_size);
        memset(buffer[i], '0', buffer_size);

        /* Pre-calculate first set of offsets */
        offsets[i] = getOffset(args.max_offset, args.read_offset, buffer_size, i, isRand);

        memset(&(cb[i]), 0, sizeof(cb[i]));
        cb[i].aio_fildes = args.fd;
        cb[i].aio_buf = (uint64_t)buffer[i];
        cb[i].aio_nbytes = buffer_size;
        cbs[i] = &(cb[i]);
    }

    /* Initialize libaio */
    ret = io_setup(args.oio, &ctx);
    if (ret < 0)
    {
        perror(getErrorMessageWithTid(args, "io_setup"));
        return return_error();
    }

    while (benchmark_iteration < args.runtime)
    {
        ops_submitted = 0;
        ops_returned = 0;
        ops_failed = 0;
        benchmark_offset = offsets[args.oio - 1];

        double start_iteration = getTime();
        while (getTime() - start_iteration < 1)
        {
            /* Prepare args.oio requests */
            for (int i = 0; i < args.oio; i++)
            {
                cb[i].aio_offset = offsets[i];
                cb[i].aio_lio_opcode = isRead ? IOCB_CMD_PREAD : IOCB_CMD_PWRITE;
            }

            /* Submit args.oio requests */
            ret = io_submit(ctx, args.oio, cbs);
            if (ret < 0)
            {
                perror(getErrorMessageWithTid(args, "io_submit"));
                return return_error();
            }
            ops_submitted += ret;

            /* Pre-calculate next set of offsets */
            for (int i = 0; i < args.oio; i++)
            {
                offsets[i] = getOffset(args.max_offset, benchmark_offset, buffer_size, ops_submitted + i, isRand);
            }

            /* Wait for args.oio requests to complete */
            timeout.tv_sec = 1;
            ret = io_getevents(ctx, args.oio, args.oio, events, &timeout);
            if (ret < 0)
            {
                fprintf(stderr, "io_getevents failed with code: %d\n", ret);
                return return_error();
            }
            ops_returned += ret;

            /* Check completion event result code */
            for (int i = 0; i < ret; i++)
            {
                if (events[i].res < 0)
                {
                    ops_failed++;
                }
            }
        }

        Result_t iteration_results;
        iteration_results.throughput = calculateThroughputGbps(ops_returned - ops_failed, buffer_size, 1);
        iteration_results.op_count = ops_returned - ops_failed;
        benchmark_throughput += iteration_results.throughput;
        benchmark_opcount += iteration_results.op_count;

        printIterationStats(args, benchmark_iteration, iteration_results);
        benchmark_iteration++;
    }

    /* Cleanup libaio */
    ret = io_destroy(ctx);
    if (ret < 0)
    {
        perror(getErrorMessageWithTid(args, "io_destroy"));
        return return_error();
    }

    Result_t results;
    results.throughput = benchmark_throughput / args.runtime;
    results.op_count = benchmark_opcount / args.runtime;

    return results;
}

Result_t async_libaio(const RuntimeArgs_t &args)
{
    Result_t results;
    switch (args.benchmark_type)
    {
    case NORMAL:
        results = (args.vec_size > 0) ? _async_libaio_vectored(args) : _async_libaio(args);
        break;
    case STRESS:
        results = _async_libaio_stress(args);
        break;
    default:
        break;
    }
    if (args.debugInfo)
        printOpStats(args, results);
    return results;
}