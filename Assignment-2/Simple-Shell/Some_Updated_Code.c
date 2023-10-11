#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_COMMAND_LENGTH 1000
#define MAX_ARGUMENTS 100
#define MAX_HISTORY_SIZE 10

char* history[MAX_HISTORY_SIZE];
int history_count = 0;

char previous_directory[MAX_COMMAND_LENGTH];

void add_to_history(char* command, pid_t pid, time_t start_time, time_t end_time);
void display_history();

void executeCommand(char **args, int pipe_flag, int pipe_read_fd);

int main() {

    char input[MAX_COMMAND_LENGTH];
    char *args[MAX_ARGUMENTS];
    int pipe_flag = 0;
    int pipe_fds[2];
    int pipe_read_fd = -1;

    while (1) {
        char cwd[1024];
        getcwd(cwd, sizeof(cwd));
        printf("Simple-Shell:%s$ ", cwd);

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

        add_to_history(input, 0, 0, 0);  // Initialize the command details

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

                if (fork() == 0) {
                    close(pipe_fds[0]);
                    dup2(pipe_read_fd, STDIN_FILENO);
                    dup2(pipe_fds[1], STDOUT_FILENO);
                    close(pipe_fds[1]);
                    executeCommand(args, 1, pipe_read_fd);
                } else {
                    close(pipe_read_fd);
                    close(pipe_fds[1]);
                    if (wait(NULL) == -1) {
                        perror("wait");
                        exit(EXIT_FAILURE);
                    }
                    pipe_read_fd = pipe_fds[0];
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

void add_to_history(char* command, pid_t pid, time_t start_time, time_t end_time) {
    if (history_count < MAX_HISTORY_SIZE) {
        history[history_count++] = strdup(command);
    } else {
        free(history[0]);
        for (int i = 0; i < MAX_HISTORY_SIZE - 1; i++) {
            history[i] = history[i+1];
        }
        history[MAX_HISTORY_SIZE - 1] = strdup(command);
    }

    // Display the PID and other details
    if (pid != 0) {
        printf("Command: %s\n", command);
        printf("PID: %d\n", pid);
        printf("Start Time: %s", ctime(&start_time));
        printf("End Time: %s", ctime(&end_time));
        printf("Duration: %.2f seconds\n", difftime(end_time, start_time));
    }
}

void display_history() {
    for (int i = 0; i < history_count; i++) {
        printf("%d. %s\n", i+1, history[i]);
    }
}

void executeCommand(char **args, int pipe_flag, int pipe_read_fd) {
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
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
        if (!pipe_flag) {
            time_t start_time, end_time;
            time(&start_time);
            if (wait(NULL) == -1) {
                perror("wait");
                exit(EXIT_FAILURE);
            }
            time(&end_time);
            add_to_history(args[0], pid, start_time, end_time);
        }
    }
}
