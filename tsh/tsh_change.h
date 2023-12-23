
#include <stdio.h>
#include <map>
#include <string>
#include "base.h"
#include <iostream>
#include "signalHandler_change.h"
#include <vector>
#include "job_change.h"

/* Global variables */
extern char **environ;          /* defined in libc */
static char prompt[] = "tsh> "; /* command line prompt (DO NOT CHANGE) */
static char sbuf[MAXLINE];      /* for composing sprintf messages */
pid_t shellId;
extern Job Jobs[MAXJOBS];

extern Group Groups[MAXARGS];
extern volatile int fg_num;
#define max(a, b) ((a) > (b) ? (a) : (b))
class TshBuiltInCommand{
public:
    virtual void execute(char** argv) = 0;
};

class FgCommand : public TshBuiltInCommand{
public:
    void execute(char **argv);
};

class BgCommand : public TshBuiltInCommand{
public:
    void execute(char **argv);
};

class JobsCommand : public TshBuiltInCommand{
public:
    void execute(char **argv);
};

class QuitCommand : public TshBuiltInCommand{
public:
    void execute(char **argv);
};

class BuiltinCommandStrategyContext{
private:
    std::map<std::string, TshBuiltInCommand*>strategies;
    std :: string cmdName;
public:
    BuiltinCommandStrategyContext(char *name);

    ~BuiltinCommandStrategyContext();

    int executeBuiltInCommand(char **argv);
};


class TshProcessHandler{
private:
    static size_t split(const char *cmdline, char **argv, char delimeter)
    {
        static char buf[MAXLINE]; /*copy of the cmdline*/
        strcpy(buf, cmdline);
        size_t n = strlen(buf), argc = 0;
        for(int i = 0; i < n; i++){
            int j = i;
            while(j < n && cmdline[j] != delimeter)j++;
            buf[j] = '\0';
            argv[argc++]= buf + i;
            i = j;
        }
        argv[argc] = NULL;
        return argc;

    }
public:
    static int parseline(const char *buf, char **argv);

    static size_t pipeCmdline(const char *cmdline, char **argv);

    static size_t redirect(char **argv);

    static int builtin_cmd(char **argv);
};

class FormalTsh{
private:
    static FormalTsh* instance;

    static int readCmdline(char *cmdline)
    {
        /* Read command line */
        if ((fgets(cmdline, MAXLINE, stdin) == NULL) && ferror(stdin)) app_error("fgets error");
        if (feof(stdin)) { /* End of file (ctrl-d) */
            exit(0);
        }
        size_t n = strlen(cmdline);
        cmdline[n - 1] = '\0';
        return n;
    }
    
    static void mySystem(char *cmdline);

    static void eval(char *cmdline);

public:
    static FormalTsh* getInstance(){
        if(instance == NULL)
            instance = new FormalTsh;
        return instance;
    }
    void init(int argc, char **argv, int *emit_prompt);

    int TshRunning(int argc, char **argv);


};



class SystemComponent {
protected:
    static pid_t pid;
    static pid_t pgid;
    static size_t nProcess;
    static int bg;
    static char *cmdline;
    static char *cmdlinei[MAXARGS];
    static char *argv[MAXARGS];
    static int pipeCurrent[2];
    static int bg_t;
    static int pipePrevInput;
    static sigset_t origMask, blockMask;
public:
    virtual void execute(int i) = 0;
};
pid_t SystemComponent::pid = -1;
pid_t SystemComponent::pgid = -1;
size_t SystemComponent::nProcess = 0;
int SystemComponent::bg = -1;
int SystemComponent::bg_t = -1;
int SystemComponent::pipePrevInput = 0;
char* SystemComponent ::cmdline;
char* SystemComponent ::cmdlinei[MAXARGS];
char* SystemComponent::argv[MAXARGS];
int SystemComponent::pipeCurrent[2];
sigset_t SystemComponent::blockMask;
sigset_t SystemComponent::origMask;

class ExecuteInit : public SystemComponent{
public:
     void execute(int i);
};

class PipeHandler : public SystemComponent {
public:   
    //i代表此时处理到第几个命令了，因为存在管道
    void execute(int i);
};

class CommandExecutor : public SystemComponent {
public:
    void execute(int i);
};

class SingleCommandProcessor : public SystemComponent{
private:
    std::vector<SystemComponent*>components;
public:
    void addComponent(SystemComponent* component){
        components.push_back(component);
    }
    void execute(int i)override{
        for (auto& component : components) {
            component->execute(i);
        }
    }
    ~SingleCommandProcessor(){
        for (auto& component : components) {
            delete component;
        }
    }
};


class MySystem: public SystemComponent{
private:
    char* cmdline;
    SingleCommandProcessor SingleProcess;
public:
    MySystem(char *cmdline):cmdline(cmdline){
        SingleProcess.addComponent(new PipeHandler());
        SingleProcess.addComponent(new CommandExecutor());
    }
    void execute(int k);
};

