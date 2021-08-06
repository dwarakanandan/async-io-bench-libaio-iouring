#include "helper.h"

#include <liburing.h>

#define QUEUE_DEPTH 100

struct file_info {
    off_t file_sz;
    struct iovec iovecs[];
};

Result_t async_liburing(const RuntimeArgs_t& args);