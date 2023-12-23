#include "tsh_change.h"


//below is the built_in command 
void FgCommand::execute(char **argv){
    JobControlFacade :: do_bg_fg(argv);
}

void BgCommand::execute(char **argv){
    JobControlFacade :: do_bg_fg(argv);
}

void JobsCommand::execute(char **argv){
    ListJobsCommand listjobsCommand;
    listjobsCommand.execute();
}

void QuitCommand::execute(char **argv){
    exit(0);
}


//built_in command pronounce end


//below is the BuiltinStrategyContext

BuiltinCommandStrategyContext ::BuiltinCommandStrategyContext(char *name){
    strategies["bg"] = new BgCommand;
    strategies["fg"] = new FgCommand;
    strategies["jobs"] = new JobsCommand;
    strategies["quit"] = new QuitCommand;
    cmdName = std::string(name);
}

BuiltinCommandStrategyContext::~BuiltinCommandStrategyContext(){
    for (auto& strategy : strategies) {
            delete strategy.second;
        }
}

int BuiltinCommandStrategyContext::executeBuiltInCommand(char **argv){
    if(strategies.find(cmdName) != strategies.end()){
            strategies[cmdName]->execute(argv);
            return 1;
        }
    return 0;
}

//BuiltinStrategyContext pronounce end 

//below is the TshProcessHandler func

int TshProcessHandler::parseline(const char *buf, char **argv){
    static char res[MAXLINE][1024];
        char knew = 1;
        size_t argc = 0;
        size_t marks = INT32_MAX;
        for (size_t i = 0, k = 0; buf[i]; ++i, knew = 1) {
            while (buf[i] && buf[i] == ' ' && marks == INT32_MAX) i++; //跳过空格
            while (buf[i] && buf[i] != '\'') {
                if (buf[i] == ' ' && marks == INT32_MAX) break; //如果遇到空格且不在单引号内，意味着一个参数完毕
                res[argc][k++] = buf[i++];   //否则参数继续
            }
            if (buf[i] == '\'') {   //如果遇到单引号
                if (marks == INT32_MAX) {
                    marks = i + 1;  //设置marks参数表明此时内容是单引号里的内容
                    knew = 0; //在对‘’里的字符串进行操作时，这个标志用来指明此时‘’里的内容还未完毕，继续进行操作
                } 
                else
                    marks = INT32_MAX; //如果遇到第二个单引号说明引号内的内容结束了
            }
            if (knew == 1){
                res[argc][k] = '\0', k = 0;//如果knew标志为1，就将第argc个单词最后加上结束符‘\0’
                argc++; //让res指向下一个存放的位置
            }
            if (!buf[i]) break; //如果字符串结尾了说明res写入完毕
            if (argc >= 100) return -1; //argv的字符串变量个数最多是100个
        }
        for (int i = 0; i < argc; ++i) argv[i] = res[i]; //将res存储的字符串值赋值到argv环境变量中
        argv[argc] = NULL;  //argv最后一个值为NULL
        if (argc == 0) return 0; 
        int bg;
        /* should the job run in the background? */
        if ((bg = (*argv[argc - 1] == '&')) != 0) { //根据命令的最后一个字符是不是为‘&’判断是否应该在后台运行
            argv[--argc] = NULL; //判断完成后将'&'从argv字符串数组中删除
        }

        return bg;
}

size_t TshProcessHandler::pipeCmdline(const char *cmdline, char **argv){
    return split(cmdline, argv, '|');
    /*return the number of commands in pipe command*/
}

size_t TshProcessHandler::redirect(char **argv){
    size_t argc = 1;  //跳过argv[0],因为argv[0]通常是
    while (argv[argc]) {
        if (!strcmp(argv[argc], "<")) {
            if (!argv[++argc]) {
                puts("tsh: syntax error near unexpected token `newline'\n");
                return 0;
            }
            int fd = open(argv[argc], O_RDONLY);
            if (fd == -1) {
                fprintf(stderr, "tsh: %s: No such file or directory", argv[argc]);
                return 0;
            }
            argv[argc - 1] = argv[argc] = NULL;
            Dup2(fd, STDIN_FILENO);
        } else if (!strcmp(argv[argc], ">")) {
            if (!argv[++argc]) {
                puts("tsh: syntax error near unexpected token `newline'\n");
                return 0;
            }
            int fd = Open(argv[argc], O_RDWR | O_CREAT, S_IRUSR | S_IWGRP | S_IWUSR | S_IRGRP);
            Dup2(fd, STDOUT_FILENO);
            argv[argc - 1] = argv[argc] = NULL;
        } else if (!strcmp(argv[argc], ">>")) {
            if (!argv[++argc]) {
                puts("tsh: syntax error near unexpected token `newline'\n");
                return 0;
            }
            int fd = open(argv[argc], O_RDWR | O_APPEND, S_IRUSR | S_IWGRP | S_IWUSR | S_IRGRP);
            if (fd == -1) {
                printf("tsh: %s: No such file or directory", argv[argc]);
                return 0;
            }
            Dup2(fd, STDOUT_FILENO);
            argv[argc - 1] = argv[argc] = NULL;
        }
        argc++;
    }
    return 1;
}



int TshProcessHandler::builtin_cmd(char **argv){
    if(!argv[0])return 0;
    char *name = argv[0];
    BuiltinCommandStrategyContext builtinExecute(name);
    return builtinExecute.executeBuiltInCommand(argv);
}


void FormalTsh::mySystem(char *cmdline){
    char *argv[MAXARGS];
    sigset_t origMask, blockMask;
    sigemptyset(&blockMask);
    /* Block SIGCHLD 为了保证addjob在deletejob之前, 且保证前台进程的终止能被检查到 */
    sigaddset(&blockMask, SIGCHLD);

    sigprocmask(SIG_BLOCK, &blockMask, &origMask);
    pid_t pid, pgid;
    signal(SIGTTOU, SIG_IGN);
    char *cmdlinei[MAXARGS];
    int bg = 0;
    setbuf(stdout, NULL);  // 取消缓冲区

    // 我们开始 取出每一个命令
    // cmdlinei 分割出来的管道中的每一个cmdline
    size_t nProcess = TshProcessHandler::pipeCmdline(cmdline, cmdlinei);
    int pipePrevInput = 0, pipeCurrent[2];  // 建立管道

    // 这里是在建立n - 1个管道，让父进程fork出 n - 1个进程，然后依次让他们重定向
    for (int i = 1; i <= nProcess; ++i) {
        if (pipe(pipeCurrent) == -1) unix_error("pipe error \n");
        int bg_t = TshProcessHandler::parseline(cmdlinei[i - 1], argv);

        if ((pid = Fork()) == 0) {
            if (i != nProcess) Dup2(pipeCurrent[1], 1);  // 最后的 输出 不重定向
            close(pipeCurrent[1]);
            setpgid(0, (i == 1) ? 0 : pgid);
            Dup2(pipePrevInput, 0);  // 重定向current 输入 为上一个进程的管道的输出
            close(pipePrevInput);  // 记得要关！！不可能close 会有问题，并且也不是一个好习惯
            TshProcessHandler::redirect(argv);  // 实现重定向，事实上不必检查这个函数的结果，因为execve会检查
            if (argv[0][0] == '.' || argv[0][0] == '/')  //直接加载，这只是一个简单判断
                execve(argv[0], argv, environ);
            else
                execvp(argv[0], argv);  // from path to execv
            printf("%s: Command not found\n", argv[0]);
            _exit(127);
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
        printf("[%d] (%d)", JobControlFacade::getjobpid(Jobs, pid)->jid, pid);
        std::cout<<cmdline<<std::endl;
    } else {
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


void FormalTsh::eval(char *cmdline){
    char *argv[MAXARGS];
    if (argv[0] == NULL) return;
    TshProcessHandler::parseline(cmdline, argv);  // 按照空格和''分割cmdline
    if (!TshProcessHandler::builtin_cmd(argv)) {  // 目前仅内置命令不支持管道
        mySystem(cmdline);
       //MySystem mysystem(cmdline);
        //mysystem.execute(0);
    }
}


void FormalTsh::init(int argc, char **argv, int *emit_prompt){
    shellId = getpid();
    /* Redirect stderr to stdout (so that driver will get all output
    * on the pipe connected to stdout) */
    dup2(1, 2);
    char c;
    /* Parse the command line */
    while ((c = getopt(argc, argv, "hvp")) != EOF) {
        switch (c) {
            case 'h': /* print help message */
                usage();
                break;
            case 'v': /* emit additional diagnostic info */
                verbose = 1;
                break;
            case 'p':             /* don't print a prompt */
                *emit_prompt = 0; /* handy for automatic testing */
                break;
            default:
                usage();
        }
    }
    
    initSignal_change();
    
    /* Initialize the job list */
    InitJobsCommand initjobsCommand;
    initjobsCommand.execute();
}

int FormalTsh::TshRunning(int argc, char **argv){
    char cmdline[MAXLINE];
    int emit_prompt = 1; /* emit prompt (default) */
    init(argc, argv, &emit_prompt);

    while (1) {
        if (emit_prompt) {
            printf("%s", prompt);
            fflush(stdout);
        }
        if (readCmdline(cmdline) == 1) continue;
        /* Evaluate the command line 核心 */
        eval(cmdline);
    }

    exit(0); /* control never reaches here */
}

FormalTsh* FormalTsh::instance = nullptr;
int main(int argc, char **argv){
    FormalTsh* tsh = FormalTsh::getInstance();
    tsh->TshRunning(argc, argv);
    return 0;
}






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
void MySystem::execute(int k){
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