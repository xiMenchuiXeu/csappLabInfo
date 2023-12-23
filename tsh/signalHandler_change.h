#ifndef SIGNALHANDLE_CHANGE
#define SIGNALHANDLE_CHANGE
#include <iostream>
#include <csignal>
#include <unistd.h>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include "sio.h"
#include "base.h"
#include "job_change.h"
extern pid_t shellId;
typedef void handler_t(int);
extern Job Jobs[MAXJOBS];
extern Group Groups[MAXARGS];
//common signal function wrapper
class SignalOperations {
public:
    static void put_help(int jid, int pgid, char *c, int singal_number)
    {
        Sio_puts("Job");
        Sio_puts(" [");
        Sio_putl(jid);
        Sio_puts("]");
        Sio_puts(" (");
        Sio_putl(pgid);
        Sio_puts(") ");
        Sio_puts(c);
        Sio_puts(" by signal ");
        Sio_putl(singal_number);
        Sio_puts("\n");
    }
    static void Sigprocmask(int how, const sigset_t *set, sigset_t *oldset)
    {
        if (sigprocmask(how, set, oldset) < 0) unix_error("Sigprocmask error");
        return;
    }

    static void Sigemptyset(sigset_t *set)
    {
        if (sigemptyset(set) < 0) unix_error("Sigemptyset error");
        return;
    }

    static void Sigfillset(sigset_t *set)
    {
        if (sigfillset(set) < 0) unix_error("Sigfillset error");
        return;
    }

    static void Sigaddset(sigset_t *set, int signum)
    {
        if (sigaddset(set, signum) < 0) unix_error("Sigaddset error");
        return;
    }

    static void Sigdelset(sigset_t *set, int signum)
    {
        if (sigdelset(set, signum) < 0) unix_error("Sigdelset error");
        return;
    }

    static int Sigismember(const sigset_t *set, int signum)
    {
        int rc;
        if ((rc = sigismember(set, signum)) < 0) unix_error("Sigismember error");
        return rc;
    }

    static int Sigsuspend(const sigset_t *set)
    {
        int rc = sigsuspend(set); /* always returns -1 */
        if (errno != EINTR) unix_error("Sigsuspend error");
        return rc;
    }

    // 其他必要的函数...
};



//strategy pattern for signal-handler
class SignalHandlerStrategy {
public:
    virtual void handle(int signal) = 0;
    static SignalOperations sigop;
};

class SigIntHandler : public SignalHandlerStrategy {
public:
    void handle(int signal);
};

class SigChildHandler : public SignalHandlerStrategy {
public:
    void handle(int signal);
};

class SigStopHandler : public SignalHandlerStrategy {
public:
    void handle(int signal);
};

class SigQuitHandler : public SignalHandlerStrategy {
public:
    void handle(int signal);
};


/*below is the strategy context class*/
class SignalHandlerContext {
private:
    std::unordered_map<int, SignalHandlerStrategy*> strategyMap;

public:
    static SignalOperations sigop;
    static SignalHandlerContext& getInstance() {
        static SignalHandlerContext instance;
        return instance;
    }

    void setStrategy(int signal, SignalHandlerStrategy* strategy) {
        strategyMap[signal] = strategy;
        Signal(signal, signalHandler);
        //这样设计的目的是将信号处理逻辑从单个大型函数中解耦出来，使得不同的信号处理可以分别在不同的策略类中实现，
        //提高了代码的模块化和可维护性。每种信号的具体处理逻辑被封装在对应的策略类中，而 signalHandler 仅作为一个分发器（dispatcher）来调用相应的策略。
    }
    
private:
    static void signalHandler(int signal) {
        auto& instance = getInstance();
        if (instance.strategyMap.find(signal) != instance.strategyMap.end()) {
            instance.strategyMap[signal]->handle(signal);
        }
    }

    static handler_t* Signal(int signum, handler_t* handler) {
        struct sigaction action, old_action;

        action.sa_handler = handler;
        sigemptyset(&action.sa_mask);
        action.sa_flags = SA_RESTART;

        if (sigaction(signum, &action, &old_action) < 0)
            unix_error("Signal error");
        return (old_action.sa_handler);
    }
};

void initSignal_change();

#endif