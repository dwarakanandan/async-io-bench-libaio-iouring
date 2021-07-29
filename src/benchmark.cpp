#include "helper.h"
#include "syncio.h"
#include "async_libaio.h"

using namespace std;

int main(int argc, char const *argv[])
{
    RuntimeArgs_t args = mapUserArgsToRuntimeArgs(argc, argv);
    fileOpen(&args);

    runBenchmark(args, syncio);

    return 0;
}