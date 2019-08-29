#! /usr/bin/python
import os
import subprocess
from prettytable import PrettyTable

ipc_tests = ["pipe", "fifo", "socketpair", "uds", "tcp"]
ipc_sizes = [128, 256, 512, 1024, 2048, 4096]
ipc_count = 100000

test_stats = []

def run_cmd(cmd):
    print cmd
    process = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output = ""
    for line in iter(process.stdout.readline, ""):
        output += line
    process.wait()
    status = process.returncode
    return status, output

for test in ipc_tests:
    stats = []
    for size in ipc_sizes:
        st, out = run_cmd("./{} {} {}".format(test, size, ipc_count))
        if st != 0: raise Exception(out)
        rs = out.strip("\n").split(" ")
        stats.append(rs)
    test_stats.append(stats)

head = []
head.append("test/size")
for size in ipc_sizes:
    head.append("{} x {}".format(size, ipc_count))

print "TPS"
table = PrettyTable(head)
for i, stats in enumerate(test_stats):
    row = []
    row.append(ipc_tests[i])
    for stat in stats:
        row.append(stat[0])
    table.add_row(row)
print str(table)

print "QPS"
table = PrettyTable(head)
for i, stats in enumerate(test_stats):
    row = []
    row.append(ipc_tests[i])
    for stat in stats:
        row.append(stat[1])
    table.add_row(row)
print str(table)