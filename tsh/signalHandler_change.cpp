#include <iostream>
#include <csignal>
#include <unistd.h>
#include <vector>
#include <algorithm>

#include "signalHandler_change.h"
#include "base.h"
#include "job_change.h"


/*signal strategy decribe*/
void SigChildHandler :: handle(int sig){
    int olderrno = errno, status;
    sigset_t mask_all, prev_all;

    sigop.Sigfillset(&mask_all);//信号处理程序在对全局变量进行访问修改时，需要阻塞所有的信号，从而保护共享全局数据结构
    pid_t pid, pgid;
    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED)) >
           0) {  // 循环等待子进程退出，确保不产生僵尸进程
           //组合这些选项的效果是，waitpid 将非阻塞地检查任何一个子进程的状态，不管它是已经结束、被停止或者是继续执行。如果调用时没有子进程的状态改变，waitpid 将立即返回 0。
        int i = 0;
        for (; i < MAXARGS; ++i)
            if (pid == Groups[i].pid) {
                pgid = Groups[i].pgid;
                break;
            }
        Job *job = JobControlFacade::getjobpid(Jobs, pgid);
        // exited
        if (WIFEXITED(status) || WIFSIGNALED(status)) {
            Groups[i].pid = 0;                // delete pid
            if (job->state == FG) fg_num--;  // 前台进程数量 --
            if (--job->processNumber > 0) continue;  // 如果这个进程组 还有进程，就稍微等待一下
            if (WIFSIGNALED(status))
                sigop.put_help(JobControlFacade::pid2jid(pgid), pgid, "terminated", 2);  // 死于信号，发个信息给终端让用户看

            sigop.Sigprocmask(SIG_BLOCK, &mask_all, &prev_all);  // signal atomic 删除这个进程组
            //deletejob(jobs, pgid);
            DeleteJobCommand deleteCommand(pgid);
            deleteCommand.execute();
            sigop.Sigprocmask(SIG_SETMASK, &prev_all, NULL);
            // stop
        } else if (WIFSTOPPED(status)) {
            sigop.put_help(JobControlFacade::pid2jid(pgid), pgid, "stopped", 20);
            if (job->state == FG) fg_num--;
            job->state = STOP;
            // continue
        } else if (WIFCONTINUED(status)) {
            // 事实上在这里也可以这么处理:  if (job->state == FG) fg_num++;
            // 但是考虑到前台进程一定是由shell指定的，所以不会是被其他进程设定
            // 我们在shell处理好fg即可，如果这么处理的话，应该在fg的时候要考虑，要保证信号先到然后再等待，比较难以实现
        }
    }
    if (pid == -1 && errno != ECHILD) Sio_error("waitpid error"); //errno != ECHILD meaning that no more children
    // 如果waitpid调用失败，并且失败的原因不是因为没有子进程；；；没有子进程的话，waitpid返回-1
    errno = olderrno;
}

void SigStopHandler :: handle(int sig){
    Job *job = JobFactory::FindFgGroup();
    kill(-job->pgrp, SIGSTOP);  // 暂停这一个组
}

void SigIntHandler :: handle(int sig){
    Job *job = JobFactory::FindFgGroup();
    kill(-job->pgrp, SIGINT);  // 杀掉这一个组
}

void SigQuitHandler :: handle(int sig){
    Sio_puts("Terminating after receipt of SIGQUIT signal\n");
    exit(1);  // to do
}

/*signal strategy describe end*/

/*signal-strategy context class comlete*/

void initSignal_change(){
    SignalHandlerContext& signal_init = SignalHandlerContext::getInstance();
    
    signal_init.setStrategy(SIGINT, new SigIntHandler);
    signal_init.setStrategy(SIGTSTP,new SigStopHandler);
    signal_init.setStrategy(SIGCHLD, new SigChildHandler);
    signal_init.setStrategy(SIGQUIT, new SigQuitHandler);

}

/*

int main() {
    // 获取 SignalHandlerContext 的实例
    SignalHandlerContext& context = SignalHandlerContext::getInstance();

    // 设置不同信号的处理策略
    context.setStrategy(SIGINT, new SigIntHandler());
    context.setStrategy(SIGTSTP, new SigTstpHandler());
    context.setStrategy(SIGCHLD, new SigChldHandler());
    context.setStrategy(SIGQUIT, new SigQuitHandler());
    // 为其他需要的信号设置策略...

    // 主程序逻辑
    while (true) {
        // 程序的主要工作...
        // 可以是命令行界面、事件循环等
    }

    return 0;
}



*/
