# Graph 500



## 报告里可以加的内容

1. 对生成的graph分析, 例如: 打印degree分布, 每一层点的个数.
2. 针对bottom-up和top-down之间switch算法的分析. 用我的heuristic和文中的自动机有什么区别?
3. bottom-up我的实现, 例如我用了什么数据结构.
4. 访存用SIMD优化?
5. 将local visited改成global之后用top-down变慢, 说明cache locality变差.
6. 加入状态机后对$\alpha, \beta$调参, 显然最优解独立, 用二分法画图找到最优点.