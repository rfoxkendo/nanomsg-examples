#!/bin/bash

echo "" >reqreptimings.log    # new file.
for uri in tcp://127.0.0.1:3000 ipc:///tmp/reqrep inproc:///reqrep
do
    echo "---- $uri timings ----" >> reqreptimings.log 
    for size in 1024 2048 4096 8192 16384 32768 65536 131072 262144 524288 1048576
    do
        
        echo =====  size $size puller $pullers >> reqreptimings.log 
        ./reqrep $uri 100000 $size   >> reqreptimings.log 
    done
done