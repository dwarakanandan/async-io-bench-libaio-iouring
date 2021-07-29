#include "helper.h"

#include <inttypes.h>
#include <sys/syscall.h>
#include <linux/aio_abi.h>

#define MAX_OPS 10
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
    uint64_t ops = 0;
    off_t offsets[MAX_OPS];

	char* buffer = (char *) aligned_alloc(1024, buffer_size);
    memset(buffer, '1', buffer_size);

	if (args.opmode.compare(SEQUENTIAL) == 0) {
        for(int i=0; i < MAX_OPS; i++) {
            offsets[i] = args.read_offset + (i * buffer_size) % _100GB;
        }
    } else {
        for(int i=0; i < MAX_OPS; i++) {
            offsets[i] = args.read_offset + (rand() * buffer_size) % _100GB;
        }
    }

	aio_context_t ctx = 0;
	struct iocb cb[MAX_OPS];
	struct iocb *cbs[MAX_OPS];
	struct io_event events[MAX_OPS];
	int ret;

	ret = io_setup(MAX_OPS, &ctx);
	if (ret < 0) {
		perror("io_setup");
		exit(-1);
	}

	for (size_t i = 0; i < MAX_OPS; i++)
	{
		memset(&(cb[i]), 0, sizeof(cb[i]));
		cb[i].aio_fildes = args.fd;
		cb[i].aio_lio_opcode = IOCB_CMD_PREAD;
		cb[i].aio_buf = (uint64_t)buffer;
		cb[i].aio_offset = offsets[i];
		cb[i].aio_nbytes = buffer_size;
		cbs[i] = &(cb[i]);
	}

	ret = io_submit(ctx, MAX_OPS, cbs);
	cout << "Submitted events: " << ret << endl;
	if (ret < 0) {
		perror("io_submit");
		exit(-1);
	}

	timespec timeout;
	timeout.tv_sec = 5;

	double start = getTime();
    while (getTime() - start < RUN_TIME) {
		ret = io_getevents(ctx, 0, MAX_OPS, events, &timeout);
		cout << "Got events: " << ret << endl;
		if (ret < 0) {
			fprintf(stderr, "io_getevents failed with code: %d\n", ret);
			exit(1);
		}
		ops+=ret;
    }

	for (size_t i = 0; i < ops; i++)
	{
		if (events[i].res < 0) {
			fprintf(stderr, "Error at event: %ld  errcode: %lld\n", i, events[i].res);
			exit(1);
		}
	}

	ret = io_destroy(ctx);
	if (ret < 0) {
		perror("io_destroy");
		exit(-1);
	}

    Result_t results;
    results.throughput = ((ops * buffer_size)/(1024.0*1024*1024 * RUN_TIME));
    results.op_count = ops;

    if (args.debugInfo) printStats(args, results);
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