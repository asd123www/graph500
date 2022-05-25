# graph profiling

这里面的数据主要是为了对生成的图有一个结构性的认识.

并且对`top-down`算法在每个stage画的时间做出测量指导我们之后的优化.

文件名为`px_ey.txt`, 其中`x`代表`SCALE`, `y`代表`edgefactor`.

测试用的是`bfs_reference.c`, 当规模达到24时由于速度问题server就给停掉了.