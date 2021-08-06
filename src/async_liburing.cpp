#include "async_liburing.h"

using namespace std;

struct file_info {
    off_t file_sz;
    struct iovec iovecs[];
};

Result_t async_liburing(const RuntimeArgs_t& args) {
    size_t buffer_size = 1024 * args.blk_size;

    char* buffer = (char *) aligned_alloc(1024, buffer_size);
	memset(buffer, '0', buffer_size);

    struct io_uring ring;
    io_uring_queue_init(QUEUE_DEPTH, &ring, 0);

    int blocks = 1;

    struct file_info *fi = (file_info*) malloc(sizeof(*fi) + (sizeof(struct iovec) * blocks));

    fi->iovecs[0].iov_len = 1024;
    fi->iovecs[0].iov_base = buffer;

    fi->file_sz = 1024;

    /* Get an SQE */
    struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
    /* Setup a readv operation */
    io_uring_prep_readv(sqe, args.fd, fi->iovecs, blocks, 1024);
    /* Set user data */
    io_uring_sqe_set_data(sqe, fi);
    /* Finally, submit the request */
    io_uring_submit(&ring);

    struct io_uring_cqe *cqe;
    int ret = io_uring_wait_cqe(&ring, &cqe);
        if (ret < 0) {
        perror("io_uring_wait_cqe");
        return return_error();
    }
    if (cqe->res < 0) {
        fprintf(stderr, "Async readv failed.\n");
        return return_error();
    }

    fi = (file_info*) io_uring_cqe_get_data(cqe);

    cout << fi->file_sz << std::endl;
    char* obuf = (char*) fi->iovecs[0].iov_base;
    for (int i = 0; i < 10; i++)
    {
        printf("%d=%d:%c\n", i, obuf[i], obuf[i]);
    }
    
    return return_error();
}