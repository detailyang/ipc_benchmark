# ipc_benchmark
IPC benchmark on Linux which is inspired by [APUE](http://www.apuebook.com/)

# Test

Just Make style as the following :D

```bash
make && make test
```

# Type

These are kinds of IPC in Linux as the following:

type|feature
| ------------- |:-------------:
pipe|unnamed pipe
fifo|named pipe
socketpair | unnamed unix domain socket
unix domain socket | named unix domain socket
TCP | remote domain socket
UDP | loopback interface

#Benchmark

In my personal MacBook Pro (Retina, 13-inch, Late 2013) Intel(R) Core(TM) i5-4258U CPU @ 2.40GHz, the benchmark is as the following:

```c
pipe
128             512           1024          4096
1319Mb/s        5110Mb/s      8932Mb/s      20297Mb/s
1288233msg/s    1247449msg/s  1090370msg/s  619407msg/s

fifo
128             512           1024          4096
1358Mb/s        5491Mb/s      8440Mb/s      22018Mb/s
1326016msg/s    1340502msg/s  1030215msg/s  671929msg/s

socketpair
128             512           1024          4096
1117Mb/s        4401Mb/s      8869Mb/s      17207Mb/s
1090370msg/s    1074548msg/s  1082674msg/s  525121msg/s

uds
128             512           1024          4096
100Mb/s         102Mb/s       109Mb/s       97Mb/s
97600msg/s      24986msg/s    13289msg/s    2968msg/s

tcp
128             512           1024          4096
106Mb/s         143Mb/s       135Mb/s       128Mb/s
103508msg/s     34852msg/s    16529msg/s    3897msg/s
```

In Hasee Notebook Intel(R) Core(TM) i7-4710MQ @ 2.5GHz, with DDR3 2400 8GB x1, the benchmark is as the following:

```c
pipe
128		512		1024		4096
1808Mb/s	6899Mb/s	10836Mb/s	33358Mb/s
1766023msg/s	1684301msg/s	1322749msg/s	1018006msg/s

fifo
128		512		1024		4096
884Mb/s		3758Mb/s	7333Mb/s	27825Mb/s
863310msg/s	917532msg/s	895158msg/s	849143msg/s

socketpair
128		512		1024		4096
752Mb/s		3591Mb/s	7418Mb/s	20385Mb/s
734559msg/s	876703msg/s	905538msg/s	622098msg/s

uds
128		512		1024		4096
70Mb/s		522Mb/s		843Mb/s		1175Mb/s
68744msg/s	127364msg/s	102957msg/s	35857msg/s

tcp
128		512		1024		4096
197Mb/s		498Mb/s		951Mb/s		1174Mb/s
192263msg/s	121701msg/s	116046msg/s	35828msg/s

udp
128		512		1024		4096
155Mb/s		497Mb/s		691Mb/s		2907Mb/s
151295msg/s	121241msg/s	84371msg/s	88712msg/s
```

Also we make the benchmark on travis-ci [![Build Status](https://travis-ci.org/detailyang/ipc_benchmark.svg?branch=master)](https://travis-ci.org/detailyang/ipc_benchmark)


Contributing
------------

To contribute to ipc_benchmark, clone this repo locally and commit your code on a separate branch. 


Author
------

> GitHub [@detailyang](https://github.com/detailyang)     


License
-------

ipc_benchmark is licensed under the [MIT](https://github.com/detailyang/ipc_benchmark/blob/master/LICENSE) license.  
