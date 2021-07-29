#include "async_libaio.h"

using namespace std;

inline int io_setup(unsigned nr, aio_context_t *ctxp) {
	return syscall(__NR_io_setup, nr, ctxp);
}

inline int io_destroy(aio_context_t ctx) {
	return syscall(__NR_io_destroy, ctx);
}

inline int io_submit(aio_context_t ctx, long nr, struct iocb **iocbpp) {
	return syscall(__NR_io_submit, ctx, nr, iocbpp);
}

inline int io_getevents(aio_context_t ctx, long min_nr, long max_nr,
		struct io_event *events, struct timespec *timeout) {
	return syscall(__NR_io_getevents, ctx, min_nr, max_nr, events, timeout);
}

Result_t return_error() {
	Result_t results;
    results.throughput = 0;
    results.op_count = 0;
	return results;
}

Result_t async_libaio(const RuntimeArgs_t& args) {
	size_t buffer_size = 1024 * args.blk_size;
    uint64_t ops_submitted = 0, ops_returned = 0, ops_failed = 0;
	bool isRead = (args.operation.compare(READ) == 0);

	char* buffer = (char *) aligned_alloc(1024, buffer_size);
    memset(buffer, 'A', buffer_size);

	aio_context_t ctx = 0;
	struct iocb cb[ASYNC_OP_BATCH_SIZE];
	struct iocb *cbs[ASYNC_OP_BATCH_SIZE];
	struct io_event events[ASYNC_OP_BATCH_SIZE];
	int ret;

	ret = io_setup(ASYNC_OP_BATCH_SIZE, &ctx);
	if (ret < 0) {
		perror(getErrorMessageWithTid(args, "io_setup"));
		return return_error();
	}

	for (size_t i = 0; i < ASYNC_OP_BATCH_SIZE; i++)
	{
		memset(&(cb[i]), 0, sizeof(cb[i]));
		cb[i].aio_fildes = args.fd;
		cb[i].aio_lio_opcode = isRead? IOCB_CMD_PREAD: IOCB_CMD_PWRITE;
		cb[i].aio_buf = (uint64_t)buffer;
		cb[i].aio_nbytes = buffer_size;
		cbs[i] = &(cb[i]);
	}

	timespec timeout;
	timeout.tv_sec = 1;

	double start = getTime();
    while (getTime() - start < RUN_TIME) {
		for (size_t i = 0; i < ASYNC_OP_BATCH_SIZE; i++)
		{
			cb[i].aio_offset = args.offsets[ops_submitted+i];
		}

		ret = io_submit(ctx, ASYNC_OP_BATCH_SIZE, cbs);
		if (ret < 0) {
			perror(getErrorMessageWithTid(args, "io_submit"));
			return return_error();
		}
		ops_submitted +=ret;

		ret = io_getevents(ctx, ASYNC_OP_BATCH_SIZE, ASYNC_OP_BATCH_SIZE, events, &timeout);
		if (ret < 0) {
			fprintf(stderr, "io_getevents failed with code: %d\n", ret);
			return return_error();
		}
		ops_returned+=ret;

		for (int i = 0; i < ret; i++)
		{
			if (events[i].res < 0) {
				ops_failed++;
			}
		}
    }

	ret = io_destroy(ctx);
	if (ret < 0) {
		perror(getErrorMessageWithTid(args, "io_destroy"));
		return return_error();
	}

    Result_t results;
    results.throughput = calculateThroughputGbps(ops_returned-ops_failed, buffer_size);
    results.op_count = ops_returned-ops_failed;
	results.ops_submitted = ops_submitted;
	results.ops_returned = ops_returned;
	results.ops_failed = ops_failed;

	if (args.debugInfo) printOpStats(args, results);
    return results;
}

RuntimeArgs_t mapUserArgsToRuntimeArgs(int argc, char const *argv[]) {
    RuntimeArgs_t args;
    args.thread_count = 1;
    args.blk_size = 16;
    args.debugInfo = false;
    
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "--file") == 0) {args.filename = argv[i+1];}
        if (strcmp(argv[i], "--bsize") == 0) {args.blk_size = atoi(argv[i+1]);}
        if (strcmp(argv[i], "--threads") == 0) {args.thread_count = atoi(argv[i+1]);}
        if (strcmp(argv[i], "--op") == 0) {args.operation = strcmp(argv[i+1], "read") == 0 ? READ: WRITE;}
        if (strcmp(argv[i], "--mode") == 0) {args.opmode = strcmp(argv[i+1], "seq") == 0 ? SEQUENTIAL: RANDOM;}
        if (strcmp(argv[i], "--debug") == 0) {args.debugInfo = true;}
        if (strcmp(argv[i], "--lib") == 0) {
            args.lib = strcmp(argv[i+1], "syncio") == 0 ? SYNCIO:
                (strcmp(argv[i+1], "libaio") == 0 ? LIBAIO : IOURING);
        }
    }
    return args;
}

int main(int argc, char const *argv[])
{
    RuntimeArgs_t args = mapUserArgsToRuntimeArgs(argc, argv);
    fileOpen(&args);

    runBenchmark(args, async_libaio);

    return 0;
}