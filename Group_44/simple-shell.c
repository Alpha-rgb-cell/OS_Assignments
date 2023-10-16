#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>

#define MAX_COMMAND_LENGTH 1000
#define MAX_ARGUMENTS 100
#define MAX_HISTORY_SIZE 10

#define JOB_READY_SIGNAL SIGUSR3

#define TSLICE 500 // You can adjust this value as needed


struct CommandExecution {
    char cmd[MAX_COMMAND_LENGTH];
    pid_t pid;
    int priority;
    struct timeval start_time;
    struct timeval end_time;
    struct timeval wait_time;
};

struct CommandExecution history[MAX_HISTORY_SIZE];
int history_count = 0;

void add_to_history(char* command, pid_t pid, int priority) {
    if (history_count < MAX_HISTORY_SIZE) {
        strncpy(history[history_count].cmd, command, sizeof(history[history_count].cmd));
        history[history_count].pid = pid;
        history[history_count].priority = priority;
        gettimeofday(&history[history_count].start_time, NULL);
        history_count++;
    } else {
        // Handle the case when the history array is full
    }
}

void printHistory(void) {
    printf("Job History:\n");
    for (int i = 0; i < history_count; i++) {
        printf("Name: %s\n", history[i].cmd);
        printf("PID: %d\n", history[i].pid);
        printf("Execution Time: %ld microseconds\n",
               (history[i].end_time.tv_sec - history[i].start_time.tv_sec) * 1000000 +
                   (history[i].end_time.tv_usec - history[i].start_time.tv_usec));
        printf("Wait Time: %ld microseconds\n",
               history[i].wait_time.tv_sec * 1000000 + history[i].wait_time.tv_usec);
        printf("Priority: %d\n", history[i].priority);
        printf("\n");
    }
}

void createAndExecuteProcess(char *executable_name, int priority) {
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        // Child process
        char *args[] = {executable_name, NULL};
        execvp(args[0], args);
        perror(args[0]);
        exit(EXIT_FAILURE);
    } else {
        // Parent process
        struct timeval start_wait_time;
        gettimeofday(&start_wait_time, NULL); // Get the start wait time

        add_to_history(executable_name, pid, priority);

        int status;
        if (waitpid(pid, &status, 0) == -1) {
            perror("waitpid");
            exit(EXIT_FAILURE);
        }

        gettimeofday(&history[history_count - 1].end_time, NULL);
        
        // Calculate the wait time by subtracting the start wait time from the child's start time
        timersub(&history[history_count - 1].start_time, &start_wait_time, &history[history_count - 1].wait_time);
    }
}


int main() {
    char input[MAX_COMMAND_LENGTH];

    while (1) {
        printf("SimpleShell$ ");
        if (fgets(input, MAX_COMMAND_LENGTH, stdin) == NULL) {
            perror("fgets");
            exit(EXIT_FAILURE);
        }

        input[strcspn(input, "\n")] = '\0';

        if (strcmp(input, "history") == 0) {
            printHistory();
            continue;
        }

        if (strcmp(input, "exit") == 0) {
            printHistory();
            break;
        }

        if (strncmp(input, "submit ", 7) == 0) {
            char *submit_command = input + 7;
            char *executable_name = strtok(submit_command, " ");
            char *priority_str = strtok(NULL, " ");

            if (executable_name != NULL) {
                int job_priority = (priority_str != NULL) ? atoi(priority_str) : 1;
                createAndExecuteProcess(executable_name, job_priority);
            } else {
                printf("Usage: submit <executable> [priority]\n");
            }
            continue;
        }
    }

    return 0;
}

