#include "tools.h"

#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>

#include "base.h"
#define MAXLINE 1024
//这是一个按照空格和''分割buf（c_str）的函数，每一个分割出来的string的首地址会存入到argv[]中，最后一个*argv为NULL
// 注意：返回的首地址存放再static char中，所以第二次调用会覆盖前一次的结果！
//return : 1表示有BG输入，0表示没有
int parseline(const char *buf, char **argv)
{
    static char res[MAXLINE][MAXPARSE];
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
static inline size_t split(const char *cmdline, char **argv, char delimeter)
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
//split函数将cmdline字符串中的delimiter字符用‘\0’进行替换，并且将格式化后的结果保存在argv数组中
//管道允许上一个进程的输出成为下一个进程的输入
size_t pipeCmdline(const char *cmdline, char **argv){
    return split(cmdline, argv, '|');
    /*return the number of commands in pipe command*/
}
//从命令后的空格开始寻找'<' or '>' 重定向成功返回1 出错返回 0
// 重定向函数
size_t redirect(char **argv)
{
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


