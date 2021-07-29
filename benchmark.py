import os

base_command = 'sudo ./build/benchmark --file /dev/md0 '

libs = []
# libs.append('syncio')
libs.append('libaio')

ops = []
ops.append('read')
# ops.append('write')

threads = []
threads.append(1)
# threads.append(8)

bsizes = [2, 4, 16, 64, 128, 256, 512, 1024, 2048, 102400]

outputs = []

def runBenchmark(lib, threads, op, mode):
    print('Running lib:' + lib + ' threads:' + str(threads) + ' op:' + op + ' mode:' + mode)
    for bsize in bsizes:
        stream = os.popen(base_command + '--threads ' + str(threads) + ' --bsize ' + str(bsize) + ' --op ' + op + ' --mode ' + mode + ' --lib ' + lib)
        outputs.append(stream.readline().strip())

for thread in threads:
    for lib in libs:
        for op in ops:
            runBenchmark(lib, thread, op, 'seq')
            runBenchmark(lib, thread, op, 'rand')

for output in outputs:
    print(output)