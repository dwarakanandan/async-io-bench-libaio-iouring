import os

base_command = 'sudo ./build/benchmark --file /dev/md0 '

bsizes = [2, 16, 64, 256, 512, 1024, 2048]

def runBenchmark(lib):
    for bsize in bsizes:
        stream = os.popen(base_command + '--threads 1 --bsize ' + str(bsize) + ' --op read --mode seq --lib ' + lib)
        outputs.append(stream.readline().strip())

    for bsize in bsizes:
        stream = os.popen(base_command + '--threads 1 --bsize ' + str(bsize) + ' --op read --mode rand --lib' + lib)
        outputs.append(stream.readline().strip())

    for bsize in bsizes:
        stream = os.popen(base_command + '--threads 1 --bsize ' + str(bsize) + ' --op write --mode seq --lib' + lib)
        outputs.append(stream.readline().strip())

    for bsize in bsizes:
        stream = os.popen(base_command + '--threads 1 --bsize ' + str(bsize) + ' --op write --mode rand --lib' + lib)
        outputs.append(stream.readline().strip())


outputs = []

runBenchmark('syncio')
runBenchmark('libaio')

for output in outputs:
    print(output)