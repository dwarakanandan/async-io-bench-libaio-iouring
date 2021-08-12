#include "helper.h"
#include "sync_io.h"
#include "async_libaio.h"
#include "async_liburing.h"

#include <future>
#include <vector>

using namespace std;

void fileNameCheck(int argc, char const *argv[]) {
    bool hasFileName = false;
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "--file") == 0) {
            hasFileName = true;
        }
    }

    std::stringstream helpstr;
    helpstr << "benchmark "
        << "--file <file_name> "
        << "--threads <thread_count> "
        << "--bsize <block_size_kb> "
        << "--op <read|write> "
        << "--mode <seq|rand> "
        << "--lib <syncio|libaio|liburing> "
        << "--oio <outstanding_io_count> "
        << "--debug (show_debug) " << endl;

    if(!hasFileName) {
        cout << helpstr.str();
        exit(-1);
    }
}

RuntimeArgs_t getDefaultArgs() {
    RuntimeArgs_t args;
    args.thread_count = 1;
    args.blk_size = 16;
    args.debugInfo = false;
    args.operation = READ;
    args.opmode = SEQUENTIAL;
    args.oio = 10;
    args.lib = SYNCIO;
    return args;
}

RuntimeArgs_t mapUserArgsToRuntimeArgs(int argc, char const *argv[]) {
    RuntimeArgs_t args = getDefaultArgs();
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "--file") == 0) {args.filename = argv[i+1];}
        if (strcmp(argv[i], "--bsize") == 0) {args.blk_size = atoi(argv[i+1]);}
        if (strcmp(argv[i], "--threads") == 0) {args.thread_count = atoi(argv[i+1]);}
        if (strcmp(argv[i], "--op") == 0) {args.operation = strcmp(argv[i+1], "read") == 0 ? READ: WRITE;}
        if (strcmp(argv[i], "--mode") == 0) {args.opmode = strcmp(argv[i+1], "seq") == 0 ? SEQUENTIAL: RANDOM;}
        if (strcmp(argv[i], "--oio") == 0) {args.oio = atoi(argv[i+1]);}
        if (strcmp(argv[i], "--debug") == 0) {args.debugInfo = true;}
        if (strcmp(argv[i], "--lib") == 0) {
            args.lib = strcmp(argv[i+1], "syncio") == 0 ? SYNCIO:
                (strcmp(argv[i+1], "libaio") == 0 ? LIBAIO : IOURING);
        }
    }
    return args;
}

void runBenchmark(RuntimeArgs_t& userArgs, Result_t (*benchmarkFunction)(const RuntimeArgs_t& args)) {
    std::vector<std::future<Result_t>> threads;
    for (int i = 1; i <= userArgs.thread_count; ++i) {
        RuntimeArgs_t args;
        args.thread_id = i;
        args.blk_size = userArgs.blk_size;
        args.fd = userArgs.fd;
        args.debugInfo = userArgs.debugInfo;
        args.read_offset = (_100GB * i) % MAX_READ_OFFSET;
        args.operation = userArgs.operation;
        args.opmode = userArgs.opmode;
        args.oio = userArgs.oio;
        threads.push_back(std::async(benchmarkFunction, args));
    }

    double totalThroughput = 0;
    uint64_t totalOps = 0;
    for (auto& t : threads) {
        auto results = t.get();
        totalThroughput += results.throughput;
        totalOps += results.op_count;
    }

    int oioPrint = (userArgs.lib == SYNCIO) ? 1 : userArgs.oio;

    cout << std::fixed
        << userArgs.lib << " "
        << userArgs.operation.substr(0,1) << " " <<  userArgs.opmode.substr(0,1) << " "
        << "BLK_SIZE_KB:" << userArgs.blk_size << " "
        << "OIO:" << oioPrint << " "
        << "OP_COUNT:" << totalOps << " "
        << "THROUGHPUT_GBPS:" << totalThroughput << endl;
}

int main(int argc, char const *argv[])
{
    fileNameCheck(argc, argv);
    srand(time(NULL));

    RuntimeArgs_t args = mapUserArgsToRuntimeArgs(argc, argv);
    fileOpen(&args);

    switch(args.lib) {
        case SYNCIO: runBenchmark(args, syncio); break;
        case LIBAIO: runBenchmark(args, async_libaio); break;
        case IOURING: runBenchmark(args, async_liburing); break;
    }

    return 0;
}