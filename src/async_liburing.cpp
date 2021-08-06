#include "async_liburing.h"

using namespace std;

Result_t async_liburing(const RuntimeArgs_t& args) {
    size_t buffer_size = 1024 * args.blk_size;
    uint64_t ops_submitted = 0, ops_returned = 0;
    char* buffer = (char *) aligned_alloc(1024, buffer_size);
	memset(buffer, '0', buffer_size);

    struct io_uring ring;
    io_uring_queue_init(QUEUE_DEPTH, &ring, 0);

    struct file_info *fi = (file_info*) malloc(sizeof(*fi) + (sizeof(struct iovec) * args.oio));
    fi->file_sz = buffer_size * args.oio;
    for (int i = 0; i < args.oio; i++) {
        fi->iovecs[i].iov_len = buffer_size;
        fi->iovecs[i].iov_base = buffer;
    }

	double start = getTime();
	while (getTime() - start < RUN_TIME) {
        cout << "Ops returned=" << ops_returned << endl;
        off_t offset =  args.read_offset + (buffer_size * ops_submitted) % _100GB;

        /* Get an SQE */
        struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
        io_uring_prep_readv(sqe, args.fd, fi->iovecs, args.oio, offset);
        io_uring_sqe_set_data(sqe, fi);

        /* Submit the request */
        io_uring_submit(&ring);
        ops_submitted+= args.oio;

        /* Wait for a completion to be available, fetch the data from the readv operation */
        struct io_uring_cqe *cqe;
        int ret = io_uring_wait_cqe(&ring, &cqe);
        if (ret < 0) {
            getErrorMessageWithTid(args, "io_uring_wait_cqe");
            return return_error();
        }
        if (cqe->res < 0) {
            fprintf(stderr, "Async readv failed with code: %d\n", cqe->res);
            return return_error();
        }

        ops_returned+= args.oio;
    }
    cout << "got results" << endl;
    Result_t results;
    results.throughput = calculateThroughputGbps(ops_returned, buffer_size);
    results.op_count = ops_returned;
	results.ops_submitted = ops_submitted;
	results.ops_returned = ops_returned;

	if (args.debugInfo) printOpStats(args, results);
    return results;
}