
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <termios.h>
#include <fcntl.h>
#include <signal.h> 

#define MAX_COMMAND_LENGTH 100
#define MAX_ARGS 10
#define MAX_ARGUMENTS 100

enum RedirectType {
    NO_REDIRECT,
    OUTPUT_REDIRECT,
    INPUT_REDIRECT,
    APPEND_REDIRECT
};

#define COLOR_GREEN "\033[0;32m"
#define COLOR_RESET "\033[0m"

void echo(const char *message) {
    printf("%s\n", message);
}

void change_directory(const char *path) {
    if (path == NULL || strcmp(path, "") == 0) {
        path = getenv("HOME");
    }

    if (strcmp(path, "-") == 0 || strcmp(path, " ") == 0) {
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
        if (chdir(path) != 0) {
            perror("chdir");
            return;
        }
    }

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

void execute_command(char *command, char *args[MAX_ARGS], enum RedirectType redirect_type, char *output_file, int run_in_background, char *input_file) {
    if (strcmp(command, "cd") == 0) {
        change_directory(args[1]); // 直接调用 change_directory
        return;
    } else if (strcmp(command, "exit") == 0) {
        exit(0); // 退出 shell
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // 子进程中的代码
        if (run_in_background) {
            setpgid(0, 0);
        }
        if (redirect_type == OUTPUT_REDIRECT || redirect_type == APPEND_REDIRECT) {
            int fd;
            if (redirect_type == OUTPUT_REDIRECT) {
                fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            } else { // APPEND_REDIRECT
                fd = open(output_file, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            }
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
        if (input_file != NULL) {
            int fd_in = open(input_file, O_RDONLY);
            if (fd_in == -1) {
                perror("open");
                exit(EXIT_FAILURE);
            }
            if (dup2(fd_in, STDIN_FILENO) == -1) {
                perror("dup2");
                exit(EXIT_FAILURE);
            }
            close(fd_in);
        }
        execvp(command, args);
        perror("");
        exit(EXIT_FAILURE);
    } else {
        if (!run_in_background) {
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
void sigint_handler(int signum) {
    // 空函数，不执行任何操作
}

int main() {
    while (1) {
        signal(SIGINT, SIG_IGN);
         struct termios term;//存储终端的相关信息
        tcgetattr(STDIN_FILENO, &term);//获取相关信息，并存入结构体
        term.c_cc[VEOF] = _POSIX_VDISABLE;//禁止使用EOF
        tcsetattr(STDIN_FILENO, TCSANOW, &term);//将设置存入终端

        char command[MAX_COMMAND_LENGTH];
        enum RedirectType redirect_type = NO_REDIRECT;
        char input_file[4096];
        char output_file[4096];
        int run_in_background = 0;

        char current_directory[4096];
        if (getcwd(current_directory, sizeof(current_directory)) == NULL) {
            perror("getcwd");
            exit(EXIT_FAILURE);
        }

        char *username = getenv("USER");

        printf(COLOR_GREEN "%s@%s:" COLOR_RESET "%s$ ", username, getenv("HOSTNAME"), current_directory);

        fgets(command, sizeof(command), stdin);
        command[strcspn(command, "\n")] = '\0';

        if (strcmp(command, "") == 0 || strspn(command, " \t") == strlen(command)) {
            continue;
        }

        char *token = strtok(command, "|");
        char *pipe_commands[MAX_ARGS];
        int num_pipes = 0;

        while (token != NULL && num_pipes < MAX_ARGS) {
            pipe_commands[num_pipes++] = token;
            token = strtok(NULL, "|");
        }

        int fd_in = 0;

        for (int i = 0; i < num_pipes; i++) {
            char *trimmed_token = strtok(pipe_commands[i], " \t");
            if (strcmp(trimmed_token, "") == 0) {
                continue;
            }

            char *args[MAX_ARGS];
            int arg_index = 0;

            while (trimmed_token != NULL && arg_index < MAX_ARGS - 1) {
                if (strcmp(trimmed_token, ">") == 0 || strcmp(trimmed_token, "<") == 0 || strcmp(trimmed_token, ">>") == 0) {
                    redirect_type = strcmp(trimmed_token, ">") == 0 ? OUTPUT_REDIRECT : strcmp(trimmed_token, ">>") == 0 ? APPEND_REDIRECT : INPUT_REDIRECT;
                    trimmed_token = strtok(NULL, " \t");
                    if (redirect_type == INPUT_REDIRECT) {
                        strcpy(input_file, trimmed_token);
                    } else {
                        strcpy(output_file, trimmed_token);
                    }
                    break;
                } else if (strcmp(trimmed_token, "&") == 0) {
                    run_in_background = 1;
                    break;
                }
                args[arg_index++] = trimmed_token;
                trimmed_token = strtok(NULL, " \t");
            }
            args[arg_index] = NULL;

            if (strcmp(args[0], "cd") == 0) {
                change_directory(args[1]);
                continue;
            } else if (strcmp(args[0], "exit") == 0) {
                exit(0);
            }

            int pipefd[2];
            if (pipe(pipefd) == -1) {
                perror("pipe");
                exit(EXIT_FAILURE);
            }

            pid_t pid = fork();
            if (pid == -1) {
                perror("fork");
                exit(EXIT_FAILURE);
            } else if (pid == 0) {
                dup2(fd_in, 0);
                if (i < num_pipes - 1) {
                    dup2(pipefd[1], 1);
                } else {
                    if (redirect_type == OUTPUT_REDIRECT || redirect_type == APPEND_REDIRECT) {
                        int fd;
                        if (redirect_type == OUTPUT_REDIRECT) {
                            fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                        } else { // APPEND_REDIRECT
                            fd = open(output_file, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                        }
                        if (fd == -1) {
                            perror("open");
                            exit(EXIT_FAILURE);
                        }
                        dup2(fd, STDOUT_FILENO);
                        close(fd);
                    }
                }

                if (redirect_type == INPUT_REDIRECT) {
                    int fd_in = open(input_file, O_RDONLY);
                    if (fd_in == -1) {
                        perror("open");
                        exit(EXIT_FAILURE);
                    }
                    dup2(fd_in, STDIN_FILENO);
                    close(fd_in);
                }

                close(pipefd[0]);
                execvp(args[0], args);
                perror("execvp");
                exit(EXIT_FAILURE);
            } else {
                wait(NULL);
                close(pipefd[1]);
                fd_in = pipefd[0];
            }
        }
    }
    return 0;
}
