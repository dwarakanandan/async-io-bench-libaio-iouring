#include "helper.h"
#include "syncio.h"
#include "async_libaio.h"

using namespace std;
#define NUM_BLOCK_SIZES 10

int block_sizes[NUM_BLOCK_SIZES] = {2, 4, 16, 32, 64, 128, 256, 512, 1024, 2048};

void runIoBenchmark(RuntimeArgs_t args, std::string operation, std::string opmode, int block_size, Result_t (*benchmarkFunction)(const RuntimeArgs_t& args)) {
    args.operation = operation;
    args.opmode = opmode;
    args.blk_size = block_size;
    runBenchmark(args, benchmarkFunction);
}

void runIoBenchmarks(RuntimeArgs_t args, Result_t (*benchmarkFunction)(const RuntimeArgs_t& args)) {
    for (size_t i = 0; i < NUM_BLOCK_SIZES; i++) runIoBenchmark(args, READ, SEQUENTIAL, block_sizes[i], benchmarkFunction);
    for (size_t i = 0; i < NUM_BLOCK_SIZES; i++) runIoBenchmark(args, READ, RANDOM, block_sizes[i], benchmarkFunction);
    for (size_t i = 0; i < NUM_BLOCK_SIZES; i++) runIoBenchmark(args, WRITE, SEQUENTIAL, block_sizes[i], benchmarkFunction);
    for (size_t i = 0; i < NUM_BLOCK_SIZES; i++) runIoBenchmark(args, WRITE, RANDOM, block_sizes[i], benchmarkFunction);
}

int main(int argc, char const *argv[])
{
    if (argc < 5) {
        cout << "benchmark --file <file_name> --threads <thread_count> --debug" << std::endl;
        return -1;
    }

    RuntimeArgs_t args = mapUserArgsToRuntimeArgs(argc, argv);
    fileOpen(&args);

    cout << "syncio:" << endl;
    runIoBenchmarks(args, syncio);

    cout << "async_libaio:" << endl;
    runIoBenchmarks(args, async_libaio);

    return 0;
}