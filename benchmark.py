import os

base_command = 'sudo ./build/benchmark --file /dev/md0 '

bsizes = [2, 4, 16, 64, 256, 512, 1024, 2048]

outputs = []
for bsize in bsizes:
    stream = os.popen(base_command + '--threads 1 --bsize ' + str(bsize) + ' --op write --mode rand --lib libaio')
    outputs.append(stream.readline())


for output in outputs:
    print(output)