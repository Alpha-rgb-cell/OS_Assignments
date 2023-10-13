#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_COMMAND_LENGTH 1000
#define MAX_ARGUMENTS 100
#define MAX_HISTORY_SIZE 10

struct CommandExecution {
    char cmd[MAX_COMMAND_LENGTH];
    pid_t pid;
};

struct CommandExecution history[MAX_HISTORY_SIZE];
int history_count = 0;

char previous_directory[MAX_COMMAND_LENGTH];

void add_to_history(char* command, pid_t pid) {
    if (history_count < MAX_HISTORY_SIZE) {
        strncpy(history[history_count].cmd, command, sizeof(history[history_count].cmd));
        history[history_count].pid = pid;
        history_count++;
    } else {
        for (int i = 0; i < MAX_HISTORY_SIZE - 1; i++) {
            history[i] = history[i+1];
        }
        strncpy(history[MAX_HISTORY_SIZE - 1].cmd, command, sizeof(history[MAX_HISTORY_SIZE - 1].cmd));
        history[MAX_HISTORY_SIZE - 1].pid = pid;
    }
}

void display_history() {
    for (int i = 0; i < history_count; i++) {
        if (strcmp(history[i].cmd, "") != 0) { // Check if the command is not empty
            printf("%d: %s\n", i + 1, history[i].cmd);
        }
    }
}

void executeCommand(char **args, int pipe_flag, int pipe_read_fd) {
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        // Child process

        if (pipe_flag) {
            if (dup2(pipe_read_fd, STDIN_FILENO) == -1) {
                perror("dup2");
                exit(EXIT_FAILURE);
            }
        }

        execvp(args[0], args);
        perror(args[0]);
        exit(EXIT_FAILURE);
    } else {
        // Parent process
        if (!pipe_flag) {
            int status;
            if (waitpid(pid, &status, 0) == -1) {
                perror("waitpid");
                exit(EXIT_FAILURE);
            }
        } else {
            add_to_history(args[0], pid);
        }
    }
}

int main() {
    char input[MAX_COMMAND_LENGTH];
    char *args[MAX_ARGUMENTS];
    int pipe_flag = 0;
    int pipe_fds[2];
    int pipe_read_fd = -1;

    while (1) {
        char cwd[1024];
        getcwd(cwd, sizeof(cwd));
        printf("Simple-Shell~ %s $ ", cwd);

        if (fgets(input, MAX_COMMAND_LENGTH, stdin) == NULL) {
            perror("fgets");
            exit(EXIT_FAILURE);
        }

        input[strcspn(input, "\n")] = '\0';

        if (strcmp(input, "history") == 0) {
            display_history();
            continue;
        }

        if (strcmp(input, "exit") == 0)
            break;

        if (strcmp(input, "cd -") == 0) {
            char *path = previous_directory;
            if (chdir(path) != 0) {
                perror("cd");
            }
            continue;
        }

        if (strncmp(input, "cd ", 3) == 0) {
            char *path = input + 3;
            if (strcmp(path, "-") == 0) {
                if (getcwd(previous_directory, sizeof(previous_directory)) == NULL) {
                    perror("getcwd");
                    exit(EXIT_FAILURE);
                }
            }
            if (chdir(path) != 0) {
                perror("cd");
            }
            continue;
        }

        if (strcmp(input, "wc -l") == 0) {
            FILE *pipe_fp;
            char buffer[1024];

            if ((pipe_fp = popen("wc -l", "r")) == NULL) {
                perror("popen");
                exit(EXIT_FAILURE);
            }

            while (fgets(buffer, sizeof(buffer), pipe_fp) != NULL) {
                printf("%s", buffer);
            }

            if (pclose(pipe_fp) == -1) {
                perror("pclose");
                exit(EXIT_FAILURE);
            }
            continue;
        }

        if (strcmp(input, "wc -c") == 0) {
            FILE *pipe_fp;
            char buffer[1024];

            if ((pipe_fp = popen("wc -c", "r")) == NULL) {
                perror("popen");
                exit(EXIT_FAILURE);
            }

            while (fgets(buffer, sizeof(buffer), pipe_fp) != NULL) {
                printf("%s", buffer);
            }

            if (pclose(pipe_fp) == -1) {
                perror("pclose");
                exit(EXIT_FAILURE);
            }
            continue;
        }

        add_to_history(input, getpid());

        char *token = strtok(input, "|");

        while (token != NULL) {
            args[0] = strtok(token, " ");
            int i = 1;

            while (i < MAX_ARGUMENTS && (args[i] = strtok(NULL, " ")) != NULL) {
                i++;
            }

            if (pipe_flag) {
                if (pipe(pipe_fds) < 0) {
                    perror("Pipe");
                    exit(EXIT_FAILURE);
                }
                args[i] = NULL;

                pid_t child_pid = fork();
                if (child_pid == 0) {
                    close(pipe_fds[0]);
                    dup2(pipe_read_fd, STDIN_FILENO);
                    dup2(pipe_fds[1], STDOUT_FILENO);
                    close(pipe_fds[1]);
                    executeCommand(args, 1, pipe_read_fd);
                    exit(EXIT_SUCCESS);
                } else {
                    close(pipe_read_fd);
                    close(pipe_fds[1]);
                    if (wait(NULL) == -1) {
                        perror("wait");
                        exit(EXIT_FAILURE);
                    }
                    pipe_read_fd = pipe_fds[0];
                    add_to_history(args[0], child_pid);
                }
                pipe_flag = 0;
            } else {
                executeCommand(args, 0, -1);
            }

            token = strtok(NULL, "|");
        }

        pipe_read_fd = -1;
    }

    return 0;
}
