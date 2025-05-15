#include "scheduler.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <time.h>
#include <string.h>

// Log an event to scheduler.log
void log_event(const char* message) {
    FILE *logfile = fopen("scheduler.log", "a");
    if (logfile != NULL) {
        time_t current_time = time(NULL);
        struct tm* tm_info = localtime(&current_time);
        char time_buffer[26];
        strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", tm_info);
        fprintf(logfile, "[%s] [INFO] %s\n", time_buffer, message);
        fclose(logfile);
    }
}

char* get_current_time_str(){
    time_t current_time = time(NULL);
    struct tm* tm_info = localtime(&current_time);
    char* time_buffer = (char*)malloc(26*sizeof(char));

    strftime(time_buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);

    return time_buffer;
} // Terminal shows currently forked and exec jobs

void fork_and_exec(Job* job) {
    pid_t pid = fork();
    if (pid == 0) {
        // Child process
        char message[256];
        snprintf(message, sizeof(message), "Executing %s (PID: %d) using exec", job->name, getpid());
        log_event(message);
        printf("[%s] [INFO] %s\n", get_current_time_str(), message); // Print to terminal
        // Simulate execution (e.g., sleep)
        sleep(job->execution_time);
        exit(0); // Simulate successful completion
    } else if (pid > 0) {
        // Parent process
        job->pid = pid;
        char message[256];
        snprintf(message, sizeof(message), "Forking new process for %s", job->name);
        log_event(message);
        printf("[%s] [INFO] %s\n", get_current_time_str(), message); //Print to terminal
        job->has_started = 1;
    } else {
        perror("fork failed");
        exit(1);
    }
}


// Handle job completion
void handle_job_completion(Job* job) {
    char message[256];
    snprintf(message, sizeof(message), "%s completed execution. Terminating (PID: %d)", job->name, job->pid);
    log_event(message);
    kill(job->pid, SIGKILL);
}

void stop_job(Job* job, int run_time) {
    char message[256];
    snprintf(message, sizeof(message), "%s ran for %d seconds. Time slice expired - Sending SIGSTOP", job->name, run_time);
    log_event(message);
    kill(job->pid, SIGSTOP);
}

// Resume the job
void resume_job(Job* job) {
    char message[256];
    snprintf(message, sizeof(message), "Resuming %s (PID: %d) - SIGCONT", job->name, job->pid);
    log_event(message);
    kill(job->pid, SIGCONT);
}

// Comparison function for qsort
int compare_jobs(const void* a, const void* b) {
    Job* jobA = (Job*)a;
    Job* jobB = (Job*)b;

    // Compare by priority first
    if (jobA->priority != jobB->priority) {
        return jobA->priority - jobB->priority; // Lower priority comes first
    }

    // If priorities are the same, compare by arrival time
    if (jobA->arrival_time != jobB->arrival_time) {
        return jobA->arrival_time - jobB->arrival_time; // Earlier arrival comes first
    }

    // If priorities and arrival times are the same, compare by remaining time
    return jobA->remaining_time - jobB->remaining_time; // Shorter remaining time comes first
}

// Function to find the next job to run based on priority and arrival time
int find_next_job(Job* jobs, int num_jobs, int current_time, int last_run_index) {
    int next_job_index = -1;
    int highest_priority = 9999; // Initialize with a very high value
    int earliest_arrival = 9999;
    int shortest_remaining = 9999;

    for (int i = 0; i < num_jobs; i++) {
        if (jobs[i].remaining_time > 0 && jobs[i].arrival_time <= current_time && i != last_run_index) {
            if (jobs[i].priority < highest_priority) {
                highest_priority = jobs[i].priority;
                earliest_arrival = jobs[i].arrival_time;
                shortest_remaining = jobs[i].remaining_time;
                next_job_index = i;
            } else if (jobs[i].priority == highest_priority) {
                if (jobs[i].arrival_time < earliest_arrival) {
                    earliest_arrival = jobs[i].arrival_time;
                    shortest_remaining = jobs[i].remaining_time;
                    next_job_index = i;
                } else if (jobs[i].arrival_time == earliest_arrival && jobs[i].remaining_time < shortest_remaining) {
                    shortest_remaining = jobs[i].remaining_time;
                    next_job_index = i;
                }
            }
        }
    }

    //If no other job is available, return the last run job.
    if(next_job_index == -1 && last_run_index != -1 && jobs[last_run_index].remaining_time > 0 && jobs[last_run_index].arrival_time <= current_time){
        next_job_index = last_run_index;
    }

    return next_job_index;
}

void run_scheduler(Job* jobs, int num_jobs, int time_quantum) {
    int current_time = 0;
    int completed_jobs = 0;
    int current_job_index = -1;
    int last_run_index = -1;

    // Initialize has_started flags
    for (int i = 0; i < num_jobs; i++) {
        jobs[i].has_started = 0;
    }

    while (completed_jobs < num_jobs) {
        current_job_index = find_next_job(jobs, num_jobs, current_time, last_run_index);

        if (current_job_index != -1) {
            if (!jobs[current_job_index].has_started) {
                fork_and_exec(&jobs[current_job_index]);
            } else {
                resume_job(&jobs[current_job_index]);
            }

            int run_time = (jobs[current_job_index].remaining_time > time_quantum) ? time_quantum : jobs[current_job_index].remaining_time;

            for (int i = 0; i < run_time; i++) {
                sleep(1);
                current_time++;
                jobs[current_job_index].remaining_time--;

                // Corrected: Only check for new arrivals within the time slice
                if(i == run_time - 1){ //check at the end of the time slice
                    for (int j = 0; j < num_jobs; j++) {
                        if (jobs[j].arrival_time == current_time && j != current_job_index && jobs[j].remaining_time > 0 && !jobs[j].has_started) {
                            fork_and_exec(&jobs[j]);
                        }
                    }
                }
            }

            if (jobs[current_job_index].remaining_time > 0) {
                stop_job(&jobs[current_job_index], run_time);
            } else {
                handle_job_completion(&jobs[current_job_index]);
                completed_jobs++;
            }

            last_run_index = current_job_index;
        } else {
            sleep(1);
            current_time++;
        }
    }
}

void schedule_processes(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open jobs file");
        return;
    }

    int time_quantum;
    fscanf(file, "TimeSlice %d", &time_quantum);

    Job jobs[100]; // Assume a max of 100 jobs
    int num_jobs = 0;

    while (fscanf(file, "%s %d %d %d", jobs[num_jobs].name, &jobs[num_jobs].arrival_time,
                   &jobs[num_jobs].priority, &jobs[num_jobs].execution_time) != EOF) {
        jobs[num_jobs].remaining_time = jobs[num_jobs].execution_time;
        num_jobs++;
    }

    fclose(file);
    run_scheduler(jobs, num_jobs, time_quantum);
}

int main() {
    // Start the scheduling process
    schedule_processes("jobs.txt");
    return 0;
}
