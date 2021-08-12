#include "async_liburing.h"

using namespace std;

Result_t async_liburing_read(const RuntimeArgs_t& args) {
    size_t buffer_size = 1024 * args.blk_size;
    uint64_t ops_submitted = 0, ops_returned = 0, ops_failed = 0;
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

        offsets[i] = getOffset(args.read_offset, buffer_size, i, isRand);

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
        for (int i = 0; i < args.oio; i++)
        {
            /* Get a Submission Queue Entry */
            sqe = io_uring_get_sqe(&ring);
            if (sqe == NULL) {
                fprintf(stderr, "io_uring_get_sqe failed\n");
                return return_error();
            }
            io_uring_prep_readv(sqe, args.fd, &iovecs[i], 1, offsets[i]);
        }

        /* Submit args.oio operations */
        ret = io_uring_submit(&ring);
        ops_submitted+= args.oio;

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

Result_t async_liburing_write(const RuntimeArgs_t& args) {
    return return_error();
}

Result_t async_liburing(const RuntimeArgs_t& args) {
    Result_t results = (args.operation == READ) ? async_liburing_read(args): async_liburing_write(args);

	if (args.debugInfo) printOpStats(args, results);
    return results;
}