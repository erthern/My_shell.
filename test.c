// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <unistd.h>
// #include <fcntl.h>
// #include <sys/wait.h>
// #include <sys/stat.h>
// #include <unistd.h>
// #include <limits.h>

// #define MAX_COMMANDS 10
// #define MAX_ARGS 10
// #define MAX_COMMAND_LENGTH 100
// #define MAX_COMMAND_LENGTH 100
// #define MAX_ARGS 10
// #define COLOR_GREEN "\033[0;32m"
// #define COLOR_RESET "\033[0m"

// void execute_command(char *command[], int redirect_index, char *redirect_file) {
//     int pid = fork();
//     if (pid == 0) {
//         // 子进程
//         if (redirect_index >= 0) {
//             // 存在重定向符号
//             if (redirect_index == 0) {
//                 // 输出重定向
//                 int fd = open(redirect_file, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
//                 dup2(fd, STDOUT_FILENO);
//                 close(fd);
//             } else if (redirect_index == 1) {
//                 // 输入重定向
//                 int fd = open(redirect_file, O_RDONLY);
//                 dup2(fd, STDIN_FILENO);
//                 close(fd);
//             }
//         }
//         execvp(command[0], command);
//         perror("execvp failed");
//         exit(1);
//     } else if (pid > 0) {
//         // 父进程
//         wait(NULL);
//     } else {
//         perror("fork failed");
//     }
// }

// int parse_command(char *command, char *commands[MAX_COMMANDS][MAX_ARGS], char *redirect_files[MAX_COMMANDS]) {
//     int command_count = 0;
//     char *token = strtok(command, "|");
//     while (token != NULL) {
//         int redirect_index = -1;
//         char *redirect_file = NULL;
//         char *arg_token = strtok(token, " ");
//         int arg_count = 0;
//         while (arg_token != NULL) {
//             if (strcmp(arg_token, ">") == 0) {
//                 // 输出重定向
//                 redirect_index = 0;
//                 redirect_file = strtok(NULL, " ");
//             } else if (strcmp(arg_token, "<") == 0) {
//                 // 输入重定向
//                 redirect_index = 1;
//                 redirect_file = strtok(NULL, " ");
//             } else {
//                 // 命令参数
//                 commands[command_count][arg_count] = arg_token;
//                 arg_count++;
//             }
//             arg_token = strtok(NULL, " ");
//         }
//         commands[command_count][arg_count] = NULL; // 参数数组的末尾设为NULL，表示参数列表的结束
//         redirect_files[command_count] = redirect_file;
//         command_count++;
//         token = strtok(NULL, "|");
//     }
//     return command_count;
// }

// int main() {
//     char command[MAX_COMMAND_LENGTH];
//     char *commands[MAX_COMMANDS][MAX_ARGS];
//     char *redirect_files[MAX_COMMANDS];

    
//     while (1) {
//         // 获取当前工作目录
//         char current_directory[4096];
//         if (getcwd(current_directory, sizeof(current_directory)) == NULL) {
//             perror("getcwd");
//             exit(EXIT_FAILURE);
//         }

//         // 获取用户名
//         char *username = getenv("USER");

//         // 打印提示符（用户名和当前路径）
//         printf(COLOR_GREEN "%s@%s:" COLOR_RESET "%s$ ", username, getenv("HOSTNAME"), current_directory);
//         fgets(command, sizeof(command), stdin);
//         command[strcspn(command, "\n")] = '\0'; // 去除换行符

//         int command_count = parse_command(command, commands, redirect_files);

//         if (strcmp(commands[0][0], "exit") == 0) {
//             // 退出shell
//             break;
//         } else if (strcmp(commands[0][0], "cd") == 0) {
//             // 改变当前工作目录
//             if (command_count == 1) {
//                 chdir(getenv("HOME"));
//             } else {
//                 chdir(commands[0][1]);
//             }
//         } else {
//             // 执行命令
//             int i;
//             for (i = 0; i < command_count; i++) {
//                 execute_command(commands[i], i < command_count - 1 ? 0 : -1, redirect_files[i]);
//             }
//         }
//     }

//     return 0;
// }
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h> 

#define MAX_COMMAND_LENGTH 100
#define MAX_ARGS 10
#define MAX_ARGUMENTS 100
enum RedirectType {
    NO_REDIRECT,
    OUTPUT_REDIRECT,
    INPUT_REDIRECT,
    APPEND_REDIRECT
};

void execute_commands(char *commands[], int num_commands, enum RedirectType redirect_type, char *output_file) {
    int pipefd[2];
    int prev_pipe = STDIN_FILENO; // 初始输入为标准输入

    for (int i = 0; i < num_commands; i++) {
        pid_t pid;
        char *command_args[MAX_ARGS] = {NULL};
        char *token = strtok(commands[i], " ");
        int arg_index = 0;

        while (token != NULL && arg_index < MAX_ARGS - 1) {
            command_args[arg_index++] = token;
            token = strtok(NULL, " ");
        }
        command_args[arg_index] = NULL;

        if (pipe(pipefd) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }

        if ((pid = fork()) == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // Child process
            close(pipefd[0]); // Close read end of pipe
            if (dup2(prev_pipe, STDIN_FILENO) == -1) {
                perror("dup2");
                exit(EXIT_FAILURE);
            }
            if (i != num_commands - 1) { // If not the last command
                if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
            } else if (redirect_type == OUTPUT_REDIRECT) {
                int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                if (fd == -1) {
                    perror("open");
                    exit(EXIT_FAILURE);
                }
                if (dup2(fd, STDOUT_FILENO) == -1) {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
                close(fd);
            } else if (redirect_type == APPEND_REDIRECT) {
                int fd = open(output_file, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                if (fd == -1) {
                    perror("open");
                    exit(EXIT_FAILURE);
                }
                if (dup2(fd, STDOUT_FILENO) == -1) {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
                close(fd);
            }
            close(pipefd[1]); // Close write end of pipe
            execvp(command_args[0], command_args);
            perror("execvp");
            exit(EXIT_FAILURE);
        } else {
            // Parent process
            close(pipefd[1]); // Close write end of pipe
            if (prev_pipe != STDIN_FILENO) {
                close(prev_pipe);
            }
            prev_pipe = pipefd[0]; // Set previous pipe to current pipe read end
        }
    }

    // Parent process waits for the last child to finish
    while (wait(NULL) > 0);
}

int main() {
    while (1) {
        char command[MAX_COMMAND_LENGTH];
        char *commands[MAX_ARGS] = {NULL};
        enum RedirectType redirect_type = NO_REDIRECT;
        char output_file[4096];

        printf("$ ");
        fgets(command, sizeof(command), stdin);
        command[strcspn(command, "\n")] = '\0';

        int num_commands = 0;
        char *token = strtok(command, ";");
        while (token != NULL && num_commands < MAX_ARGS) {
            commands[num_commands++] = token;
            token = strtok(NULL, ";");
        }

        for (int i = 0; i < num_commands; i++) {
            char *args[MAX_ARGS] = {NULL};
            int arg_index = 0;
            char *token = strtok(commands[i], " ");
            while (token != NULL && arg_index < MAX_ARGS - 1) {
                if (strcmp(token, ">") == 0 || strcmp(token, "<") == 0 || strcmp(token, ">>") == 0) {
                    // Skip redirection symbols
                    token = strtok(NULL, " ");
                    continue;
                }
                args[arg_index++] = token;
                token = strtok(NULL, " ");
            }
            args[arg_index] = NULL;

            for (int j = 0; j < arg_index; j++) {
                if (strcmp(args[j], ">") == 0) {
                    if (j + 1 < arg_index) {
                        strcpy(output_file, args[j + 1]);
                        redirect_type = OUTPUT_REDIRECT;
                        args[j] = NULL;
                        break;
                    }
                } else if (strcmp(args[j], "<") == 0) {
                    if (j + 1 < arg_index) {
                        strcpy(output_file, args[j + 1]);
                        redirect_type = INPUT_REDIRECT;
                        args[j] = NULL;
                        break;
                    }
                } else if (strcmp(args[j], ">>") == 0) {
                    if (j + 1 < arg_index) {
                        strcpy(output_file, args[j + 1]);
                        redirect_type = APPEND_REDIRECT;
                        args[j] = NULL;
                        break;
                    }
                }
            }

            execute_commands(args, 1, redirect_type, output_file);
        }
    }

    return 0;
}
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <unistd.h>
// #include <sys/wait.h>
// #include <sys/stat.h>
// #include <fcntl.h>

// #define MAX_COMMAND_LENGTH 100
// #define MAX_ARGS 10
// #define MAX_ARGUMENTS 100

// enum RedirectType {
//     NO_REDIRECT,
//     OUTPUT_REDIRECT,
//     INPUT_REDIRECT,
//     APPEND_REDIRECT
// };

// #define COLOR_GREEN "\033[0;32m"
// #define COLOR_RESET "\033[0m"

// void echo(const char *message) {
//     printf("%s\n", message);
// }


// void change_directory(const char *path) {
//     if (path == NULL || strcmp(path, "") == 0) {
//         path = getenv("HOME"); // 使用环境变量 HOME 来获取家目录路径
//     }

//     if (strcmp(path, "-") == 0 || strcmp(path, " ") == 0) {
//         const char *previous_directory = getenv("OLDPWD");
//         if (previous_directory == NULL) {
//             fprintf(stderr, "找不到上次所在的目录\n");
//             return;
//         }
//         if (chdir(previous_directory) != 0) {
//             perror("chdir");
//             return;
//         }
//     } else {
//         if (chdir(path) != 0) {
//             perror("chdir");
//             return;
//         }
//     }

//     char current_directory[4096];
//     if (getcwd(current_directory, sizeof(current_directory)) == NULL) {
//         perror("getcwd");
//         return;
//     }
//     if (setenv("OLDPWD", getenv("PWD"), 1) != 0) {
//         perror("setenv");
//         return;
//     }
//     if (setenv("PWD", current_directory, 1) != 0) {
//         perror("setenv");
//         return;
//     }
// }

// void execute_command(char *command, char *args[MAX_ARGS], enum RedirectType redirect_type, char *output_file, int run_in_background) {
//     if (strcmp(command, "cd") == 0) {
//        change_directory(args[1]);
//     } else if (strcmp(command, "exit") == 0) {
//         exit(0);
//     } else if (strcmp(command, "wc") == 0) {
//         // Handle wc command
//         pid_t pid = fork();
//         if (pid == -1) {
//             perror("fork");
//             exit(EXIT_FAILURE);
//         } else if (pid == 0) {
//             if (redirect_type == OUTPUT_REDIRECT || redirect_type == APPEND_REDIRECT) {
//                 int fd;
//                 if (redirect_type == OUTPUT_REDIRECT) {
//                     fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
//                 } else { // APPEND_REDIRECT
//                     fd = open(output_file, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
//                 }
//                 if (fd == -1) {
//                     perror("open");
//                     exit(EXIT_FAILURE);
//                 }
//                 if (dup2(fd, STDOUT_FILENO) == -1) {
//                     perror("dup2");
//                     exit(EXIT_FAILURE);
//                 }
//                 close(fd);
//             }
//             execvp(command, args);
//             perror("");
//             exit(EXIT_FAILURE);
//         } else {
//             if (!run_in_background) {
//                 int status;
//                 waitpid(pid, &status, 0);
//                 if (WIFEXITED(status)) {
//                     int exit_status = WEXITSTATUS(status);
//                     printf("子进程退出，退出状态码：%d\n", exit_status);
//                 } else if (WIFSIGNALED(status)) {
//                     int term_signal = WTERMSIG(status);
//                     printf("子进程被信号终止，信号编号：%d\n", term_signal);
//                 }
//             }
//         }
//     } else {
//         // Handle other commands
//         pid_t pid = fork();
//         if (pid == -1) {
//             perror("fork");
//             exit(EXIT_FAILURE);
//         } else if (pid == 0) {
//             if (run_in_background) {
//                 setpgid(0, 0);
//             }
//             if (redirect_type == OUTPUT_REDIRECT || redirect_type == APPEND_REDIRECT) {
//                 int fd;
//                 if (redirect_type == OUTPUT_REDIRECT) {
//                     fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
//                 } else { // APPEND_REDIRECT
//                     fd = open(output_file, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
//                 }
//                 if (fd == -1) {
//                     perror("open");
//                     exit(EXIT_FAILURE);
//                 }
//                 if (dup2(fd, STDOUT_FILENO) == -1) {
//                     perror("dup2");
//                     exit(EXIT_FAILURE);
//                 }
//                 close(fd);
//             }
//             execvp(command, args);
//             perror("");
//             exit(EXIT_FAILURE);
//         } else {
//             if (!run_in_background) {
//                 int status;
//                 waitpid(pid, &status, 0);
//                 if (WIFEXITED(status)) {
//                     int exit_status = WEXITSTATUS(status);
//                     printf("子进程退出，退出状态码：%d\n", exit_status);
//                 } else if (WIFSIGNALED(status)) {
//                     int term_signal = WTERMSIG(status);
//                     printf("子进程被信号终止，信号编号：%d\n", term_signal);
//                 }
//             }
//         }
//     }
// }

// int main() {
//     while (1) {
//         char command[MAX_COMMAND_LENGTH];
//         char *args[MAX_ARGS][MAX_ARGUMENTS] = { NULL };
//         enum RedirectType redirect_type = NO_REDIRECT;
//         char output_file[4096];
//         int run_in_background = 0;

//         char current_directory[4096];
//         if (getcwd(current_directory, sizeof(current_directory)) == NULL) {
//             perror("getcwd");
//             exit(EXIT_FAILURE);
//         }

//         char *username = getenv("USER");

//         printf(COLOR_GREEN "%s@%s:" COLOR_RESET "%s$ ", username, getenv("HOSTNAME"), current_directory);

//        // 读取用户命令
//         fgets(command, sizeof(command), stdin);
//         command[strcspn(command, "\n")] = '\0';

//         // 检查命令是否为空行或全是空格
//         if (strcmp(command, "") == 0 || strspn(command, " \t") == strlen(command)) {
//             continue;
//         }

//         char *token = strtok(command, " ");
//         int arg_index = 0;
//         int token_index = 0;
//         while (token != NULL && arg_index < MAX_ARGS - 1 && token_index < MAX_ARGUMENTS - 1) {
//             if (strcmp(token, ">") == 0 || strcmp(token, "<") == 0 || strcmp(token, ">>") == 0) {
//                 redirect_type = strcmp(token, ">") == 0 ? OUTPUT_REDIRECT : strcmp(token, ">>") == 0 ? APPEND_REDIRECT : INPUT_REDIRECT;
//                 token = strtok(NULL, " ");
//                 strcpy(output_file, token);
//                 break; // Stop parsing arguments after encountering redirection
//             } else if (strcmp(token, "&") == 0) {
//                 run_in_background = 1;
//                 break;
//             } else if (strcmp(token, "|") == 0) {
//                 arg_index++;
//                 token_index = 0;
//                 token = strtok(NULL, " ");
//                 continue;
//             }
//             args[arg_index][token_index] = token;
//             token_index++;
//             token = strtok(NULL, " ");
//         }
//         args[arg_index][token_index] = NULL;

//         if (args[0][0] != NULL && strcmp(args[0][0], "|") == 0) {
//             printf("语法错误：管道命令必须与其他命令组合\n");
//             continue;
//         }

//         int pipe_index = -1;
//         for (int i = 0; i <= arg_index; i++) {
//             if (args[i][0] != NULL && strcmp(args[i][0], "|") == 0) {
//                 pipe_index = i;
//                 break;
//             }
//         }

//         pipe_index = -1;
//     for (int i = 0; i <= arg_index; i++) {
//         if (args[i][0] != NULL && strcmp(args[i][0], "|") == 0) {
//         pipe_index = i;
//         break;
//         }
//     }

//     if (pipe_index != -1) {
//         int pipefd[2];
//         if (pipe(pipefd) == -1) {
//             perror("pipe");
//             exit(EXIT_FAILURE);
//         }

//         pid_t pid1 = fork();
//         if (pid1 == -1) {
//             perror("fork");
//             exit(EXIT_FAILURE);
//         } else if (pid1 == 0) {
//             close(pipefd[0]);
//             dup2(pipefd[1], STDOUT_FILENO);
//             close(pipefd[1]);
//             args[pipe_index][0] = NULL;
//             execvp(args[0][0], args[0]);
//             perror("");
//             exit(EXIT_FAILURE);
//         }

//         pid_t pid2 = fork();
//         if (pid2 == -1) {
//             perror("fork");
//             exit(EXIT_FAILURE);
//         } else if (pid2 == 0) {
//             close(pipefd[1]);
//             dup2(pipefd[0], STDIN_FILENO);
//             close(pipefd[0]);
//             execvp(args[pipe_index + 1][0], args[pipe_index + 1]);
//             perror("");
//             exit(EXIT_FAILURE);
//         }

//         close(pipefd[0]);
//         close(pipefd[1]);
//         waitpid(pid1, NULL, 0);
//         waitpid(pid2, NULL, 0);
//     }
    
//      else {
//         execute_command(args[0][0], args[0], redirect_type, redirect_type ? output_file : NULL, run_in_background);
//     }


//         if (getcwd(current_directory, sizeof(current_directory)) == NULL) {
//             perror("getcwd");
//             exit(EXIT_FAILURE);
//         }

//         if (setenv("PWD", current_directory, 1) != 0) {
//             perror("setenv");
//             exit(EXIT_FAILURE);
//         }
//     }

//     return 0;
// }
