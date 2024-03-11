#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_INPUT_SIZE 1024

void execute_command(char *command) {
    // 分割命令和参数
    char *args[MAX_INPUT_SIZE];
    int arg_count = 0;

    char *token = strtok(command, " ");
    while (token != NULL) {
        args[arg_count++] = token;
        token = strtok(NULL, " ");
    }
    args[arg_count] = NULL;

    // 创建子进程执行命令
    pid_t pid = fork();

    if (pid == 0) {
        // 子进程
        if (execvp(args[0], args) == -1) {
            perror("shell");
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        // 创建子进程失败
        perror("shell");
    } else {
        // 等待子进程结束
        waitpid(pid, NULL, 0);
    }
}

int main() {
    char input[MAX_INPUT_SIZE];

    while (1) {
        // 输出Shell提示符
        printf("myshell> ");
        fflush(stdout);

        // 读取用户输入
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }

        // 移除换行符
        size_t len = strlen(input);
        if (len > 0 && input[len - 1] == '\n') {
            input[len - 1] = '\0';
        }

        // 退出Shell
        if (strcmp(input, "exit") == 0) {
            break;
        }

        // 执行命令
        execute_command(input);
    }

    return 0;
}
