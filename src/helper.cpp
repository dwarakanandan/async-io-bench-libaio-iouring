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