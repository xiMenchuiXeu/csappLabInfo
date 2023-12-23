#include "job_change.h"
#include<iostream>
int nextjid = 1;
size_t curjob = 0;

int verbose = 0;/*if true, print additional output*/
size_t jobmru = 0;

volatile int fg_num = 0; /*前台中运行进程个数*/
extern pid_t shellId;  /*extern 关键字，定义了一个全局变量，并且希望在其他文件中使用这个变量*/
Job Jobs[MAXJOBS];
Group Groups[MAXARGS];
JobFactory Jobmake;
void WaitFgCommand ::execute(){
    //tcsetpgrp函数用于设置与终端相关联的前台进程组ID
    tcsetpgrp(0, pgid); //将控制终端的前台进程组设置为pgid
    //将文件描述符 0（通常代表标准输入，即 stdin）关联的终端的前台进程组设置为 pgid 指定的进程组ID。
    while(fg_num)pause();//等待前台作业完成

    tcsetpgrp(0, shellId);//将终端的控制权从前台进程pgid转移到shellID进程
    return;
}

void ClearJobCommand ::execute(){
    job->processNumber = 0;//进程组中还活着的进程数量
    job->pgrp = 0;
    job->jid = 0;
    job->state = UNDEF;
    job->cmdline[0] = '\0';
}

void InitJobsCommand :: execute(){
    for(int i = 0; i < MAXJOBS; i++){
        ClearJobCommand clearCommand(&Jobs[i]);
        clearCommand.execute();
    }
}

void FindMaxJidCommand :: execute(){
    int i;
    for(i = 0; i < MAXJOBS; i++){
        if(Jobs[i].jid > maxjid)
            maxjid = Jobs[i].jid;
    }
}

void AddJobCommand ::execute(){
    if(job.pgrp < 1)return;
    for(int i = 0; i < MAXJOBS; i++){
        if(Jobs[i].pgrp == 0){/*represent empty location*/
            Jobs[i].pgrp = job.pgrp;
            Jobs[i].state = job.state;
            Jobs[i].processNumber = job.processNumber;
            Jobs[i].jid = nextjid++;
            if(nextjid >= MAXJOBS)nextjid = 1; // circle operation
            Jobs[i].cmdline = job.cmdline;
            if(verbose){
                //verbose mode
                printf("Added job [%d] %d %s\n", Jobs[i].jid, Jobs[i].pgrp, Jobs[i].cmdline);
            }
            return ;
        }
    }
    return;
}

void DeleteJobCommand ::execute(){
    int i;
    if(pgrp < 1)return ;

    for(i = 0; i < MAXJOBS; i++){
        if(Jobs[i].pgrp == pgrp){
            if(verbose){
                //verbose mode
                printf("delete job [%d] %d %s\n", Jobs[i].jid, Jobs[i].pgrp, Jobs[i].cmdline);
            }
            ClearJobCommand clearCommand(&Jobs[i]);
            clearCommand.execute();
            FindMaxJidCommand FindMaxJid;
            FindMaxJid.execute();
            nextjid = FindMaxJid.getMaxJid() + 1;
            
            return ;
        }
    }
    return ;
}


void ListJobsCommand ::execute(){
    int i;
    for(i = 0; i < MAXJOBS; i++){
        if(Jobs[i].pgrp != 0){//jobs[i].pgrp == 0 means empty place
            printf("[%d] (%d)", Jobs[i].jid, Jobs[i].pgrp);
            switch(Jobs[i].state){
                case BG:
                    printf("BG Running");
                    break;
                case FG:
                    printf("FG Running");
                    break;
                case STOP:
                    printf("Stopped");
                    break;
                default:
                    printf("listjobs: Internal error: job[%zd].state=%d ", i, Jobs[i].state);
            }
            std::cout << Jobs[i].cmdline << std::endl;
        }
    }
}

int JobControlFacade ::do_bg_fg(char **argv){
    int pos = bg_fg_read_pos(argv); //get jid
    if(pos == -1){
        printf("%s command requires PID or %%jobid argument\n", argv[0]);
        return -1;
    }
    if (pos == -2) {  // process超过范围
        printf("(%s): No such process\n", argv[1]);
        return -1;
    }
    if (pos == -3) {  // job超过范围
        printf("%s: No such job\n", argv[1]);
        return -1;
    }
    if (pos == -4) {  // 不是数字
        printf("%s: argument must be a PID or %%jobid\n", argv[0]);
        return -1;
    }
    //above is error handler
    int res, state = (!strcmp(argv[0], "bg") ? BG : FG); //确定命令行中明确是bg or fg process
    Job *job = getjobjid(Jobs, pos);// 得到相对应的job
    res = restartJob(job, state);   //相当于唤醒这个组;将job以state状态进行restart
    if(state == BG){  //后台可以直接返回
        printf("[%d] (%d) ", job->jid, job->pgrp);
        std::cout<<job->cmdline<<std::endl;
        return 0;
    }
    if(res == -1){
        printf("%s: no such job\n", argv[1]);
        return -1;
    }
    else if(res == 0){//state == BG && job->state == BG
        printf("tsh: %s: job %d already in %s\n", argv[0],job->jid,
                state == BG ? "BACKGROUND": "FOREGROUND"
        );
        return -1;
    }
    jobmru = curjob;
    curjob = (size_t)pos;
    WaitFgCommand waitfgCommand(job->pgrp);
    waitfgCommand.execute();//将前台与job相联系
    
    return 0;
    
}

Job* JobControlFacade::getjobpid(Job *jobs, pid_t pid){
    if(pid < 1)return NULL;
    for(int i = 0; i < MAXJOBS; i++)
        if(jobs[i].pgrp == pid)return &jobs[i];
    return NULL;
}

Job* JobControlFacade::getjobjid(Job *jobs, int jid){
    if(jid < 1)return NULL;   //jid start from 1 ? !
    for(int i = 0; i < MAXJOBS; i++){
        if(jobs[i].jid == jid)return &jobs[i];
    }
    return NULL;
}