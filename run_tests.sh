#! /bin/bash

readonly IPCS=(pipe fifo socketpair uds tcp)
readonly SIZE=(128 512 1024 4096)

for ipc in ${IPCS[@]}; do
    result=()
    for i in ${SIZE[@]}; do
    result+=($i)
        t=$(./$ipc $i 1024)
    result+=($t)
    done

    printf "%s\n" $ipc
    for value in ${result[@]}; do
    printf "%s\n" $value
    done | column
done
