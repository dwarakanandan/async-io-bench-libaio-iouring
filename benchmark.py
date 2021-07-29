import os

base_command = 'sudo ./build/benchmark --file /dev/md0 '

stream = os.popen(base_command + '--threads 8 --bsize 2 --op write --mode rand --lib libaio')
output = stream.read()