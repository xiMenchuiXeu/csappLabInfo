# csappLabInfo

未改进前的纯c文件无change后缀
改进后的文件主要是job,signalHandler,tsh文件，有change后缀
mySystem文件主要是对原先tsh文件中的mySystem函数进行重写，但是重写并没有成功；运行出现问题

signalHandler主要采取策略模式进行重写，job主要是命令模式进行重写
tsh文件对于内置命令主要是命令模式，非内置命令主要是组合模式（重写MySystem函数but failed）
原始的代码见https://gitee.com/lin-xi-269/csapplab/tree/master/lab6tshlab
