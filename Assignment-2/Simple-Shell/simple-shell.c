#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>

#define MAX_INPUT_LENGTH 1024
#define MAX_HISTORY_SIZE 50

struct CommandExecution {
    char cmd[MAX_INPUT_LENGTH];
    pid_t pid;
    time_t start_time;
    time_t end_time;
    double duration;
};

void launch(char *cmd, struct CommandExecution *history, int *history_count) {
    // Record start time
    time(&history[*history_count].start_time);

    // Fork a child process
    pid_t pid = fork();

    if (pid == 0) {
        // Child process
        // Execute the command using the system function
        int result = system(cmd);
        exit(result); // Exit the child process with the result of the command
    } else if (pid < 0) {
        perror("Fork failed");
    } else {
        // Parent process
        if (cmd[strlen(cmd) - 1] == '&') {
            // Background process
            printf("[%d] %s\\n", pid, cmd);
        } else {
            // Foreground process
            waitpid(pid, NULL, 0);
        }

        // Record end time
        time(&history[*history_count].end_time);

        // Calculate duration
        history[*history_count].duration = difftime(history[*history_count].end_time, history[*history_count].start_time);

        // Get the PID of the child process
        history[*history_count].pid = pid;

        // Update history count
        (*history_count)++;
    }
}


int main() {
    char input[MAX_INPUT_LENGTH];
    struct CommandExecution history[MAX_HISTORY_SIZE];
    int history_count = 0;

    printf("SimpleShell> ");

    while (fgets(input, sizeof(input), stdin)) {
        input[strcspn(input, "\n")] = '\0';

        if (strcmp(input, "exit") == 0 || strcmp(input, "quit") == 0) {
            printf("Command Execution Details:\n");
            int i = 0;
            while (i < history_count) {
                printf("Command: %s\n", history[i].cmd);
                printf("PID: %d\n", history[i].pid);
                printf("Start Time: %s", ctime(&history[i].start_time));
                printf("End Time: %s", ctime(&history[i].end_time));
                printf("Duration: %.2f seconds\n", history[i].duration);
                i++;
            }
            break;
        }

        if (strcmp(input, "history") == 0) {
            printf("Command History:\n");
            int i = 0;
            while (i < history_count) {
                printf("%d: %s\n", i + 1, history[i].cmd);
                i++;
            }
            continue;
        }

        if (strncmp(input, "cd ", 3) == 0) {
            char* dir = input + 3;
            if (chdir(dir) != 0) {
                perror("cd failed");
            }
            continue;
        }

        if (history_count < MAX_HISTORY_SIZE) {
            strncpy(history[history_count].cmd, input, sizeof(history[history_count].cmd));
            launch(input, history, &history_count);
        } else {
            int i = 0;
            while (i < MAX_HISTORY_SIZE - 1) {
                history[i] = history[i + 1];
                i++;
            }
            strncpy(history[MAX_HISTORY_SIZE - 1].cmd, input, sizeof(history[MAX_HISTORY_SIZE - 1].cmd));
            launch(input, history, &history_count);
        }

        printf("\nSimpleShell> "); 
    }

    return 0;
}
