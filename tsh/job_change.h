#ifndef JOBS_CHANGEH
#define JOBS_CHANGEH
#include "base.h"
#include "get_num.h"
#include <string>
#include <vector>
#include <iostream>
/*
如何理解job here:
csapp defination:
Unix shell 使用作业(job)这个抽象概念来表示为对一条命令行求值而创建的进程组。在任何时刻，至多只有
**一个前台作业(job)和0 or 多个后台作业**；
from CSAPP page529
*/


/*job states*/
#define UNDEF 0 /*undefined??*/
#define STOP 0X7f   /*running in foreground???*/
#define CONTINUE 0xffff /*running in background???*/
#define BG 1
#define FG 2
#define MAXJOBS 16 /*max jobs at any point in time*/
#define MAXJID 1 << 16 /*max job ID*/
#define MAXLINE 1024
#define MAXARGS     128 /* max args on a command line */
/*
 * Jobs states: FG (foreground), BG (background), ST (stopped)
 * Job state transitions and enabling actions:
 *     FG -> ST  : ctrl-z
 *     ST -> FG  : fg command
 *     ST -> BG  : bg command
 *     BG -> FG  : fg command
 * At most 1 job can be in the FG state.
 */



extern int nextjid; /*next job ID to allocate*/
extern size_t jobmru; // 上一个运行的job
extern size_t curjob; //当前运行的job
extern int verbose;
extern volatile int fg_num;

class Job {
public:
    pid_t pgrp;              // process group of this job
    int processNumber;       // 进程组中还活着的进程数量
    int state;               // UNDEF, BG, FG, ST
    std::string cmdline;     // MAXLINE = 1024
    int jid;                 // job ID

};

class Group{
public:
    int pid;
    int pgid;
};

extern Job Jobs[MAXJOBS];
extern Group Groups[MAXARGS];


class JobCommand {
public:
    virtual void execute() = 0;
};

class WaitFgCommand : public JobCommand{/*Waits for a foreground job to complete.*/
private:
    pid_t pgid;

public:
    WaitFgCommand(pid_t pgid):pgid(pgid){};
    void execute();
};

class ClearJobCommand : public JobCommand{
private:
    Job *job;
public:
    ClearJobCommand(Job *job) : job(job){}

    void execute();
};

class InitJobsCommand : public JobCommand{

public:
    void execute();
};

class FindMaxJidCommand : public JobCommand{
private:
    int maxjid = 0;
public:
    void execute();
        
    int getMaxJid() const {
        return maxjid;  // 提供一个方法来获取最大 JID;先execute后再调用getMaxJid
    }
};

class JobControlFacade{
public:
    static int restartJob(Job *job, int state){
        if(job == NULL)return -1;
        //if(state == FG )fg_num ++;
        if(state == FG && job->state != FG)fg_num += job->processNumber;
        if(job->state == UNDEF)return -1;
        if(job->state == BG && state == BG)return 0;//如果作业的状态是bg,并且重启请求的状态也是BG,那么不进行任何操作
        kill(-job->pgrp, SIGCONT);//向job所在的进程组发出信号，SIGCONT表明继续进程如果该进程停止
        job->state = state;
        return 1;
    }
    
    static int pid2jid(pid_t pid){
        if(pid < 0)return 0;
        for(int i = 0; i < MAXJOBS; i++)
            if(Jobs[i].pgrp == pid)return Jobs[i].jid;
        return 0;
    }

    static int bg_fg_read_pos(char **argv)//从argv中读出pid or job id
    {
        int pos;
        if(argv[1] == NULL || argv[1][0] == '\0')
            return -1;
        else if(isdigit(argv[1][0])){
            pos = (size_t)pid2jid(getInt(argv[1], 0, "bg_fg_read_pos"));
            if(pos == 0)return -2; // no such process;
        }
        else if(argv[1][0] == '%'){
            pos = (size_t)getInt(argv[1] + 1, 0, "bg_fg_read_pos");
            if(pos >= nextjid || pos == 0)return -3;// no such job 
        }
        else return -4; //not digit number
        return pos;
    }

public:
    static int do_bg_fg(char **argv);  //this func needs to check again!!!
    
    
    static Job *getjobpid(Job *jobs, pid_t pid);

    /*getjobjid :find a job by jid on the job list*/
    static Job *getjobjid(Job *jobs, int jid);
  
};

class JobFactory {
public:
    static Job createJob(pid_t pgrp, int processNumber,int state, const std::string& cmdline) {
        Job job;
        job.pgrp = pgrp;
        job.processNumber = processNumber;
        job.state = state;
        job.cmdline = cmdline;
        // 初始化其他属性
        return job;
    }

    static class Job *FindFgGroup(){
        pid_t pid, pgid;
        int i = 0;
        for (; i < MAXJOBS; ++i)
            if (Jobs[i].state == FG) {
                pgid = Groups[i].pgid;
                break;
            }
        return JobControlFacade::getjobpid(Jobs, pgid);
    }
};



extern JobFactory Jobmake; //jobmake是一个工厂实例化对象用来make job object


class AddJobCommand : public JobCommand{
private:
    Job job;
public:
    AddJobCommand(pid_t pgrp, int state, std::string cmdline, int processNumber){
        job = Jobmake.createJob(pgrp, processNumber, state, cmdline);
    }
    void execute();
    
};

class DeleteJobCommand : public JobCommand{
private:
    pid_t pgrp;

public:
    DeleteJobCommand(pid_t pgrp):pgrp(pgrp){}
    
    void execute();
    
};

class ListJobsCommand : public JobCommand{
public:
    void execute();
};








//c++中静态成员函数不依赖于类的实例，可以通过类名直接调用


#endif