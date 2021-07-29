#include "helper.h"

#include <inttypes.h>
#include <sys/syscall.h>
#include <linux/aio_abi.h>

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
	aio_context_t ctx;
	struct iocb cb;
	struct iocb *cbs[1];
	struct io_event events[1];
	int ret;

	int buffer_size = 100;
	char* buffer = (char *) aligned_alloc(1024, buffer_size);
    memset(buffer, '1', buffer_size);

	ctx = 0;

	ret = io_setup(128, &ctx);
	if (ret < 0) {
		perror("io_setup");
		exit(-1);
	}

	memset(&cb, 0, sizeof(cb));
	cb.aio_fildes = args.fd;
	cb.aio_lio_opcode = IOCB_CMD_PREAD;
	cb.aio_buf = (uint64_t)buffer;
	cb.aio_offset = 0;
	cb.aio_nbytes = 100;

	cbs[0] = &cb;

	ret = io_submit(ctx, 1, cbs);
	if (ret != 1) {
		if (ret < 0) perror("io_submit");
		else fprintf(stderr, "io_submit failed\n");
		exit(-1);
	}

	ret = io_getevents(ctx, 1, 1, events, NULL);
	printf("events: %d\n", ret);

	for (size_t i = 0; i < 10; i++)
    {
        printf("%lu: %d %c\n", i, buffer[i], buffer[i]);
    }

	ret = io_destroy(ctx);
	if (ret < 0) {
		perror("io_destroy");
		exit(-1);
	}

	Result_t results;
    results.throughput = 1;
    results.op_count = 1;

	return results;
}

int main(int argc, char const *argv[])
{
    if (argc < 3 ) {
        cout << "async_libaio --file <file> --threads <threads> --bsize <block_size_kB> --op <r|w> --mode <s|r> --debug" << endl;
        exit(1);
    }

    srand(time(NULL));

    RuntimeArgs_t args = mapUserArgsToRuntimeArgs(argc, argv);
    fileOpen(&args);

    runBenchmark(args, async_libaio);

    return 0;
}