#ifndef JOBS_H
#define JOBS_H
#include "base.h"
#include "get_num.h"


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

struct job_t{

    pid_t pgrp; /*process group of this job*/
    int processNumber ;//进程组中还活着的进程数量
    int state;/*UNDEF,BG,FG,ST*/
    char cmdline[MAXLINE];  /*MAXLINE = 1024*/
    int jid; //job ID
};

struct Group{
    int pid;
    int pgid;
};
//group structure is a mapping of pid-->pgid
extern struct job_t jobs[MAXJOBS];
extern struct Group group[MAXARGS];

void waitfg(pid_t pid);
void clearjob(struct job_t *job);
void initjobs(struct job_t *jobs);
int maxjid(struct job_t *jobs);
int addjob(struct job_t *jobs, pid_t pgrp, int state, char *cmdline,
           int processNumber);
int deletejob(struct job_t *jobs, pid_t pid);
pid_t fgpid(struct job_t *jobs);
struct job_t *getjobpid(struct job_t *jobs, pid_t pid);
struct job_t *getjobjid(struct job_t *jobs, int jid);
int pid2jid(pid_t pid);
void listjobs(struct job_t *jobs);
int is_fg_pid(pid_t pid);
int do_bg_fg(char **argv);
void waitfg(pid_t pid);
#endif
