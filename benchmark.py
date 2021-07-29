import os

base_command = 'sudo ./build/benchmark --file /dev/md0 '

bsizes = [2, 4, 16, 64, 128, 256, 512, 1024, 2048]

outputs = []

def runBenchmark(lib, threads, op, mode):
    print('Running: lib:' + lib + ' threads: ' + str(threads) + ' op: ' + op + ' mode: ' + mode)
    for bsize in bsizes:
        stream = os.popen(base_command + '--threads ' + str(threads) + ' --bsize ' + str(bsize) + ' --op ' + op + ' --mode ' + mode + ' --lib ' + lib)
        outputs.append(stream.readline().strip())


runBenchmark('syncio', 1, 'read', 'seq')
runBenchmark('syncio', 1, 'read', 'rand')
runBenchmark('syncio', 1, 'write', 'seq')
runBenchmark('syncio', 1, 'write', 'rand')
runBenchmark('libaio', 1, 'read', 'seq')
runBenchmark('libaio', 1, 'read', 'rand')
runBenchmark('libaio', 1, 'write', 'seq')
runBenchmark('libaio', 1, 'write', 'rand')

for output in outputs:
    print(output)