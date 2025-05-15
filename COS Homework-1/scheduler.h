// scheduler.h
#include <sys/types.h>  // For pid_t implementation

#ifndef SCHEDULER_H
#define SCHEDULER_H

typedef struct {
    char name[10];
    int arrival_time;
    int priority;
    int execution_time;
    int remaining_time;
    pid_t pid;
    int has_started; // Add this line
} Job;

void log_event(const char* message);
void fork_and_exec(Job* job);
void handle_job_completion(Job* job);
void stop_job(Job* job, int run_time); // Update this line
void resume_job(Job* job);
void schedule_processes(const char* filename);
void run_scheduler(Job* jobs, int num_jobs, int time_quantum);

#endif

