#include "async_liburing.h"

using namespace std;

Result_t _async_liburing(const RuntimeArgs_t& args)
{
    size_t buffer_size = 1024 * args.blk_size;
    uint64_t ops_submitted = 0, ops_returned = 0, ops_failed = 0;
    bool isRead = (args.operation == READ);
	bool isRand = (args.opmode == RANDOM);

    struct io_uring ring;
    struct io_uring_sqe *sqe;
    struct io_uring_cqe *cqe;
    struct iovec *iovecs;
    __kernel_timespec timeout;
    sigset_t sigset;
    char* buffer[args.oio];
    off_t offsets[args.oio];
    int ret;

    iovecs = (iovec*) calloc(args.oio, sizeof(struct iovec));
    for (int i = 0; i < args.oio; i++)
    {
        buffer[i] = (char *) aligned_alloc(1024, buffer_size);
	    memset(buffer[i], '0', buffer_size);

        /* Pre-calculate first set of offsets */
        offsets[i] = getOffset(args.max_offset, args.read_offset, buffer_size, i, isRand);

        iovecs[i].iov_base = buffer[i];
        iovecs[i].iov_len = buffer_size;
    }

    /* Initialize io_uring */
    ret = io_uring_queue_init(1024, &ring, 0);
    if (ret < 0) {
        perror(getErrorMessageWithTid(args, "io_uring_queue_init"));
        return return_error();
    }

    double start = getTime();
	while (getTime() - start < RUN_TIME) {
        for (int i = 0; i < args.oio; i++) {
            /* Get a Submission Queue Entry */
            sqe = io_uring_get_sqe(&ring);
            isRead ? io_uring_prep_readv(sqe, args.fd, &iovecs[i], 1, offsets[i]) :
                io_uring_prep_writev(sqe, args.fd, &iovecs[i], 1, offsets[i]);
        }

        /* Submit args.oio operations */
        ret = io_uring_submit(&ring);
        ops_submitted+= args.oio;

        /* Pre-calculate next set of offsets */
        for (int i = 0; i < args.oio; i++) {
            offsets[i] = getOffset(args.max_offset, args.read_offset, buffer_size, ops_submitted+i, isRand);
        }

        /* Wait for args.oio IO requests to complete */
        timeout.tv_sec = 1;
        ret = io_uring_wait_cqes(&ring, &cqe, args.oio, &timeout, &sigset);
        if (ret < 0) {
            perror(getErrorMessageWithTid(args, "io_uring_wait_cqe"));
            return return_error();
        }

        /* Check completion event result code */
        if (cqe->res < 0) {
            ops_failed+= args.oio;
        }
        io_uring_cq_advance(&ring, args.oio);

        ops_returned+= args.oio;
    }

    /* Cleanup io_uring */
    io_uring_queue_exit(&ring);

    Result_t results;
    results.throughput = calculateThroughputGbps(ops_returned-ops_failed, buffer_size);
    results.op_count = ops_returned-ops_failed;
	results.ops_submitted = ops_submitted;
	results.ops_returned = ops_returned;
	results.ops_failed = ops_failed;
    return results;
}

Result_t _async_liburing_fixed_buffer(const RuntimeArgs_t& args)
{
    size_t buffer_size = 1024 * args.blk_size;
    uint64_t ops_submitted = 0, ops_returned = 0, ops_failed = 0;
    bool isRead = (args.operation == READ);
	bool isRand = (args.opmode == RANDOM);

    struct io_uring ring;
    struct io_uring_sqe *sqe;
    struct io_uring_cqe *cqe;
    __kernel_timespec timeout;
    off_t offsets[args.oio];
    sigset_t sigset;
    int ret;

    for (int i = 0; i < args.oio; i++)
    {
        /* Pre-calculate first set of offsets */
        offsets[i] = getOffset(args.max_offset, args.read_offset, buffer_size, i, isRand);
    }

    /* Initialize io_uring */
    ret = io_uring_queue_init(1024, &ring, 0);
    if (ret < 0) {
        perror(getErrorMessageWithTid(args, "io_uring_queue_init"));
        return return_error();
    }

    struct iovec iov[args.oio];
    for (int i = 0; i < args.oio; i++) {
        iov[i].iov_base = (char *) aligned_alloc(1024, buffer_size);
        iov[i].iov_len = buffer_size;
    }

    ret = io_uring_register_buffers(&ring, iov, args.oio);
    if (ret) {
        fprintf(stderr, "Error registering buffers: %s\n", strerror(-ret));
        return return_error();
    }

    double start = getTime();
	while (getTime() - start < RUN_TIME) {
        for (int i = 0; i < args.oio; i++) {
            /* Get a Submission Queue Entry */
            sqe = io_uring_get_sqe(&ring);
            isRead ? io_uring_prep_read_fixed(sqe, args.fd, iov[i].iov_base, buffer_size, offsets[i], 0) :
                io_uring_prep_write_fixed(sqe, args.fd, iov[i].iov_base, buffer_size, offsets[i], 0);
        }

        /* Submit args.oio operations */
        ret = io_uring_submit(&ring);
        ops_submitted+= args.oio;

        /* Pre-calculate next set of offsets */
        for (int i = 0; i < args.oio; i++) {
            offsets[i] = getOffset(args.max_offset, args.read_offset, buffer_size, ops_submitted+i, isRand);
        }

        /* Wait for args.oio IO requests to complete */
        // for (int i = 0; i < args.oio; i++) {
        //     timeout.tv_sec = 1;
        //     ret = io_uring_wait_cqe_timeout(&ring, &cqe, &timeout);
        //     if (ret < 0) {
        //         perror(getErrorMessageWithTid(args, "io_uring_wait_cqe"));
        //         return return_error();
        //     }

        //     /* Check completion event result code */
        //     if (cqe->res < 0) {
        //         ops_failed++;
        //     }
        //     io_uring_cqe_seen(&ring, cqe);
        // }
        timeout.tv_sec = 1;
        ret = io_uring_wait_cqes(&ring, &cqe, args.oio, &timeout, &sigset);
        cout << ret << endl;
        if (ret < 0) {
            perror(getErrorMessageWithTid(args, "io_uring_wait_cqe"));
            return return_error();
        }

        // /* Check completion event result code */
        // if (cqe->res < 0) {
        //     ops_failed+= args.oio;
        // }
        io_uring_cq_advance(&ring, args.oio);

        ops_returned+= args.oio;
    }

    /* Cleanup io_uring */
    io_uring_queue_exit(&ring);

    Result_t results;
    results.throughput = calculateThroughputGbps(ops_returned-ops_failed, buffer_size);
    results.op_count = ops_returned-ops_failed;
	results.ops_submitted = ops_submitted;
	results.ops_returned = ops_returned;
	results.ops_failed = ops_failed;
    return results;
}

Result_t async_liburing(const RuntimeArgs_t& args) {
    Result_t results = _async_liburing_fixed_buffer(args);
	if (args.debugInfo) printOpStats(args, results);
    return results;
}