#include "helper.h"

#include <liburing.h>

#define QUEUE_DEPTH 1024

Result_t async_liburing(const RuntimeArgs_t& args);