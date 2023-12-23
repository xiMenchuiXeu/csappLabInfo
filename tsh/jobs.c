
#include "jobs.h"

#include<ctype.h>
#include<stdint.h>
#include<stdlib.h>
#include<string.h>

#include "sio.h"

struct Group group[MAXARGS];
struct job_t jobs[MAXJOBS]; /* The job list */
int nextjid = 1;
size_t curjob = 0;

int verbose = 0;/*if true, print additional output*/
size_t jobmru = 0;

volatile int fg_num = 0; /*前台中运行进程个数*/
extern pid_t shellId;  /*extern 关键字，定义了一个全局变量，并且希望在其他文件中使用这个变量*/

void waitfg(pid_t pgid)/*Waits for a foreground job to complete.*/
{
    //tcsetpgrp函数用于设置与终端相关联的前台进程组ID
    tcsetpgrp(0, pgid); //将控制终端的前台进程组设置为pgid
    //将文件描述符 0（通常代表标准输入，即 stdin）关联的终端的前台进程组设置为 pgid 指定的进程组ID。
    while(fg_num)pause();//等待前台作业完成

    tcsetpgrp(0, shellId);//将终端的控制权从前台进程pgid转移到shellID进程
    return;
}
//restart foreground job which stopped by entering "ctrl + z"
static int restartjob(struct job_t *job, int state)//static关键字表面该函数只在定义它的文件内可见
{
    if(job == NULL)return -1;
    //if(state == FG )fg_num ++;
    if(state == FG && job->state != FG)fg_num += job->processNumber;
    if(job->state == UNDEF)return -1;
    if(job->state == BG && state == BG)return 0;//如果作业的状态是bg,并且重启请求的状态也是BG,那么不进行任何操作
    kill(-job->pgrp, SIGCONT);//向job所在的进程组发出信号，SIGCONT表明继续进程如果该进程停止
    job->state = state;
    return 1;
}
// map process ID to job ID
inline int pid2jid(pid_t pid)
{
    if(pid < 0)return 0;
    for(int i = 0; i < MAXJOBS; i++)
        if(jobs[i].pgrp == pid)return jobs[i].jid;
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
//find job by pid on the job list
inline struct job_t *getjobpid(struct job_t *jobs, pid_t pid)
{
    if(pid < 1)return NULL;
    for(int i = 0; i < MAXJOBS; i++)
        if(jobs[i].pgrp == pid)return &jobs[i];
    return NULL;
}

/*getjobjid :find a job by jid on the job list*/
inline struct job_t *getjobjid(struct job_t *jobs, int jid)
{
    if(jid < 1)return NULL;   //jid start from 1 ? !
    for(int i = 0; i < MAXJOBS; i++){
        if(jobs[i].jid == jid)return &jobs[i];
    }
    return 0;
}
/**/
int do_bg_fg(char **argv)   //this func needs to check again!!!
{
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
    struct job_t *job = getjobjid(jobs, pos);// 得到相对应的job
    res = restartjob(job, state);   //相当于唤醒这个组;将job以state状态进行restart
    if(state == BG){  //后台可以直接返回
        printf("[%d] (%d) %s\n", job->jid, job->pgrp, job->cmdline);
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
    waitfg(job->pgrp);//将前台与job相联系
    return 0;
}
/* clearjob - Clear the one of the entries in a job struct */
void clearjob(struct job_t *job){
    job->processNumber = 0;//进程组中还活着的进程数量
    job->pgrp = 0;
    job->jid = 0;
    job->state = UNDEF;
    job->cmdline[0] = '\0';
}
/*initialize the job list*/
void initjobs(struct job_t *jobs){
    for(int i = 0; i < MAXJOBS; i++)
        clearjob(&jobs[i]);
}
/*return max allocated job id*/
int maxjid(struct job_t*jobs){
    int i, max = 0;
    for(i = 0; i < MAXJOBS; i++)
        if(jobs[i].jid > max)max = jobs[i].jid;
    return max;
}
/*add new job operation ,successfully return 1, else 0*/
int addjob(struct job_t *jobs, pid_t pgrp, int state, char *cmdline, int processNumber)
{
    if(pgrp < 1)return 0;
    for(int i = 0; i < MAXJOBS; i++){
        if(jobs[i].pgrp == 0){/*represent empty location*/
            jobs[i].pgrp = pgrp;
            jobs[i].state = state;
            jobs[i].processNumber = processNumber;
            jobs[i].jid = nextjid++;
            if(nextjid >= MAXJOBS)nextjid = 1; // circle operation
            strcpy(jobs[i].cmdline, cmdline);
            if(verbose){
                //verbose mode
                printf("Added job [%d] %d %s\n", jobs[i].jid, jobs[i].pgrp, jobs[i].cmdline);
            }
            return 1;
        }
    }
    return 0;
}

//delete job successfully return 1 else return 0;
int deletejob(struct job_t *jobs, pid_t pgrp)
{
    int i;
    if(pgrp < 1)return 0;

    for(i = 0; i < MAXJOBS; i++){
        if(jobs[i].pgrp == pgrp){
            if(verbose){
                //verbose mode
                printf("delete job [%d] %d %s\n", jobs[i].jid, jobs[i].pgrp, jobs[i].cmdline);
            }
            clearjob(&jobs[i]);
            nextjid = maxjid(jobs) + 1;
            
            return 1;
        }
    }
    return 0;
}

void listjobs(struct job_t *jobs){
    int i;
    for(i = 0; i < MAXJOBS; i++){
        if(jobs[i].pgrp != 0){//jobs[i].pgrp == 0 means empty place
            printf("[%d] (%d)", jobs[i].jid, jobs[i].pgrp);
            switch(jobs[i].state){
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
                    printf("listjobs: Internal error: job[%zd].state=%d ", i, jobs[i].state);
            }
            printf("%s\n", jobs[i].cmdline);
        }
    }
}


/*job function routines ends*/