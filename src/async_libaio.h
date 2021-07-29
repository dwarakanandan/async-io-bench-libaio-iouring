#include "helper.h"

#include <inttypes.h>
#include <sys/syscall.h>
#include <linux/aio_abi.h>

Result_t async_libaio(const RuntimeArgs_t& args);