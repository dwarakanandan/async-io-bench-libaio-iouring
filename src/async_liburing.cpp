#include "async_liburing.h"

using namespace std;

Result_t async_liburing(const RuntimeArgs_t& args) {
    int ret = 0;
    size_t buffer_size = 1024 * args.blk_size;
    uint64_t ops_submitted = 0, ops_returned = 0, ops_failed = 0;
    bool isRead = (args.operation.compare(READ) == 0);
	bool isRand = (args.opmode.compare(RANDOM) == 0);

    char* buffer = (char *) aligned_alloc(1024, buffer_size);
	memset(buffer, '0', buffer_size);

    struct io_uring ring;
    struct io_uring_sqe *sqe;
    struct io_uring_cqe *cqe;
    struct iovec *iovecs;

    iovecs = (iovec*) calloc(args.oio, sizeof(struct iovec));

    ret = io_uring_queue_init(args.oio, &ring, 0);
    if (ret < 0) {
        perror(getErrorMessageWithTid(args, "io_uring_queue_init"));
        return return_error();
    }

	double start = getTime();
	while (getTime() - start < RUN_TIME) {
        /* Prepare args.oio IO requests */
        for (int i = 0; i < args.oio; i++)
        {
            /* Get a Submission Queue Entry */
            sqe = io_uring_get_sqe(&ring);
            if (sqe == NULL) {
                fprintf(stderr, "io_uring_get_sqe failed\n");
                return return_error();
            }
            iovecs[i].iov_base = buffer;
            iovecs[i].iov_len = buffer_size;

            off_t offset = getOffset(args.read_offset, buffer_size, ops_submitted+i, isRand);
            isRead? io_uring_prep_readv(sqe, args.fd, &iovecs[i], 1, offset):
                io_uring_prep_writev(sqe, args.fd, &iovecs[i], 1, offset);
        }

        /* Submit the requests */
        ret = io_uring_submit(&ring);
        ops_submitted+= ret;

        /* Wait for args.oio IO requests to complete */
        for (int i = 0; i < args.oio; i++) {
            ret = io_uring_wait_cqe(&ring, &cqe);
            if (ret < 0) {
                perror(getErrorMessageWithTid(args, "io_uring_wait_cqe"));
                return return_error();
            }

            /* Check completion event result code */
            if (cqe->res < 0) {
                ops_failed++;
            }
            io_uring_cqe_seen(&ring, cqe);
        }

        ops_returned+= args.oio;
    }

    io_uring_queue_exit(&ring);

    Result_t results;
    results.throughput = calculateThroughputGbps(ops_returned-ops_failed, buffer_size);
    results.op_count = ops_returned-ops_failed;
	results.ops_submitted = ops_submitted;
	results.ops_returned = ops_returned;
	results.ops_failed = ops_failed;

	if (args.debugInfo) printOpStats(args, results);
    return results;
}