#include "helper.h"

using namespace std;

#define MAX_OPS 200000
#define RUN_TIME 2

void syncioRead(double start, int fd, char* buffer, size_t buffer_size, off_t offsets[], uint64_t* ops) {
    while (gettime() - start < RUN_TIME) {
        ssize_t readCount = pread(fd, buffer, buffer_size, offsets[*ops]);
        if(readCount < 0) {
            perror("Read error");
            return;
        }
        (*ops)++;
    }
}

void syncioWrite(double start, int fd, char* buffer, size_t buffer_size, off_t offsets[], uint64_t* ops) {
    while (gettime() - start < RUN_TIME) {
        ssize_t writeCount = pwrite(fd, buffer, buffer_size, offsets[*ops]);
        if(writeCount < 0) {
            perror("Write error");
            return;
        }
        (*ops)++;
    }
}

double syncio(const RuntimeArgs_t& args) {
    size_t buffer_size = 1024 * args.blk_size;
    uint64_t ops = 0;
    off_t offsets[MAX_OPS];
    
    if (args.opmode.compare(SEQUENTIAL) == 0) {
        for(int i=0; i < MAX_OPS; i++) {
            offsets[i] = args.read_offset + (i * buffer_size) % _100GB;
        }
    } else {
        for(int i=0; i < MAX_OPS; i++) {
            offsets[i] = args.read_offset + (rand() * buffer_size) % _100GB;
        }
    }

    char* buffer = (char *) aligned_alloc(1024, buffer_size);
    memset(buffer, '1', buffer_size);

    double start = gettime();
    if (args.operation.compare(READ) == 0) {
        syncioRead(start, args.fd, buffer, buffer_size, offsets, &ops);
    } else {
        syncioWrite(start, args.fd, buffer, buffer_size, offsets, &ops);
    }
    double throughput = ((ops * buffer_size)/(1024.0*1024*1024 * RUN_TIME));

    printStats(args, throughput, ops);
    return throughput;
}

int main(int argc, char const *argv[])
{
    if (argc < 3 ) {
        cout << "syncio --file <file> --threads <threads> --bsize <block_size_kB> --op <r|w> --mode <s|r> --debug" << endl;
        exit(1);
    }

    srand(time(NULL));

    RuntimeArgs_t args = mapUserArgsToRuntimeArgs(argc, argv);
    fileOpen(args);

    runBenchmark(args, syncio);

    return 0;
}
