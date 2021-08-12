import os

base_command = './build/benchmark --file /dev/md127 '

libs = []
libs.append('syncio')
libs.append('libaio')
libs.append('iouring')

ops = []
ops.append('read')
ops.append('write')

threads = []
threads.append(1)
threads.append(8)

bsizes = [2, 4, 16, 64, 128, 256, 512, 1024, 2048, 102400]

oio_sizes = [1, 10, 50, 100, 500, 1000]

outputs_global = []

def runBenchmarkAllOios(lib, threads, op, mode, bsize):
    print('Running lib:' + lib + ' threads:' + str(threads) + ' op:' + op + ' mode:' + mode + ' bsize:' + str(bsize))
    outputs = []
    for oio in oio_sizes:
        stream = os.popen(base_command + '--threads ' + str(threads) + ' --bsize ' + str(bsize) + ' --oio ' + str(oio) + ' --op ' + op + ' --mode ' + mode + ' --lib ' + lib)
        outputs.append(stream.readline().strip())
    outputs_global.append(outputs)
    for output in outputs:
        print(output)
    print()
        

def runBenchmarkAllBlks(lib, threads, op, mode):
    print('Running lib:' + lib + ' threads:' + str(threads) + ' op:' + op + ' mode:' + mode)
    outputs = []
    for bsize in bsizes:
        stream = os.popen(base_command + '--threads ' + str(threads) + ' --bsize ' + str(bsize) + ' --op ' + op + ' --mode ' + mode + ' --lib ' + lib)
        outputs.append(stream.readline().strip())
    outputs_global.append(outputs)
    for output in outputs:
        print(output)
    print()

def runBenchmark(lib, threads, op, mode, bsize):
    print('Running lib:' + lib + ' threads:' + str(threads) + ' op:' + op + ' mode:' + mode + ' bsize:' + str(bsize))
    outputs = []
    stream = os.popen(base_command + '--threads ' + str(threads) + ' --bsize ' + str(bsize) + ' --op ' + op + ' --mode ' + mode + ' --lib ' + lib)
    outputs.append(stream.readline().strip())
    outputs_global.append(outputs)
    for output in outputs:
        print(output)
    print()

def mainBenchmark():
    for thread in threads:
        for lib in libs:
            for op in ops:
                runBenchmarkAllBlks(lib, thread, op, 'seq')
                runBenchmarkAllBlks(lib, thread, op, 'rand')

def basicSanityTest():
    runBenchmark('syncio', 1, 'read', 'seq', 4)
    runBenchmark('libaio', 1, 'read', 'seq', 4)
    runBenchmark('iouring', 1, 'read', 'seq', 4)
    runBenchmark('syncio', 1, 'read', 'rand', 4)
    runBenchmark('libaio', 1, 'read', 'rand', 4)
    runBenchmark('iouring', 1, 'read', 'rand', 4)
    runBenchmark('syncio', 1, 'write', 'seq', 4)
    runBenchmark('libaio', 1, 'write', 'seq', 4)
    runBenchmark('iouring', 1, 'write', 'seq', 4)
    runBenchmark('syncio', 1, 'write', 'rand', 4)
    runBenchmark('libaio', 1, 'write', 'rand', 4)
    runBenchmark('iouring', 1, 'write', 'rand', 4)
    runBenchmark('syncio', 8, 'read', 'seq', 4)
    runBenchmark('libaio', 8, 'read', 'seq', 4)
    runBenchmark('iouring', 8, 'read', 'seq', 4)
    runBenchmark('syncio', 8, 'read', 'rand', 4)
    runBenchmark('libaio', 8, 'read', 'rand', 4)
    runBenchmark('iouring', 8, 'read', 'rand', 4)
    runBenchmark('syncio', 8, 'write', 'seq', 4)
    runBenchmark('libaio', 8, 'write', 'seq', 4)
    runBenchmark('iouring', 8, 'write', 'seq', 4)
    runBenchmark('syncio', 8, 'write', 'rand', 4)
    runBenchmark('libaio', 8, 'write', 'rand', 4)
    runBenchmark('iouring', 8, 'write', 'rand', 4)

basicSanityTest()

# runBenchmarkAllOios('libaio' ,1, 'read', 'seq', 4)
# runBenchmarkAllOios('iouring' ,1, 'read', 'seq', 4)

# runBenchmarkAllOios('libaio' , 1, 'read', 'rand', 4)
# runBenchmarkAllOios('iouring' , 1, 'read', 'rand', 4)