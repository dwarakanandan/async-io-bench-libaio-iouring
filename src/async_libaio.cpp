#include "async_libaio.h"

using namespace std;

Result_t _async_libaio(const RuntimeArgs_t& args, int libaio_opcode) {
	size_t buffer_size = 1024 * args.blk_size;
	uint64_t ops_submitted = 0, ops_returned = 0, ops_failed = 0;
	bool isRand = (args.opmode == RANDOM);

	aio_context_t ctx = 0;
	struct iocb cb[args.oio];
	struct iocb *cbs[args.oio];
	struct io_event events[args.oio];
	timespec timeout;
	char* buffer[args.oio];
	off_t offsets[args.oio];
	int ret;

	for (int i = 0; i < args.oio; i++)
	{
		buffer[i] = (char *) aligned_alloc(1024, buffer_size);
	    memset(buffer[i], '0', buffer_size);

		/* Pre-calculate first set of offsets */
        offsets[i] = getOffset(args.max_offset, args.read_offset, buffer_size, i, isRand);

		memset(&(cb[i]), 0, sizeof(cb[i]));
		cb[i].aio_fildes = args.fd;
		cb[i].aio_lio_opcode = libaio_opcode;
		cb[i].aio_buf = (uint64_t) buffer[i];
		cb[i].aio_nbytes = buffer_size;
		cbs[i] = &(cb[i]);
	}

	/* Initialize libaio */
	ret = io_setup(args.oio, &ctx);
	if (ret < 0) {
		perror(getErrorMessageWithTid(args, "io_setup"));
		return return_error();
	}

	double start = getTime();
	while (getTime() - start < RUN_TIME) {
		/* Prepare args.oio requests */
		for (int i = 0; i < args.oio; i++) cb[i].aio_offset = offsets[i];
		
		/* Submit args.oio requests */
		ret = io_submit(ctx, args.oio, cbs);
		if (ret < 0) {
			perror(getErrorMessageWithTid(args, "io_submit"));
			return return_error();
		}
		ops_submitted +=ret;

		/* Pre-calculate next set of offsets */
        for (int i = 0; i < args.oio; i++) {
            offsets[i] = getOffset(args.max_offset, args.read_offset, buffer_size, ops_submitted+i, isRand);
        }

		/* Wait for args.oio requests to complete */
		timeout.tv_sec = 1;
		ret = io_getevents(ctx, args.oio, args.oio, events, &timeout);
		if (ret < 0) {
			fprintf(stderr, "io_getevents failed with code: %d\n", ret);
			return return_error();
		}
		ops_returned+=ret;

		/* Check completion event result code */
		for (int i = 0; i < ret; i++)
		{
			if (events[i].res < 0) {
				ops_failed++;
			}
		}
	}

	/* Cleanup libaio */
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
	return results;
}

Result_t async_libaio(const RuntimeArgs_t& args) {
    Result_t results = (args.operation == READ) ?
		_async_libaio(args, IOCB_CMD_PREAD) :
		_async_libaio(args, IOCB_CMD_PWRITE);

	if (args.debugInfo) printOpStats(args, results);
    return results;
}