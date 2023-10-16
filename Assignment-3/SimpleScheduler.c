#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>

// Constants
#define MAX_PROCESSES 100
#define DEFAULT_TSLICE 100 // Default time quantum in milliseconds

// Data structure to represent a process in the ready queue
typedef struct {
    pid_t pid;
    int priority;
    int execution_time;
    int wait_time;
} Process;

Process ready_queue[MAX_PROCESSES];
int num_processes = 0;

int NCPU; // Total number of CPU resources
int TSLICE; // Time quantum in milliseconds

struct itimerval it_val;

// Function to handle SIGALRM signal for time slicing
void handle_alarm(int signo) {
    // Stop the current process
    kill(ready_queue[0].pid, SIGSTOP);
}

// Function to initialize the scheduler
void initialize_scheduler(int ncpu, int tslice) {
    NCPU = ncpu;
    TSLICE = tslice;

    // Set up the timer for the first alarm
    it_val.it_value.tv_sec = 0;
    it_val.it_value.tv_usec = TSLICE * 1000; // Convert to microseconds
    it_val.it_interval = it_val.it_value;
    setitimer(ITIMER_REAL, &it_val, NULL);
}

// Function to add a process to the ready queue
void add_process(pid_t pid, int priority) {
    if (num_processes < MAX_PROCESSES) {
        Process process;
        process.pid = pid;
        process.priority = priority;
        process.execution_time = 0;
        process.wait_time = 0;

        // Add the process to the correct position based on priority
        int i = num_processes;
        while (i > 0 && ready_queue[i - 1].priority > priority) {
            ready_queue[i] = ready_queue[i - 1];
            i--;
        }
        ready_queue[i] = process;

        num_processes++;
    } else {
        fprintf(stderr, "Ready queue is full. Cannot add more processes.\n");
    }
}

// Function to start the scheduling
void start_scheduling() {
    signal(SIGALRM, handle_alarm);
    alarm(TSLICE / 1000); // Set up the initial time slice alarm

    int current_process = 0;
    while (num_processes > 0) {
        if (num_processes >= NCPU) {
            // Start NCPU processes
            for (int i = 0; i < NCPU; i++) {
                kill(ready_queue[i].pid, SIGCONT);
                current_process++;
            }
            // Wait for alarm signal (time slice)
            pause();
            // Stop NCPU processes
            for (int i = 0; i < NCPU; i++) {
                kill(ready_queue[i].pid, SIGSTOP);
            }
            // Update execution times
            for (int i = 0; i < NCPU; i++) {
                ready_queue[i].execution_time += TSLICE;
                ready_queue[i].wait_time += TSLICE * (num_processes - NCPU);
            }
        } else {
            // Start all remaining processes
            for (int i = current_process; i < num_processes; i++) {
                kill(ready_queue[i].pid, SIGCONT);
            }
            // Wait for alarm signal (time slice)
            pause();
            // Stop all remaining processes
            for (int i = current_process; i < num_processes; i++) {
                kill(ready_queue[i].pid, SIGSTOP);
            }
            // Update execution times
            for (int i = current_process; i < num_processes; i++) {
                ready_queue[i].execution_time += TSLICE;
                ready_queue[i].wait_time += TSLICE * (num_processes - current_process - 1);
            }
        }
    }

    // Terminate the scheduler
    printf("All processes have completed. SimpleScheduler is exiting.\n");
    exit(0);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <NCPU> <TSLICE>\n", argv[0]);
        exit(1);
    }

    int ncpu = atoi(argv[1]);
    int tslice = atoi(argv[2]);

    initialize_scheduler(ncpu, tslice);

    // Start the scheduling loop
    start_scheduling();

    return 0;
}
