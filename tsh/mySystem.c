#include "mySystem.h"

void ExecuteInit::execute(int i){
    sigemptyset(&blockMask);
    sigaddset(&blockMask, SIGCHLD);
    sigprocmask(SIG_BLOCK, &blockMask, &origMask);
    signal(SIGTTOU, SIG_IGN);
    nProcess = TshProcessHandler::pipeCmdline(cmdline, cmdlinei);
}


void PipeHandler::execute(int i){
    if (i != nProcess) Dup2(pipeCurrent[1], 1);  // 最后的 输出 不重定向
        close(pipeCurrent[1]);
        setpgid(0, (i == 1) ? 0 : pgid);
        Dup2(pipePrevInput, 0);  // 重定向current 输入 为上一个进程的管道的输出
        close(pipePrevInput);  // 记得要关！！不可能close 会有问题，并且也不是一个好习惯
        TshProcessHandler::redirect(argv);  // 实现重定向，事实上不必检查这个函数的结果，因为execve会检查
}

void CommandExecutor::execute(int i){
     // 这里包含命令执行逻辑
    if (argv[0][0] == '.' || argv[0][0] == '/')  //直接加载，这只是一个简单判断
            execve(argv[0], argv, environ);
    else
            execvp(argv[0], argv);  // from path to execv
    printf("%s: Command not found\n", argv[0]);
    _exit(127);
}
void MySystem::execute(int i){
    ExecuteInit init;
    init.execute(0);
    setbuf(stdout, NULL); 
    for(int i = 0; i < nProcess; i++){
        if (pipe(pipeCurrent) == -1) unix_error("pipe error \n");
            bg_t = TshProcessHandler::parseline(cmdlinei[i - 1], argv);
            if((pid = Fork()) == 0){
            SingleProcess.execute(i);
            }
        bg = max(bg, bg_t);  // 只要有一次bg我们就当这整一个进程组是后台进程了
        if (i == 1) pgid = pid;  // 现在是父进程， 如果是处理第一个子进程 ，那么 他就是组长
        setpgid(pid, pgid);      
        close(pipeCurrent[1]);
        // 建立pid 和pgid 之间的映射，为了之后的处理管道组中有多个进程，如果是c++的话，我会使用unordered_map 
        for (int i = 0; i < MAXARGS; ++i)
            if (!Groups[i].pid) {
                Groups[i].pid = pid, Groups[i].pgid = pgid;
                break;
            }

        pipePrevInput = pipeCurrent[0];   // 管道逐步向前推进 
    }
    if (bg) {
    //addjob(jobs, pgid, BG, cmdline, nProcess);
    
    AddJobCommand addjobCommand(pgid, BG, std::string(cmdline), nProcess);
    addjobCommand.execute();
    sigprocmask(SIG_SETMASK, &origMask, NULL);
    printf("[%d] (%d) %s\n", JobControlFacade::getjobpid(Jobs, pid)->jid, pid, cmdline);
    } 
    else {
    //addjob(jobs, pgid, FG, cmdline, nProcess);
    AddJobCommand addjobCommand(pgid, FG, std::string(cmdline), nProcess);
    addjobCommand.execute();
    fg_num += nProcess;
    sigprocmask(SIG_SETMASK, &origMask, NULL);  
    WaitFgCommand waitfgCommand(pgid);
    waitfgCommand.execute();
    //waitfg(pgid);
    }
}