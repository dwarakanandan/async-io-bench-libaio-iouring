#include "helper.h"
#include "sync_io.h"
#include "async_libaio.h"
#include "async_liburing.h"
#include "msg_rate.h"

#include <future>
#include <vector>
#include <sys/ioctl.h>
#include <sys/stat.h>

using namespace std;

void fileNameCheck(int argc, char const *argv[])
{
    bool hasFileName = false;
    for (int i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], "--file") == 0)
        {
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
            << "--vsize <vectored IO batch size> "
            << "--runtime <runtime seconds> "
            << "--nodirect (disable O_DIRECT) "
            << "--btype <normal|stress|msg> "
            << "--minterval <message interval in nano seconds> "
            << "--debug (show_debug) " << endl;

    if (!hasFileName)
    {
        cout << helpstr.str();
        exit(-1);
    }
}

RuntimeArgs_t getDefaultArgs()
{
    RuntimeArgs_t args;
    args.thread_count = 1;
    args.blk_size = 4;
    args.debugInfo = false;
    args.operation = READ;
    args.opmode = SEQUENTIAL;
    args.oio = 8;
    args.lib = SYNCIO;
    args.odirect = true;
    args.vec_size = 0;
    args.runtime = 1;
    args.benchmark_type = NORMAL;
    args.message_interval = 1000;
    return args;
}

RuntimeArgs_t mapUserArgsToRuntimeArgs(int argc, char const *argv[])
{
    RuntimeArgs_t args = getDefaultArgs();
    for (int i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], "--file") == 0)
        {
            args.filename = argv[i + 1];
        }
        if (strcmp(argv[i], "--bsize") == 0)
        {
            args.blk_size = atoi(argv[i + 1]);
        }
        if (strcmp(argv[i], "--threads") == 0)
        {
            args.thread_count = atoi(argv[i + 1]);
        }
        if (strcmp(argv[i], "--op") == 0)
        {
            args.operation = strcmp(argv[i + 1], "read") == 0 ? READ : WRITE;
        }
        if (strcmp(argv[i], "--mode") == 0)
        {
            args.opmode = strcmp(argv[i + 1], "seq") == 0 ? SEQUENTIAL : RANDOM;
        }
        if (strcmp(argv[i], "--oio") == 0)
        {
            args.oio = atoi(argv[i + 1]);
        }
        if (strcmp(argv[i], "--vsize") == 0)
        {
            args.vec_size = atoi(argv[i + 1]);
        }
        if (strcmp(argv[i], "--runtime") == 0)
        {
            args.runtime = atoi(argv[i + 1]);
        }
        if (strcmp(argv[i], "--debug") == 0)
        {
            args.debugInfo = true;
        }
        if (strcmp(argv[i], "--nodirect") == 0)
        {
            args.odirect = false;
        }
        if (strcmp(argv[i], "--lib") == 0)
        {
            args.lib = strcmp(argv[i + 1], "syncio") == 0 ? SYNCIO : (strcmp(argv[i + 1], "libaio") == 0 ? LIBAIO : IOURING);
        }
        if (strcmp(argv[i], "--btype") == 0)
        {
            args.benchmark_type = strcmp(argv[i + 1], "stress") == 0 ? STRESS : (strcmp(argv[i + 1], "msg") == 0 ? MSG_RATE : NORMAL);
        }
        if (strcmp(argv[i], "--minterval") == 0)
        {
            args.message_interval = atoi(argv[i + 1]);
        }
    }
    return args;
}

void runBenchmark(RuntimeArgs_t &userArgs, Result_t (*benchmarkFunction)(const RuntimeArgs_t &args))
{
    std::vector<std::future<Result_t>> threads;
    for (int i = 0; i < userArgs.thread_count; ++i)
    {
        RuntimeArgs_t args;
        args.thread_id = i;
        args.blk_size = userArgs.blk_size;
        args.fd = userArgs.fd;
        args.debugInfo = userArgs.debugInfo;
        args.read_offset = (_100GB * i) % userArgs.max_offset;
        args.operation = userArgs.operation;
        args.opmode = userArgs.opmode;
        args.oio = userArgs.oio;
        args.max_offset = userArgs.max_offset;
        args.vec_size = userArgs.vec_size;
        args.runtime = userArgs.runtime;
        args.benchmark_type = userArgs.benchmark_type;
        threads.push_back(std::async(benchmarkFunction, args));
    }

    double totalThroughput = 0;
    uint64_t totalOps = 0;
    for (auto &t : threads)
    {
        auto results = t.get();
        totalThroughput += results.throughput;
        totalOps += results.op_count;
    }

    cout << std::fixed
         << libToString(userArgs.lib) << " "
         << operationToString(userArgs.operation) << " "
         << opmodeToString(userArgs.opmode) << " "
         << "BLKS_KB:" << userArgs.blk_size << " "
         << "OIO:" << ((userArgs.lib == SYNCIO) ? 1 : userArgs.oio) << " "
         << "OP_CNT:" << totalOps << " "
         << "TPUT_GBPS:" << totalThroughput << endl;
}

off_t getFileSize(int fd)
{
    struct stat st;

    if (fstat(fd, &st) < 0)
    {
        perror("fstat");
        return -1;
    }

    if (S_ISBLK(st.st_mode))
    {
        unsigned long long bytes;
        if (ioctl(fd, BLKGETSIZE64, &bytes) != 0)
        {
            perror("ioctl");
            return -1;
        }
        return bytes;
    }
    else if (S_ISREG(st.st_mode))
        return st.st_size;

    return -1;
}

int main(int argc, char const *argv[])
{
    fileNameCheck(argc, argv);
    srand(time(NULL));

    RuntimeArgs_t args = mapUserArgsToRuntimeArgs(argc, argv);
    fileOpen(&args);

    args.max_offset = getFileSize(args.fd);

    if (args.benchmark_type == MSG_RATE)
    {
        runMessageRateBenchmark(args);
        return 0;
    }

    switch (args.lib)
    {
    case SYNCIO:
        runBenchmark(args, syncio);
        break;
    case LIBAIO:
        runBenchmark(args, async_libaio);
        break;
    case IOURING:
        runBenchmark(args, async_liburing);
        break;
    }

    return 0;
}