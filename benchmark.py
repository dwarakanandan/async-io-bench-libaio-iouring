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

oio_sizes = [1, 10, 50, 100, 500, 1000]

outputs_global = []

def runBenchmark(lib, threads, op, mode, bsize, oio):
    print('Running lib:' + lib + ' threads:' + str(threads) + ' op:' + op + ' mode:' + mode + ' bsize:' + str(bsize) + ' oio:' + str(oio))
    outputs = []
    stream = os.popen(base_command + '--threads ' + str(threads) + ' --bsize ' + str(bsize) + ' --oio ' + str(oio) + ' --op ' + op + ' --mode ' + mode + ' --lib ' + lib)
    outputs.append(stream.readline().strip())
    outputs_global.append(outputs)
    print(outputs[0])
        

def runBenchmarkAllBlks(lib, threads, op, mode):
    print('Running lib:' + lib + ' threads:' + str(threads) + ' op:' + op + ' mode:' + mode)
    outputs = []
    for bsize in bsizes:
        stream = os.popen(base_command + '--threads ' + str(threads) + ' --bsize ' + str(bsize) + ' --op ' + op + ' --mode ' + mode + ' --lib ' + lib)
        outputs.append(stream.readline().strip())
    outputs_global.append(outputs)
    for output in outputs:
        print(output)

def mainBenchmark():
    for thread in threads:
        for lib in libs:
            for op in ops:
                runBenchmarkAllBlks(lib, thread, op, 'seq')
                runBenchmarkAllBlks(lib, thread, op, 'rand')

def oioBenchmark():
    for oio in oio_sizes:
        runBenchmark('libaio', 1, 'read', 'seq', 16, oio)
        runBenchmark('libaio', 8, 'read', 'seq', 16, oio)