import os
import time

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

bsizes = [2, 4, 8, 16, 64, 128, 512, 1024, 2048, 102400]

oio_sizes = [2, 4, 8, 16, 64, 128, 256, 512, 1024]

outputs_global = []

def runBenchmarkAllOiosTput(lib, threads, op, mode, bsize):
    print('Running lib:' + lib + ' threads:' + str(threads) + ' op:' + op + ' mode:' + mode + ' bsize:' + str(bsize))
    print('TPUT_GBPS average over 10 runs for OIO:')
    print(oio_sizes)
    for oio in oio_sizes:
        tputs = []
        for i in range(0, 10):
            time.sleep(2)
            stream = os.popen(base_command + '--threads ' + str(threads) + ' --bsize ' + str(bsize) + ' --oio ' + str(oio) + ' --op ' + op + ' --mode ' + mode + ' --lib ' + lib)
            split = stream.readline().strip().split(' ')
            for s in split:
                if s.startswith('TPUT_GBPS'):
                    tputs.append(float(s.split(':')[1]))
        avg_tput = sum(tputs)/len(tputs)
        print("%.2f" % avg_tput)
    print()

def runBenchmarkAllOiosOpcount(lib, threads, op, mode, bsize):
    print('Running lib:' + lib + ' threads:' + str(threads) + ' op:' + op + ' mode:' + mode + ' bsize:' + str(bsize))
    print('OP_CNT average over 10 runs for OIO:')
    print(oio_sizes)
    for oio in oio_sizes:
        tputs = []
        for i in range(0, 10):
            time.sleep(2)
            stream = os.popen(base_command + '--threads ' + str(threads) + ' --bsize ' + str(bsize) + ' --oio ' + str(oio) + ' --op ' + op + ' --mode ' + mode + ' --lib ' + lib)
            split = stream.readline().strip().split(' ')
            for s in split:
                if s.startswith('OP_CNT'):
                    tputs.append(int(s.split(':')[1]))
        avg_opcount = sum(tputs)/len(tputs)
        print("%d" % avg_opcount)
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
    # print('Running lib:' + lib + ' threads:' + str(threads) + ' op:' + op + ' mode:' + mode + ' bsize:' + str(bsize))
    outputs = []
    stream = os.popen(base_command + '--threads ' + str(threads) + ' --bsize ' + str(bsize) + ' --op ' + op + ' --mode ' + mode + ' --lib ' + lib)
    outputs.append(stream.readline().strip())
    outputs_global.append(outputs)
    for output in outputs:
        print(output)

runBenchmarkAllOiosOpcount('libaio' , 1, 'read', 'rand', 4)
runBenchmarkAllOiosOpcount('iouring' , 1, 'read', 'rand', 4)