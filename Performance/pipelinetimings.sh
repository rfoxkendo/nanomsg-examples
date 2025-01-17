#!/bin/bash

echo "" >pipelineTimings.log    # new file.
for uri in tcp://127.0.0.1:3000 ipc:///tmp/pipeline ipc:///pipeline
do
    echo "---- $uri timings ----" >> pipelineTimings.log 
    for size in 1024 2048 4096 8192 16384 32768 65536 131072 262144 524288 1048576
    do
        for pullers in 1 2 3 4 5
        do
            echo size $size puller $pullers >> pipelineTimings.log 
            ./pipeline $uri 100000 $size $pullers  >> pipelineTimings.log 
        done
    done
done