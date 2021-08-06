#include "async_liburing.h"

using namespace std;

Result_t async_liburing(const RuntimeArgs_t& args) {
    int ret = 0;
    size_t buffer_size = 1024 * args.blk_size;
    uint64_t ops_submitted = 0, ops_returned = 0;
    bool isRead = (args.operation.compare(READ) == 0);
	bool isRand = (args.opmode.compare(RANDOM) == 0);

    char* buffer = (char *) aligned_alloc(1024, buffer_size);
	memset(buffer, '0', buffer_size);

    struct io_uring ring;
    ret = io_uring_queue_init(QUEUE_DEPTH, &ring, 0);
    if (ret < 0) {
        perror(getErrorMessageWithTid(args, "io_uring_queue_init"));
        return return_error();
    }

	double start = getTime();
	while (getTime() - start < RUN_TIME) {
        cout << "ops_submitted: " << ops_submitted << endl;
        struct io_uring_sqe *sqe[args.oio];
        for (int i = 0; i < args.oio; i++)
        {
            /* Get a Submission Queue Entry */
            sqe[i] = io_uring_get_sqe(&ring);
            if (sqe[i] == NULL) {
                fprintf(stderr, "io_uring_get_sqe failed\n");
                return return_error();
            }
            off_t offset = getOffset(args.read_offset, buffer_size, ops_submitted+i, isRand);
            io_uring_prep_read(sqe[i], args.fd, buffer, buffer_size, offset);
        }

        /* Submit the requests */
        ret = io_uring_submit(&ring);
        ops_submitted+= ret;

        /* Wait for args.oio requests to complete */
        for (int i = 0; i < args.oio; i++) {
            struct io_uring_cqe *cqe;
            ret = io_uring_wait_cqe(&ring, &cqe);
            if (ret < 0) {
                perror(getErrorMessageWithTid(args, "io_uring_wait_cqe"));
                return return_error();
            }

            /* Check completion event result code */
            if (cqe->res < 0) {
                fprintf(stderr, "Async read failed with code: %d\n", cqe->res);
                return return_error();
            }
            io_uring_cqe_seen(&ring, cqe);
        }

        ops_returned+= args.oio;
    }

    io_uring_queue_exit(&ring);

    Result_t results;
    results.throughput = calculateThroughputGbps(ops_returned, buffer_size);
    results.op_count = ops_returned;
	results.ops_submitted = ops_submitted;
	results.ops_returned = ops_returned;

	if (args.debugInfo) printOpStats(args, results);
    return results;
}