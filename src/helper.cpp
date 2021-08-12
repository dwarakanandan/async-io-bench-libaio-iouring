#include "helper.h"

using namespace std;

void printStats(const RuntimeArgs_t& args, const Result_t results) {
    std::stringstream stats;
    stats << "TID:" << args.thread_id
        << " offset:" << args.read_offset / _1GB << "GB"
        << " ops:" << results.op_count
        << " throughput:" << results.throughput << " GB/s" << endl << endl;
    cout << stats.str();
}

void printOpStats(const RuntimeArgs_t& args, const Result_t results) {
	std::stringstream msg;
	msg << "TID:" << args.thread_id
		<< " OP_SUBMIT: " << results.ops_submitted << " "
		<< " OP_RETURNED: " << results.ops_returned << " "
		<< " OP_FAILED: " << results.ops_failed << endl
        << " offset:" << args.read_offset / _1GB << "GB"
        << " ops:" << results.op_count
        << " throughput:" << results.throughput << " GB/s" << endl << endl;
	cout << msg.str();
}

const char* getErrorMessageWithTid(const RuntimeArgs_t& args, std::string error) {
	std::stringstream msg;
	msg << "TID:" << args.thread_id << " "<< error;
	return msg.str().c_str();
}

void fileOpen(RuntimeArgs_t *args) {
    args->fd = (args->odirect) ?
        open(args->filename.c_str(), O_RDWR | O_CREAT | O_DIRECT, S_IRUSR | S_IWUSR) :
        open(args->filename.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (args->fd < 0) {
        perror("Open error");
        exit(-1);
    }
}

double calculateThroughputGbps(uint64_t ops, size_t buffer_size) {
    return ((ops * buffer_size)/(1024.0*1024*1024 * RUN_TIME));
}

off_t getOffset(off_t maxOffset, off_t initialOffset, size_t buffer_size, uint64_t iteration, bool isRand) {
    return isRand ?
        (initialOffset + (rand() * buffer_size) ) % maxOffset:
        (initialOffset + (iteration * buffer_size) ) % maxOffset;
}

Result_t return_error() {
	Result_t results;
	results.throughput = 0;
	results.op_count = 0;
	return results;
}

std::string operationToString(OPERATION operation) {
    switch (operation) {
        case READ: return "RD";
        case WRITE: return "WR";
        default: return "";
    }
}

std::string opmodeToString(OPMODE opmode) {
    switch (opmode) {
        case SEQUENTIAL: return "SEQ";
        case RANDOM: return "RND";
        default: return "";
    }
}

std::string libToString(LIB lib) {
    switch (lib) {
        case SYNCIO: return "SYNCIO";
        case LIBAIO: return "LIBAIO";
        case IOURING: return "IOURING";
        default: return "";
    }
}