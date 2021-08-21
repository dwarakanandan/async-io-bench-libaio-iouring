#include "async_liburing.h"

using namespace std;

Result_t _async_liburing_vectored(const RuntimeArgs_t &args)
{
    size_t buffer_size = 1024 * args.blk_size;
    uint64_t ops_submitted = 0, ops_returned = 0, ops_failed = 0;
    bool isRead = (args.operation == READ);
    bool isRand = (args.opmode == RANDOM);

    struct io_uring ring;
    struct io_uring_sqe *sqe;
    struct io_uring_cqe *cqe;
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
    }

    /* Initialize io_uring */
    ret = io_uring_queue_init(1024, &ring, 0);
    if (ret < 0)
    {
        perror(getErrorMessageWithTid(args, "io_uring_queue_init"));
        return return_error();
    }

    double start = getTime();
    while (getTime() - start < args.runtime)
    {
        for (int i = 0; i < args.oio; i++)
        {
            /* Get a Submission Queue Entry */
            sqe = io_uring_get_sqe(&ring);
            isRead ? io_uring_prep_readv(sqe, args.fd, iovecs_oio[i], args.vec_size, offsets[i]) : io_uring_prep_writev(sqe, args.fd, iovecs_oio[i], args.vec_size, offsets[i]);
        }

        /* Submit args.oio operations */
        ret = io_uring_submit(&ring);
        ops_submitted += (args.oio * args.vec_size);

        /* Pre-calculate next set of offsets */
        for (int i = 0; i < args.oio; i++)
        {
            offsets[i] = getOffset(args.max_offset, args.read_offset, buffer_size, ops_submitted + (i * args.vec_size), isRand);
        }

        /* Wait for args.oio IO requests to complete */
        ret = io_uring_wait_cqe_nr(&ring, &cqe, args.oio);
        if (ret < 0)
        {
            fprintf(stderr, "Error io_uring_wait_cqe_nr: %s\n", strerror(-ret));
            return return_error();
        }
        size_t head = 0;
        io_uring_for_each_cqe(&ring, head, cqe)
        {
            /* Check completion event result code */
            if (cqe->res < 0)
            {
                ops_failed++;
            }
        }
        io_uring_cq_advance(&ring, args.oio);

        ops_returned += (args.oio * args.vec_size);
    }

    /* Cleanup io_uring */
    io_uring_queue_exit(&ring);

    Result_t results;
    results.throughput = calculateThroughputGbps(ops_returned - ops_failed, buffer_size, args.runtime);
    results.op_count = ops_returned - ops_failed;
    results.ops_submitted = ops_submitted;
    results.ops_returned = ops_returned;
    results.ops_failed = ops_failed;
    return results;
}

Result_t _async_liburing_vectored_sqe_polling(const RuntimeArgs_t &args)
{
    size_t buffer_size = 1024 * args.blk_size;
    uint64_t ops_submitted = 0, ops_returned = 0, ops_failed = 0;
    bool isRead = (args.operation == READ);
    bool isRand = (args.opmode == RANDOM);

    struct io_uring ring;
    struct io_uring_params params;
    struct io_uring_sqe *sqe;
    struct io_uring_cqe *cqe;
    struct iovec *iovecs_oio[args.oio];
    off_t offsets[args.oio];
    int fds[1];
    int ret;

    memset(&params, 0, sizeof(params));
    params.flags |= IORING_SETUP_SQPOLL;
    params.sq_thread_idle = 5000;

    /* Initialize io_uring */
    ret = io_uring_queue_init_params(1024, &ring, &params);
    if (ret < 0)
    {
        perror(getErrorMessageWithTid(args, "io_uring_queue_init"));
        return return_error();
    }

    /* Register file descriptors to be used with SQE kernel polling*/
    fds[0] = args.fd;
    ret = io_uring_register_files(&ring, fds, 1);
    if (ret)
    {
        fprintf(stderr, "Error registering fds: %s\n", strerror(-ret));
        return return_error();
    }

    /* Initialize buffers */
    for (int i = 0; i < args.oio; i++)
    {
        iovecs_oio[i] = (iovec *)calloc(args.vec_size, sizeof(struct iovec));
        for (int j = 0; j < args.vec_size; j++)
        {
            iovecs_oio[i][j].iov_base = (char *)aligned_alloc(1024, buffer_size);
            iovecs_oio[i][j].iov_len = buffer_size;
        }
    }

    /* Pre-calculate first set of offsets */
    for (int i = 0; i < args.oio; i++)
    {
        offsets[i] = getOffset(args.max_offset, args.read_offset, buffer_size, i * args.vec_size, isRand);
    }

    double start = getTime();
    while (getTime() - start < args.runtime)
    {
        for (int i = 0; i < args.oio; i++)
        {
            /* Get a Submission Queue Entry */
            sqe = io_uring_get_sqe(&ring);
            /* File descriptor is now an index to the actual FD registered in the fds array*/
            isRead ? io_uring_prep_readv(sqe, 0, iovecs_oio[i], args.vec_size, offsets[i]) : io_uring_prep_writev(sqe, 0, iovecs_oio[i], args.vec_size, offsets[i]);
            sqe->flags |= IOSQE_FIXED_FILE;
        }

        /* Submit args.oio operations */
        ret = io_uring_submit(&ring);
        ops_submitted += (args.oio * args.vec_size);

        /* Pre-calculate next set of offsets */
        for (int i = 0; i < args.oio; i++)
        {
            offsets[i] = getOffset(args.max_offset, args.read_offset, buffer_size, ops_submitted + (i * args.vec_size), isRand);
        }

        /* Wait for args.oio IO requests to complete */
        ret = io_uring_wait_cqe_nr(&ring, &cqe, args.oio);
        if (ret < 0)
        {
            fprintf(stderr, "Error io_uring_wait_cqe_nr: %s\n", strerror(-ret));
            return return_error();
        }
        size_t head = 0;
        io_uring_for_each_cqe(&ring, head, cqe)
        {
            /* Check completion event result code */
            if (cqe->res < 0)
            {
                ops_failed++;
            }
        }
        io_uring_cq_advance(&ring, args.oio);

        ops_returned += (args.oio * args.vec_size);
    }

    /* Cleanup io_uring */
    io_uring_queue_exit(&ring);

    Result_t results;
    results.throughput = calculateThroughputGbps(ops_returned - ops_failed, buffer_size, args.runtime);
    results.op_count = ops_returned - ops_failed;
    results.ops_submitted = ops_submitted;
    results.ops_returned = ops_returned;
    results.ops_failed = ops_failed;
    return results;
}

Result_t _async_liburing_fixed_buffer(const RuntimeArgs_t &args)
{
    size_t buffer_size = 1024 * args.blk_size;
    uint64_t ops_submitted = 0, ops_returned = 0, ops_failed = 0;
    bool isRead = (args.operation == READ);
    bool isRand = (args.opmode == RANDOM);

    struct io_uring ring;
    struct io_uring_sqe *sqe;
    struct io_uring_cqe *cqe;
    struct iovec *iovecs;
    off_t offsets[args.oio];
    char *buffer[args.oio];
    int ret;

    /* Initialize io_uring */
    ret = io_uring_queue_init(1024, &ring, 0);
    if (ret < 0)
    {
        perror(getErrorMessageWithTid(args, "io_uring_queue_init"));
        return return_error();
    }

    /* Initialize and Register buffers */
    iovecs = (iovec *)calloc(args.oio, sizeof(struct iovec));
    for (int i = 0; i < args.oio; i++)
    {
        buffer[i] = (char *)aligned_alloc(1024, buffer_size);
        memset(buffer[i], '0', buffer_size);
        iovecs[i].iov_base = buffer[i];
        iovecs[i].iov_len = buffer_size;
    }

    ret = io_uring_register_buffers(&ring, iovecs, args.oio);
    if (ret)
    {
        fprintf(stderr, "Error registering buffers: %s\n", strerror(-ret));
        return return_error();
    }

    /* Pre-calculate first set of offsets */
    for (int i = 0; i < args.oio; i++)
    {
        offsets[i] = getOffset(args.max_offset, args.read_offset, buffer_size, i, isRand);
    }

    double start = getTime();
    while (getTime() - start < args.runtime)
    {
        for (int i = 0; i < args.oio; i++)
        {
            /* Get a Submission Queue Entry */
            sqe = io_uring_get_sqe(&ring);
            isRead ? io_uring_prep_read_fixed(sqe, args.fd, iovecs[i].iov_base, buffer_size, offsets[i], i) : io_uring_prep_write_fixed(sqe, args.fd, iovecs[i].iov_base, buffer_size, offsets[i], i);
        }

        /* Submit args.oio operations */
        ret = io_uring_submit(&ring);
        ops_submitted += args.oio;

        /* Pre-calculate next set of offsets */
        for (int i = 0; i < args.oio; i++)
        {
            offsets[i] = getOffset(args.max_offset, args.read_offset, buffer_size, ops_submitted + i, isRand);
        }

        /* Wait for args.oio IO requests to complete */
        ret = io_uring_wait_cqe_nr(&ring, &cqe, args.oio);
        if (ret < 0)
        {
            fprintf(stderr, "Error io_uring_wait_cqe_nr: %s\n", strerror(-ret));
            return return_error();
        }
        size_t head = 0;
        io_uring_for_each_cqe(&ring, head, cqe)
        {
            /* Check completion event result code */
            if (cqe->res < 0)
            {
                ops_failed++;
            }
        }
        io_uring_cq_advance(&ring, args.oio);

        ops_returned += args.oio;
    }

    /* Cleanup io_uring */
    io_uring_queue_exit(&ring);

    Result_t results;
    results.throughput = calculateThroughputGbps(ops_returned - ops_failed, buffer_size, args.runtime);
    results.op_count = ops_returned - ops_failed;
    results.ops_submitted = ops_submitted;
    results.ops_returned = ops_returned;
    results.ops_failed = ops_failed;
    return results;
}

Result_t _async_liburing_fixed_buffer_sqe_polling(const RuntimeArgs_t &args)
{
    size_t buffer_size = 1024 * args.blk_size;
    uint64_t ops_submitted = 0, ops_returned = 0, ops_failed = 0;
    bool isRead = (args.operation == READ);
    bool isRand = (args.opmode == RANDOM);

    struct io_uring ring;
    struct io_uring_params params;
    struct io_uring_sqe *sqe;
    struct io_uring_cqe *cqe;
    struct iovec *iovecs;
    off_t offsets[args.oio];
    char *buffer[args.oio];
    int fds[1];
    int ret;

    memset(&params, 0, sizeof(params));
    params.flags |= IORING_SETUP_SQPOLL;
    params.sq_thread_idle = 5000;

    /* Initialize io_uring */
    ret = io_uring_queue_init_params(1024, &ring, &params);
    if (ret < 0)
    {
        perror(getErrorMessageWithTid(args, "io_uring_queue_init"));
        return return_error();
    }

    /* Register file descriptors to be used with SQE kernel polling*/
    fds[0] = args.fd;
    ret = io_uring_register_files(&ring, fds, 1);
    if (ret)
    {
        fprintf(stderr, "Error registering fds: %s\n", strerror(-ret));
        return return_error();
    }

    /* Initialize and Register buffers */
    iovecs = (iovec *)calloc(args.oio, sizeof(struct iovec));
    for (int i = 0; i < args.oio; i++)
    {
        buffer[i] = (char *)aligned_alloc(1024, buffer_size);
        memset(buffer[i], '0', buffer_size);
        iovecs[i].iov_base = buffer[i];
        iovecs[i].iov_len = buffer_size;
    }

    ret = io_uring_register_buffers(&ring, iovecs, args.oio);
    if (ret)
    {
        fprintf(stderr, "Error registering buffers: %s\n", strerror(-ret));
        return return_error();
    }

    /* Pre-calculate first set of offsets */
    for (int i = 0; i < args.oio; i++)
    {
        offsets[i] = getOffset(args.max_offset, args.read_offset, buffer_size, i, isRand);
    }

    double start = getTime();
    while (getTime() - start < args.runtime)
    {
        for (int i = 0; i < args.oio; i++)
        {
            /* Get a Submission Queue Entry */
            sqe = io_uring_get_sqe(&ring);
            /* File descriptor is now an index to the actual FD registered in the fds array*/
            isRead ? io_uring_prep_read_fixed(sqe, 0, iovecs[i].iov_base, buffer_size, offsets[i], i) : io_uring_prep_write_fixed(sqe, 0, iovecs[i].iov_base, buffer_size, offsets[i], i);
            sqe->flags |= IOSQE_FIXED_FILE;
        }

        /* Submit args.oio operations */
        ret = io_uring_submit(&ring);
        ops_submitted += args.oio;

        /* Pre-calculate next set of offsets */
        for (int i = 0; i < args.oio; i++)
        {
            offsets[i] = getOffset(args.max_offset, args.read_offset, buffer_size, ops_submitted + i, isRand);
        }

        /* Wait for args.oio IO requests to complete */
        ret = io_uring_wait_cqe_nr(&ring, &cqe, args.oio);
        if (ret < 0)
        {
            fprintf(stderr, "Error io_uring_wait_cqe_nr: %s\n", strerror(-ret));
            return return_error();
        }
        size_t head = 0;
        io_uring_for_each_cqe(&ring, head, cqe)
        {
            /* Check completion event result code */
            if (cqe->res < 0)
            {
                ops_failed++;
            }
        }
        io_uring_cq_advance(&ring, args.oio);

        ops_returned += args.oio;
    }

    /* Cleanup io_uring */
    io_uring_queue_exit(&ring);

    Result_t results;
    results.throughput = calculateThroughputGbps(ops_returned - ops_failed, buffer_size, args.runtime);
    results.op_count = ops_returned - ops_failed;
    results.ops_submitted = ops_submitted;
    results.ops_returned = ops_returned;
    results.ops_failed = ops_failed;
    return results;
}

Result_t _async_liburing_stress(const RuntimeArgs_t &args)
{
    uint64_t ops_submitted = 0, ops_returned = 0, ops_failed = 0;
    int benchmark_iteration = 0;
    double benchmark_throughput = 0;
    uint64_t benchmark_opcount = 0;
    off_t benchmark_offset = 0;
    size_t buffer_size = 1024 * args.blk_size;
    bool isRead = (args.operation == READ);
    bool isRand = (args.opmode == RANDOM);

    struct io_uring ring;
    struct io_uring_sqe *sqe;
    struct io_uring_cqe *cqe;
    struct iovec *iovecs;
    off_t offsets[args.oio];
    char *buffer[args.oio];
    int ret;

    /* Initialize io_uring */
    ret = io_uring_queue_init(1024, &ring, 0);
    if (ret < 0)
    {
        perror(getErrorMessageWithTid(args, "io_uring_queue_init"));
        return return_error();
    }

    /* Initialize and Register buffers */
    iovecs = (iovec *)calloc(args.oio, sizeof(struct iovec));
    for (int i = 0; i < args.oio; i++)
    {
        buffer[i] = (char *)aligned_alloc(1024, buffer_size);
        memset(buffer[i], '0', buffer_size);
        iovecs[i].iov_base = buffer[i];
        iovecs[i].iov_len = buffer_size;
    }

    ret = io_uring_register_buffers(&ring, iovecs, args.oio);
    if (ret)
    {
        fprintf(stderr, "Error registering buffers: %s\n", strerror(-ret));
        return return_error();
    }

    /* Pre-calculate first set of offsets */
    for (int i = 0; i < args.oio; i++)
    {
        offsets[i] = getOffset(args.max_offset, args.read_offset, buffer_size, i, isRand);
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
            for (int i = 0; i < args.oio; i++)
            {
                /* Get a Submission Queue Entry */
                sqe = io_uring_get_sqe(&ring);
                isRead ? io_uring_prep_read_fixed(sqe, args.fd, iovecs[i].iov_base, buffer_size, offsets[i], i) : io_uring_prep_write_fixed(sqe, args.fd, iovecs[i].iov_base, buffer_size, offsets[i], i);
            }

            /* Submit args.oio operations */
            ret = io_uring_submit(&ring);
            ops_submitted += args.oio;

            /* Pre-calculate next set of offsets */
            for (int i = 0; i < args.oio; i++)
            {
                offsets[i] = getOffset(args.max_offset, benchmark_offset, buffer_size, ops_submitted + i, isRand);
            }

            /* Wait for args.oio IO requests to complete */
            ret = io_uring_wait_cqe_nr(&ring, &cqe, args.oio);
            if (ret < 0)
            {
                fprintf(stderr, "Error io_uring_wait_cqe_nr: %s\n", strerror(-ret));
                return return_error();
            }
            size_t head = 0;
            io_uring_for_each_cqe(&ring, head, cqe)
            {
                /* Check completion event result code */
                if (cqe->res < 0)
                {
                    ops_failed++;
                }
            }
            io_uring_cq_advance(&ring, args.oio);

            ops_returned += args.oio;
        }

        Result_t iteration_results;
        iteration_results.throughput = calculateThroughputGbps(ops_returned - ops_failed, buffer_size, 1);
        iteration_results.op_count = ops_returned - ops_failed;
        benchmark_throughput += iteration_results.throughput;
        benchmark_opcount += iteration_results.op_count;

        printIterationStats(args, benchmark_iteration, iteration_results);
        benchmark_iteration++;
    }

    /* Cleanup io_uring */
    io_uring_queue_exit(&ring);

    Result_t results;
    results.throughput = benchmark_throughput / args.runtime;
    results.op_count = benchmark_opcount / args.runtime;

    return results;
}

Result_t async_liburing(const RuntimeArgs_t &args)
{
    Result_t results;
    switch (args.benchmark_type)
    {
    case NORMAL:
        if (geteuid() == 0)
        {
            /* SQE kernel polling only possible with root access */
            if (args.debugInfo)
            {
                std::stringstream msg;
                msg << "TID:" << args.thread_id << " SQE kernel polling enabled" << endl;
                cout << msg.str();
            }
            results = (args.vec_size > 0) ? _async_liburing_vectored_sqe_polling(args) : _async_liburing_fixed_buffer_sqe_polling(args);
        }
        else
        {
            results = (args.vec_size > 0) ? _async_liburing_vectored(args) : _async_liburing_fixed_buffer(args);
        }
        break;
    case STRESS:
        results = _async_liburing_stress(args);
        break;
    default:
        break;
    }

    if (args.debugInfo)
        printOpStats(args, results);
    return results;
}