# edge profiling

这里面的数据主要是为了对生成的图各个阶段访问边信息作出统计.

主要目的为检查每个阶段发送的边访问次数, 对比使用`bitset`传输的数据量.

文件名为`px_ey.txt`, 其中`x`代表`SCALE`, `y`代表`edgefactor`.

测试用的是`bfs_reference.c`, 当规模达到24时由于速度问题server就给停掉了.

## Analyze

这是$n = (1<<23)$情况下的数据.
```
Round: 0, pts: 50, time: 0.000
      valid/all: 50/50 
Round: 1, pts: 197446, time: 0.004
      valid/all: 197446/355613 
Round: 2, pts: 3938017, time: 0.637
      valid/all: 3938017/162580558 
Round: 3, pts: 469785, time: 0.472
      valid/all: 469785/104895143 
Round: 4, pts: 1799, time: 0.008
      valid/all: 1799/594484 
Round: 5, pts: 3, time: 0.000
      valid/all: 3/1805 
Round: 6, pts: 0, time: 0.000
      valid/all: 0/3 
```
如果是用bitset在节点之间同步`visit`信息, 则数据量为$(1<<23)/32 = (1<<18) = 262144$个`int`.

对比`Round 1,2,3,4`传输的边单位为$2$个`int`, 我认为这可以忽略, 这显示出我们算法的可行性.
