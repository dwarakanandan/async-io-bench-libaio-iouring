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

Result_t async_libaio(const RuntimeArgs_t& args) {
	size_t buffer_size = 1024 * args.blk_size;
	uint64_t ops_submitted = 0, ops_returned = 0, ops_failed = 0;
	bool isRead = (args.operation.compare(READ) == 0);
	bool isRand = (args.opmode.compare(RANDOM) == 0);

	char* buffer = (char *) aligned_alloc(1024, buffer_size);
	memset(buffer, '0', buffer_size);

	aio_context_t ctx = 0;
	struct iocb cb[args.oio];
	struct iocb *cbs[args.oio];
	struct io_event events[args.oio];
	int ret;

	ret = io_setup(args.oio, &ctx);
	if (ret < 0) {
		perror(getErrorMessageWithTid(args, "io_setup"));
		return return_error();
	}

	for (int i = 0; i < args.oio; i++)
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
		// Submit args.oio events
		for (int i = 0; i < args.oio; i++) cb[i].aio_offset = getOffset(args.read_offset, buffer_size, ops_submitted+i, isRand);
		ret = io_submit(ctx, args.oio, cbs);
		if (ret < 0) {
			perror(getErrorMessageWithTid(args, "io_submit"));
			return return_error();
		}
		ops_submitted +=ret;

		// Wait for args.oio events to complete
		ret = io_getevents(ctx, args.oio, args.oio, events, &timeout);
		if (ret < 0) {
			fprintf(stderr, "io_getevents failed with code: %d\n", ret);
			return return_error();
		}
		ops_returned+=ret;

		// Check event result codes
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