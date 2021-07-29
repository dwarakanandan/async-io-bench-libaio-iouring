#include "helper.h"

using namespace std;

const std::string SEQUENTIAL = "SEQUENTIAL";
const std::string RANDOM = "RANDOM";

const std::string READ = "READ";
const std::string WRITE = "WRITE";

void printStats(const RuntimeArgs_t& args, double throughput, uint64_t ops) {
    std::stringstream stats;
    stats << "TID:" << args.thread_id
        << " offset:" << args.read_offset / _1GB << "GB"
        << " ops:" << ops
        << " throughput:" << throughput << " GB/s" << endl;
    if (args.debugInfo) cout << stats.str();
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
        if (strcmp(argv[i], "--op") == 0) {args.operation = strcmp(argv[i+1], "r") == 0 ? READ: WRITE;}
        if (strcmp(argv[i], "--mode") == 0) {args.opmode = strcmp(argv[i+1], "s") == 0 ? SEQUENTIAL: RANDOM;}
        if (strcmp(argv[i], "--debug") == 0) {args.debugInfo = true;}
    }
    return args;
}

void fileOpen(RuntimeArgs_t args) {
    args.fd = open(args.filename.c_str(), O_RDWR | O_CREAT | O_DIRECT, S_IRUSR | S_IWUSR);
    if (args.fd < 0) {
        perror("Open error");
        exit(-1);
    }
}

void runBenchmark(RuntimeArgs_t& userArgs, double (*benchmarkFunction)(const RuntimeArgs_t& args)) {
    // Force block size for sequential operations to 100 MB irrespective of user selection
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
        threads.push_back(std::async(benchmarkFunction, args));
    }
    double totalThroughput = 0;
    for (auto& t : threads) {
        totalThroughput += t.get();
    }
    cout << userArgs.operation << " " <<  userArgs.opmode << " BLK_SIZE: " << blk_size <<
            " kB   Throughput = " << totalThroughput << " GB/s" << endl << endl;
}