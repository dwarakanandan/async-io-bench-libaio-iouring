import os
import time

base_command = 'taskset -c 20 ./build/benchmark --file /dev/md127 '
base_command_without_taskset = './build/benchmark --file /dev/md127 '

def runBenchmarkAllOios(lib, threads, op, mode, bsize):
    oio_sizes = [2, 4, 8, 16, 32, 64, 128, 256, 512, 1024]
    print('Running runBenchmarkAllOios lib:' + lib + ' threads:' + str(threads) + ' op:' + op + ' mode:' + mode + ' bsize:' + str(bsize))
    avg_tputs = []
    avg_opcounts = []
    for oio in oio_sizes:
        tputs = []
        opcounts = []
        for i in range(0, 10):
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
    print('TPUT_GBPS average over 10 runs for OIO:')
    for avg_tput in avg_tputs:
        print("%.2f" % round(avg_tput, 2))
    print('\n')
    print('OP_CNT average over 10 runs for OIO:')
    for avg_opcount in avg_opcounts:
        print("%d" % avg_opcount)
    print('\n')

def runBenchmarkAllOiosNodirect(lib, threads, op, mode, bsize):
    oio_sizes = [2, 4, 8, 16, 32, 64, 128, 256, 512, 1024]
    print('Running runBenchmarkAllOiosNodirect lib:' + lib + ' threads:' + str(threads) + ' op:' + op + ' mode:' + mode + ' bsize:' + str(bsize))
    avg_tputs = []
    avg_opcounts = []
    for oio in oio_sizes:
        tputs = []
        opcounts = []
        for i in range(0, 10):
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
    print('TPUT_GBPS average over 10 runs for OIO:')
    for avg_tput in avg_tputs:
        print("%.2f" % round(avg_tput, 2))
    print('\n')
    print('OP_CNT average over 10 runs for OIO:')
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
            for i in range(0, 10):
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
    print('TPUT_GBPS average over 10 runs for OIO:')
    for avg_tput in avg_tputs:
        print("%.2f" % round(avg_tput, 2))
    print('\n')
    print('OP_CNT average over 10 runs for OIO:')
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
            for i in range(0, 10):
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
    print('TPUT_GBPS average over 10 runs for OIO:')
    for avg_tput in avg_tputs:
        print("%.2f" % round(avg_tput, 2))
    print('\n')
    print('OP_CNT average over 10 runs for OIO:')
    for avg_opcount in avg_opcounts:
        print("%d" % avg_opcount)
    print('\n')

def runBenchmarkAllThreads(lib, op, mode, bsize, oio):
    thread_counts = [1, 4, 8, 16, 32, 64, 128]
    print('Running runBenchmarkAllThreads lib:' + lib + ' op:' + op + ' mode:' + mode + ' bsize:' + str(bsize) + ' oio:' + str(oio))
    avg_tputs = []
    avg_opcounts = []
    for thread in thread_counts:
        tputs = []
        opcounts = []
        for i in range(0, 10):
            stream = os.popen(base_command_without_taskset + '--threads ' + str(thread) + ' --bsize ' + str(bsize) + ' --op ' + op + ' --mode ' + mode + ' --lib ' + lib + ' --oio ' + str(oio))
            split = stream.readline().strip().split(' ')
            for s in split:
                if s.startswith('TPUT_GBPS'):
                    tputs.append(float(s.split(':')[1]))
                if s.startswith('OP_CNT'):
                    opcounts.append(int(s.split(':')[1]))
        avg_tputs.append(sum(tputs)/len(tputs))
        avg_opcounts.append(sum(opcounts)/len(opcounts))

    print(thread_counts)
    print('TPUT_GBPS average over 10 runs for thread_counts:')
    for avg_tput in avg_tputs:
        print("%.2f" % round(avg_tput, 2))
    print('\n')

def runBenchmarkAllThreadsVectored(lib, op, mode, bsize, oio, vsize):
    thread_counts = [1, 4, 8, 16, 32, 64, 128]
    print('Running runBenchmarkAllThreads lib:' + lib + ' op:' + op + ' mode:' + mode + ' bsize:' + str(bsize) + ' oio:' + str(oio) + ' vsize: ' + str(vsize))
    avg_tputs = []
    avg_opcounts = []
    for thread in thread_counts:
        tputs = []
        opcounts = []
        for i in range(0, 10):
            stream = os.popen(base_command_without_taskset + '--threads ' + str(thread) + ' --bsize ' + str(bsize) + ' --op ' + op + ' --mode ' + mode + ' --lib ' + lib + ' --oio ' + str(oio) + ' --vsize ' + str(vsize))
            split = stream.readline().strip().split(' ')
            for s in split:
                if s.startswith('TPUT_GBPS'):
                    tputs.append(float(s.split(':')[1]))
                if s.startswith('OP_CNT'):
                    opcounts.append(int(s.split(':')[1]))
        avg_tputs.append(sum(tputs)/len(tputs))
        avg_opcounts.append(sum(opcounts)/len(opcounts))

    print(thread_counts)
    print('TPUT_GBPS average over 10 runs for thread_counts:')
    for avg_tput in avg_tputs:
        print("%.2f" % round(avg_tput, 2))
    print('\n')

def runMsgRateBenchmarkAllOios(lib, threads, op, mode, bsize):
    oio_sizes = [1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024]
    print('Running runBenchmarkAllOios lib:' + lib + ' threads:' + str(threads) + ' op:' + op + ' mode:' + mode + ' bsize:' + str(bsize))
    avg_tputs = []
    avg_opcounts = []
    for oio in oio_sizes:
        tputs = []
        opcounts = []
        for i in range(0, 10):
            stream = os.popen(base_command + '--threads ' + str(threads) + ' --bsize ' + str(bsize) + ' --oio ' + str(oio) + ' --op ' + op + ' --mode ' + mode + ' --lib ' + lib+ ' --minterval ' + str(1) + ' --btype msg --nodirect')
            split = stream.readline().strip().split(' ')
            for s in split:
                if s.startswith('TPUT_GBPS'):
                    tputs.append(float(s.split(':')[1]))
                if s.startswith('OP_CNT'):
                    opcounts.append(int(s.split(':')[1]))
        avg_tputs.append(sum(tputs)/len(tputs))
        avg_opcounts.append(sum(opcounts)/len(opcounts))
    
    print(oio_sizes)
    print('TPUT_GBPS average over 10 runs for OIO:')
    for avg_tput in avg_tputs:
        print("%.2f" % round(avg_tput, 2))
    print('\n')
    print('OP_CNT average over 10 runs for OIO:')
    for avg_opcount in avg_opcounts:
        print("%d" % avg_opcount)
    print('\n')

# runBenchmarkAllOios('libaio' , 1, 'read', 'rand', 4)
# runBenchmarkAllOios('iouring' , 1, 'read', 'rand', 4)


# runBenchmarkAllOiosVectored('libaio' , 1, 'read', 'rand', 4)
# runBenchmarkAllOiosVectored('iouring' , 1, 'read', 'rand', 4)


# runBenchmarkAllOiosVectored('iouring' , 1, 'read', 'rand', 4)
# runBenchmarkAllOiosVectoredNodirect('iouring' , 1, 'read', 'rand', 4)


# runBenchmarkAllOios('iouring' , 1, 'read', 'rand', 4)
# runBenchmarkAllOiosNodirect('iouring' , 1, 'read', 'rand', 4)


# runBenchmarkAllThreads('syncio', 'read', 'rand', 4, 1)
# runBenchmarkAllThreads('libaio', 'read', 'rand', 4, 8)
# runBenchmarkAllThreads('iouring', 'read', 'rand', 4, 8)
# runBenchmarkAllThreads('libaio', 'read', 'rand', 4, 64)
# runBenchmarkAllThreads('iouring', 'read', 'rand', 4, 64)
# runBenchmarkAllThreads('libaio', 'read', 'rand', 4, 128)
# runBenchmarkAllThreads('iouring', 'read', 'rand', 4, 128)


# runBenchmarkAllThreadsVectored('syncio', 'read', 'rand', 4, 1, 100)
# runBenchmarkAllThreadsVectored('libaio', 'read', 'rand', 4, 8, 100)
# runBenchmarkAllThreadsVectored('iouring', 'read', 'rand', 4, 8, 100)
# runBenchmarkAllThreadsVectored('libaio', 'read', 'rand', 4, 64, 50)
# runBenchmarkAllThreadsVectored('iouring', 'read', 'rand', 4, 64, 50)
# runBenchmarkAllThreadsVectored('libaio', 'read', 'rand', 4, 128, 10)
# runBenchmarkAllThreadsVectored('iouring', 'read', 'rand', 4, 128, 10)

runMsgRateBenchmarkAllOios('iouring', 1, 'READ', 'RAND', 256)