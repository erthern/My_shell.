#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <limits.h>

#define MAX_COMMAND_LENGTH 100
#define MAX_ARGS 10

// ANSI颜色转义序列
#define COLOR_GREEN "\033[0;32m"
#define COLOR_RESET "\033[0m"

void echo(const char *message) {
    printf("%s\n", message);
}

void change_directory(const char *path) {
    if (path == NULL) {
        fprintf(stderr, "目录路径为空\n");
        return;
    }

    if (strcmp(path, "-") == 0) {
        // 切换到上次所在的目录
        const char *previous_directory = getenv("OLDPWD");
        if (previous_directory == NULL) {
            fprintf(stderr, "找不到上次所在的目录\n");
            return;
        }
        if (chdir(previous_directory) != 0) {
            perror("chdir");
            return;
        }
    } else {
        // 切换到指定的目录
        if (chdir(path) != 0) {
            perror("chdir");
            return;
        }
    }

    // 更新环境变量OLDPWD为当前所在的目录
    char current_directory[4096];
    if (getcwd(current_directory, sizeof(current_directory)) == NULL) {
        perror("getcwd");
        return;
    }
    if (setenv("OLDPWD", getenv("PWD"), 1) != 0) {
        perror("setenv");
        return;
    }
    if (setenv("PWD", current_directory, 1) != 0) {
        perror("setenv");
        return;
    }
}

void execute_command(char *command, char *args[], char *output_file) {
    if (strcmp(command, "cd") == 0) {
        change_directory(args[1]);
    } else if (strcmp(command, "exit") == 0) {
        // 退出 Shell
        exit(0);
    } else if (strcmp(command, "grep") == 0) {
        // 创建子进程执行命令
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // 子进程
            // 输出重定向到文件
            if (output_file != NULL) {
                freopen(output_file, "w", stdout);
            }
            // 调用 execvp 执行 grep 命令
            execvp(command, args);
            perror("");
            exit(EXIT_FAILURE);
        } else {
            // 父进程
            int status;
            waitpid(pid, &status, 0);
            if (WIFEXITED(status)) {
                int exit_status = WEXITSTATUS(status);
                printf("子进程退出，退出状态码：%d\n", exit_status);
            } else if (WIFSIGNALED(status)) {
                int term_signal = WTERMSIG(status);
                printf("子进程被信号终止，信号编号：%d\n", term_signal);
            }
        }
    } else if (strcmp(command, "echo") == 0) {
        // 处理 echo 命令
        char *message = args[1];
        echo(message);
    } else {
        // 其他命令
        // 创建子进程执行命令
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // 子进程
            // 输出重定向到文件
            if (output_file != NULL) {
                freopen(output_file, "w", stdout);
            }
            (command, args);
            perror("");
            exit(EXIT_FAILURE);
        } else {
            // 父进程
            int status;
            waitpid(pid, &status, 0);
            if (WIFEXITED(status)) {
                int exit_status = WEXITSTATUS(status);
                printf("子进程退出，退出状态码：%d\n", exit_status);
            } else if (WIFSIGNALED(status)) {
                int term_signal = WTERMSIG(status);
                printf("子进程被信号终止，信号编号：%d\n", term_signal);
            }
        }
    }
}

int main() {
    while (1) {
        char command[MAX_COMMAND_LENGTH];
        char *args[MAX_ARGS] = { NULL };
        char output_file[4096];

        // 获取当前工作目录
        char current_directory[4096];
        if (getcwd(current_directory, sizeof(current_directory)) == NULL) {
            perror("getcwd");
            exit(EXIT_FAILURE);
        }

        // 获取用户名
        char *username = getenv("USER");

        // 打印提示符（用户名和当前路径）
        printf(COLOR_GREEN "%s@%s:" COLOR_RESET "%s$ ", username, getenv("HOSTNAME"), current_directory);

        fgets(command, sizeof(command), stdin);
        command[strcspn(command, "\n")] = '\0';

        // 解析命令和参数
        char *token = strtok(command, " ");
        int arg_index = 0;
        while (token != NULL && arg_index < MAX_ARGS - 1) {
            args[arg_index] = token;
            arg_index++;
            token = strtok(NULL, " ");
        }
        args[arg_index] = NULL;

        // 检查是否有输出重定向符号 ">"
        int redirect_output = 0;
        for (int i = 0; i < arg_index; i++) {
            if (strcmp(args[i], ">") == 0) {
                if (i + 1 < arg_index) {
                    strcpy(output_file, args[i + 1]);
                    redirect_output = 1;
                    break;
                }
            }
        }

        // 执行命令
        execute_command(args[0], args, redirect_output ? output_file : NULL);

        // 更新当前工作目录
        if (getcwd(current_directory, sizeof(current_directory)) == NULL) {
            perror("getcwd");
            exit(EXIT_FAILURE);
        }

        // 更新提示符中的路径
        if (setenv("PWD", current_directory, 1) != 0) {
            perror("setenv");
            exit(EXIT_FAILURE);
        }
    }

    return 0;
}