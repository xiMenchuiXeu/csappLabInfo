

#define max(a, b) ((a) > (b) ? (a) : (b))
#include <vector>
extern "C" char **environ;
extern volatile int fg_num;
extern Group Groups[MAXARGS];
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

