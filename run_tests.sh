#! /bin/sh

# Tests file binaries and test sizes
ipc_tests="posixq pipe fifo socketpair uds tcp shm udp"
ipc_sizes="128 256 512 1024 2048"
ipc_count=10000

# Write to log file, keeps echo parameters
write_log()
{
    local arg=""

    if [ $# -eq 2 ]; then
	arg=$1
	shift
    fi
    echo ${arg} $1 >> ${logfile}
}

# Check whether the test binaries actually exist
for test in ${ipc_tests}
do
    if [ ! -x ${test} ]; then
	echo "Program ${test} do not exist or is not a binary."
	echo "Have you built the tests?"
	exit 1
    fi
done

# Call the test
for test in ${ipc_tests}
do
    # Initialize
    logfile=$(mktemp /tmp/ipc.XXXXXX)
    line1=""
    line2=""

    write_log "${test}"

    # Headers
    write_log "$(echo ${ipc_sizes} | tr [:space:] '|')"

    # Benchmark's actual data
    for tsize in ${ipc_sizes}
    do
	    result=$(./${test} ${tsize} ${ipc_count})
	    line1="${line1}|$(echo $result | cut -d ' ' -f1)"
	    line2="${line2}|$(echo $result | cut -d ' ' -f2)"
    done
    write_log "${line1}"
    write_log "${line2}"

    # Display results
    column -s '|' -t ${logfile}
    echo ""

    # Cleanup
    rm ${logfile}
done
