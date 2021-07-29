#include "helper.h"

using namespace std;

void syncioRead(int fd, char* buffer, size_t buffer_size, off_t offsets[], uint64_t* ops) {
    double start = getTime();
    while (getTime() - start < RUN_TIME) {
        ssize_t readCount = pread(fd, buffer, buffer_size, offsets[*ops]);
        if(readCount < 0) {
            perror("Read error");
            return;
        }
        (*ops)++;
    }
}

void syncioWrite(int fd, char* buffer, size_t buffer_size, off_t offsets[], uint64_t* ops) {
    double start = getTime();
    while (getTime() - start < RUN_TIME) {
        ssize_t writeCount = pwrite(fd, buffer, buffer_size, offsets[*ops]);
        if(writeCount < 0) {
            perror("Write error");
            return;
        }
        (*ops)++;
    }
}

Result_t syncio(const RuntimeArgs_t& args) {
    size_t buffer_size = 1024 * args.blk_size;
    uint64_t ops = 0;

    char* buffer = (char *) aligned_alloc(1024, buffer_size);
    memset(buffer, 'A', buffer_size);

    if (args.operation.compare(READ) == 0) {
        syncioRead(args.fd, buffer, buffer_size, args.offsets, &ops);
    } else {
        syncioWrite(args.fd, buffer, buffer_size, args.offsets, &ops);
    }

    Result_t results;
    results.throughput = calculateThroughputGbps(ops, buffer_size);
    results.op_count = ops;

    if (args.debugInfo) printStats(args, results);
    return results;
}

int main(int argc, char const *argv[])
{
    if (argc < 3 ) {
        cout << "syncio --file <file> --threads <threads> --bsize <block_size_kB> --op <r|w> --mode <s|r> --debug" << endl;
        exit(1);
    }

    RuntimeArgs_t args = mapUserArgsToRuntimeArgs(argc, argv);
    fileOpen(&args);

    runBenchmark(args, syncio);

    return 0;
}
