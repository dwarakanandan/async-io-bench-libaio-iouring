#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"

#include "msg_rate.h"

using namespace std::chrono_literals;
using std::condition_variable;
using std::mutex;
using std::queue;
using std::thread;
using std::unique_lock;
using std::vector;

struct IO_request_t
{
    off_t offset;
    RuntimeArgs_t args;
};

class WorkQueue
{
    condition_variable work_available;
    mutex work_mutex;
    queue<IO_request_t> work;

public:
    void push_work(IO_request_t io_request)
    {
        unique_lock<mutex> lock(work_mutex);

        bool was_empty = work.empty();
        work.push(io_request);

        lock.unlock();

        if (was_empty)
        {
            work_available.notify_one();
        }
    }

    IO_request_t wait_and_pop()
    {
        unique_lock<mutex> lock(work_mutex);
        double start = getTime();
        while (work.empty())
        {
            if (getTime() - start > 1)
            {
                /* 1 sec consumer wait timeout*/
                IO_request_t tmp;
                tmp.offset = -1;
                return tmp;
            }
            work_available.wait_for(lock, std::chrono::seconds(1));
        }

        IO_request_t tmp = work.front();
        work.pop();
        return tmp;
    }
};

void _io_request_producer(WorkQueue &work_queue, const RuntimeArgs_t &args)
{
    size_t buffer_size = 1024 * args.blk_size;
    uint64_t iter = 0;
    double start = getTime();
    while (getTime() - start < args.runtime)
    {
        IO_request_t io_request;
        io_request.args = args;
        io_request.offset = getOffset(args.max_offset, args.read_offset, buffer_size, iter++, true);
        work_queue.push_work(io_request);
        if (args.message_interval != 0)
        {
            std::this_thread::sleep_for(std::chrono::nanoseconds(args.message_interval));
        }
    }
}

void _io_request_handler(WorkQueue &work_queue)
{
    uint64_t total_requests_handled = 0;
    int outstanding_requests = 0;
    io_uring ring;
    struct iovec *iovecs;
    size_t buffer_size;
    RuntimeArgs_t args;
    ssize_t rval;

    char *buffer;

    double start = getTime();
    while (getTime() - start < 1)
    {
        IO_request_t io_request = work_queue.wait_and_pop();
        args = io_request.args;
        buffer_size = 1024 * args.blk_size;
        outstanding_requests++;
        if (io_request.offset == -1)
            break;

        if (total_requests_handled == 0)
        {
            if (args.lib == IOURING)
            {
                int ret = io_uring_queue_init(1024, &ring, 0);
                if (ret < 0)
                {
                    perror("io_uring_queue_init");
                    return;
                }
                /* Initialize and Register buffers */
                char *buffer[args.oio];

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
                    return;
                }
            }
            if (args.lib == SYNCIO)
            {
                buffer = (char *)aligned_alloc(1024, buffer_size);
                memset(buffer, '0', buffer_size);
            }
        }
        switch (args.lib)
        {
        case SYNCIO:
            rval = (args.operation == READ) ? pread(args.fd, buffer, buffer_size, io_request.offset) : pwrite(args.fd, buffer, buffer_size, io_request.offset);
            if (rval < 0)
            {
                fprintf(stderr, "IO request failed: %s\n", strerror(-rval));
                return;
            }
            break;
        case LIBAIO:
            break;
        case IOURING:
            struct io_uring_sqe *sqe;
            sqe = io_uring_get_sqe(&ring);
            int submit_index = outstanding_requests - 1;
            (args.operation == READ) ? io_uring_prep_read_fixed(sqe, args.fd, iovecs[submit_index].iov_base, buffer_size, io_request.offset, submit_index) : io_uring_prep_write_fixed(sqe, args.fd, iovecs[submit_index].iov_base, buffer_size, io_request.offset, submit_index);
            if (outstanding_requests == args.oio)
            {
                struct io_uring_cqe *cqe;
                int ret = io_uring_submit(&ring);
                /* Wait for args.oio IO requests to complete */
                ret = io_uring_wait_cqe_nr(&ring, &cqe, outstanding_requests);
                if (ret < 0)
                {
                    fprintf(stderr, "Error io_uring_wait_cqe_nr: %s\n", strerror(-ret));
                    return;
                }
                size_t head = 0;
                io_uring_for_each_cqe(&ring, head, cqe)
                {
                    /* Check completion event result code */
                    if (cqe->res < 0)
                    {
                        fprintf(stderr, "IO request failed: %s\n", strerror(-(cqe->res)));
                        return;
                    }
                }
                io_uring_cq_advance(&ring, outstanding_requests);
            }
            break;
        }
        if (outstanding_requests == args.oio)
            outstanding_requests = 0;
        total_requests_handled++;
    }

    std::cout << std::fixed
              << libToString(args.lib) << " "
              << operationToString(args.operation) << " "
              << opmodeToString(args.opmode) << " "
              << "BLKS_KB:" << args.blk_size << " "
              << "OIO:" << ((args.lib == SYNCIO) ? 1 : args.oio) << " "
              << "OP_CNT:" << total_requests_handled << " "
              << "TPUT_GBPS:" << calculateThroughputGbps(total_requests_handled, buffer_size, args.runtime) << std::endl;
}

void runMessageRateBenchmark(RuntimeArgs_t &args)
{
    WorkQueue work_queue;

    std::thread io_request_producer([&]()
                                    { _io_request_producer(work_queue, args); });

    std::thread io_request_handler([&]()
                                   { _io_request_handler(work_queue); });

    io_request_producer.join();
    io_request_handler.join();
}