#include "helper.h"
#include "sync_io.h"
#include "async_libaio.h"
#include "async_liburing.h"

#include <thread>
#include <iostream>
#include <chrono>
#include <queue>
#include <mutex>
#include <vector>
#include <condition_variable>

void runMessageRateBenchmark(RuntimeArgs_t &userArgs);