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
    size_t buffer_size = 1024 * args.blk_size;;
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

void runBenchmark(RuntimeArgs_t& userArgs) {
    //int blk_size = (strcmp(userArgs.opmode, SEQUENTIAL) == 0) ? 102400: userArgs.blk_size;
    int blk_size = userArgs.blk_size;
    std::vector<std::future<double>> threads;
    for (int i = 0; i < userArgs.thread_count; ++i) {
        RuntimeArgs_t args;
        args.thread_id = i;
        args.blk_size = blk_size;
        args.fd = userArgs.fd;
        args.debugInfo = userArgs.debugInfo;
        args.read_offset = (_100GB * i) % MAX_READ_OFFSET;
        args.operation = userArgs.operation;
        args.opmode = userArgs.opmode;
        threads.push_back(std::async(syncio, args));
    }
    double totalThroughput = 0;
    for (auto& t : threads) {
        totalThroughput += t.get();
    }
    cout << userArgs.operation << " " <<  userArgs.opmode << " BLK_SIZE: " << blk_size <<
            " kB   Throughput = " << totalThroughput << " GB/s" << endl << endl;
}

int main(int argc, char const *argv[])
{
    if (argc < 3 ) {
        cout << "syncio --file <file> --threads <threads> --bsize <block_size_kB> --op <r|w> --mode <s|r> --debug" << endl;
        exit(1);
    }

    srand(time(NULL));

    const char* filename;
    RuntimeArgs_t args;
    args.thread_count = 1;
    args.blk_size = 16;
    args.debugInfo = false;
    
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "--file") == 0) {filename = argv[i+1];}
        if (strcmp(argv[i], "--bsize") == 0) {args.blk_size = atoi(argv[i+1]);}
        if (strcmp(argv[i], "--threads") == 0) {args.thread_count = atoi(argv[i+1]);}
        if (strcmp(argv[i], "--op") == 0) {args.operation = strcmp(argv[i+1], "r") == 0 ? READ: WRITE;}
        if (strcmp(argv[i], "--mode") == 0) {args.opmode = strcmp(argv[i+1], "s") == 0 ? SEQUENTIAL: RANDOM;}
        if (strcmp(argv[i], "--debug") == 0) {args.debugInfo = true;}
    }

    args.fd = open(filename, O_RDWR | O_CREAT | O_DIRECT, S_IRUSR | S_IWUSR);
    if (args.fd < 0) {
        perror("Open error");
        return -1;
    }

    runBenchmark(args);

    return 0;
}
