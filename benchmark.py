import os
import time

base_command = 'taskset -c 20 ./build/benchmark --file /dev/md127 '

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



outputs_global = []

def runBenchmarkAllOios(lib, threads, op, mode, bsize):
    oio_sizes = [2, 4, 8, 16, 32, 64, 128, 256, 512]
    print('Running runBenchmarkAllOios lib:' + lib + ' threads:' + str(threads) + ' op:' + op + ' mode:' + mode + ' bsize:' + str(bsize))
    avg_tputs = []
    avg_opcounts = []
    for oio in oio_sizes:
        tputs = []
        opcounts = []
        for i in range(0, 5):
            stream = os.popen(base_command + '--threads ' + str(threads) + ' --bsize ' + str(bsize) + ' --oio ' + str(oio) + ' --op ' + op + ' --mode ' + mode + ' --lib ' + lib)
            split = stream.readline().strip().split(' ')
            for s in split:
                if s.startswith('TPUT_GBPS'):
                    tputs.append(float(s.split(':')[1]))
                if s.startswith('OP_CNT'):
                    opcounts.append(int(s.split(':')[1]))
        avg_tputs.append(sum(tputs)/len(tputs))
        avg_opcounts.append(sum(opcounts)/len(opcounts))
    
    print(oio_sizes)
    print('TPUT_GBPS average over 5 runs for OIO:')
    for avg_tput in avg_tputs:
        print("%.2f" % round(avg_tput, 2))
    print('\n')
    print('OP_CNT average over 5 runs for OIO:')
    for avg_opcount in avg_opcounts:
        print("%d" % avg_opcount)
    print('\n')

def runBenchmarkAllOiosNodirect(lib, threads, op, mode, bsize):
    oio_sizes = [2, 4, 8, 16, 32, 64, 128, 256, 512]
    print('Running runBenchmarkAllOiosNodirect lib:' + lib + ' threads:' + str(threads) + ' op:' + op + ' mode:' + mode + ' bsize:' + str(bsize))
    avg_tputs = []
    avg_opcounts = []
    for oio in oio_sizes:
        tputs = []
        opcounts = []
        for i in range(0, 5):
            stream = os.popen(base_command + '--threads ' + str(threads) + ' --bsize ' + str(bsize) + ' --oio ' + str(oio) + ' --op ' + op + ' --mode ' + mode + ' --lib ' + lib + ' --nodirect')
            split = stream.readline().strip().split(' ')
            for s in split:
                if s.startswith('TPUT_GBPS'):
                    tputs.append(float(s.split(':')[1]))
                if s.startswith('OP_CNT'):
                    opcounts.append(int(s.split(':')[1]))
        avg_tputs.append(sum(tputs)/len(tputs))
        avg_opcounts.append(sum(opcounts)/len(opcounts))
    
    print(oio_sizes)
    print('TPUT_GBPS average over 5 runs for OIO:')
    for avg_tput in avg_tputs:
        print("%.2f" % round(avg_tput, 2))
    print('\n')
    print('OP_CNT average over 5 runs for OIO:')
    for avg_opcount in avg_opcounts:
        print("%d" % avg_opcount)
    print('\n')

def runBenchmarkAllOiosVectored(lib, threads, op, mode, bsize):
    oio_sizes = [8, 16, 32, 64, 128]
    vector_size = [10, 25, 50, 100]
    print('Running runBenchmarkAllOiosVectored lib:' + lib + ' threads:' + str(threads) + ' op:' + op + ' mode:' + mode + ' bsize:' + str(bsize) )
    avg_tputs = []
    avg_opcounts = []
    for oio in oio_sizes:
        for vsize in vector_size:
            tputs = []
            opcounts = []
            for i in range(0, 5):
                stream = os.popen(base_command + '--threads ' + str(threads) + ' --bsize ' + str(bsize) + ' --oio ' + str(oio) + ' --vsize ' + str(vsize) + ' --op ' + op + ' --mode ' + mode + ' --lib ' + lib)
                split = stream.readline().strip().split(' ')
                for s in split:
                    if s.startswith('TPUT_GBPS'):
                        tputs.append(float(s.split(':')[1]))
                    if s.startswith('OP_CNT'):
                        opcounts.append(int(s.split(':')[1]))
            avg_tputs.append(sum(tputs)/len(tputs))
            avg_opcounts.append(sum(opcounts)/len(opcounts))
    
    print(oio_sizes)
    print(vector_size)
    print('TPUT_GBPS average over 5 runs for OIO:')
    for avg_tput in avg_tputs:
        print("%.2f" % round(avg_tput, 2))
    print('\n')
    print('OP_CNT average over 5 runs for OIO:')
    for avg_opcount in avg_opcounts:
        print("%d" % avg_opcount)
    print('\n')

def runBenchmarkAllOiosVectoredNodirect(lib, threads, op, mode, bsize):
    oio_sizes = [8, 16, 32, 64, 128]
    vector_size = [10, 25, 50, 100]
    print('Running runBenchmarkAllOiosVectoredNodirect lib:' + lib + ' threads:' + str(threads) + ' op:' + op + ' mode:' + mode + ' bsize:' + str(bsize) )
    avg_tputs = []
    avg_opcounts = []
    for oio in oio_sizes:
        for vsize in vector_size:
            tputs = []
            opcounts = []
            for i in range(0, 5):
                stream = os.popen(base_command + '--threads ' + str(threads) + ' --bsize ' + str(bsize) + ' --oio ' + str(oio) + ' --vsize ' + str(vsize) + ' --op ' + op + ' --mode ' + mode + ' --lib ' + lib + ' --nodirect')
                split = stream.readline().strip().split(' ')
                for s in split:
                    if s.startswith('TPUT_GBPS'):
                        tputs.append(float(s.split(':')[1]))
                    if s.startswith('OP_CNT'):
                        opcounts.append(int(s.split(':')[1]))
            avg_tputs.append(sum(tputs)/len(tputs))
            avg_opcounts.append(sum(opcounts)/len(opcounts))
    
    print(oio_sizes)
    print(vector_size)
    print('TPUT_GBPS average over 5 runs for OIO:')
    for avg_tput in avg_tputs:
        print("%.2f" % round(avg_tput, 2))
    print('\n')
    print('OP_CNT average over 5 runs for OIO:')
    for avg_opcount in avg_opcounts:
        print("%d" % avg_opcount)
    print('\n')

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


runBenchmarkAllOios('libaio' , 1, 'read', 'rand', 4)
runBenchmarkAllOios('iouring' , 1, 'read', 'rand', 4)

runBenchmarkAllOiosVectored('libaio' , 1, 'read', 'rand', 4)
runBenchmarkAllOiosVectored('iouring' , 1, 'read', 'rand', 4)

# runBenchmarkAllOiosVectored('iouring' , 1, 'read', 'rand', 4)
# runBenchmarkAllOiosVectoredNodirect('iouring' , 1, 'read', 'rand', 4)

# runBenchmarkAllOios('iouring' , 1, 'read', 'rand', 4)
# runBenchmarkAllOiosNodirect('iouring' , 1, 'read', 'rand', 4)