import os

base_command = 'sudo ./build/benchmark --file /dev/md0 '
output = []

benchmark_options = [
    '--threads 1 --bsize 2 --op write --mode rand --lib libaio'
]

bsizes = [2, 4, 16, 64, 256, 512, 1024, 2048]

for bsize in bsizes:
    stream = os.popen(base_command + '--threads 1 --bsize ' + str(bsize) + ' --op write --mode rand --lib libaio')
    output.append(stream.readline())


print(output)