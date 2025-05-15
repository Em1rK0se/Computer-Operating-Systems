COS Homework-1: Process Scheduler Simulation

Description:
This program simulates a preemptive priority-based process scheduler using a round-robin approach with a specified time quantum. It reads job information from a "jobs.txt" file, schedules the jobs according to their arrival time, priority, and remaining execution time, and logs the execution events to a "scheduler.log" file. Additionally, the program prints the scheduling events to the terminal for real-time monitoring.

Files:
- scheduler.c: The main source code for the scheduler program.
- scheduler.h: Header file containing job structure and function declarations.
- jobs.txt: Input file containing job information.
- scheduler.log: Output log file containing scheduling events.
- Makefile: Automation script for compiling and running the program.
- README: This file, providing instructions and information.

Compilation:
To compile the program, navigate to the project directory in your terminal and run:

make

This command will compile the "scheduler.c" file and create an executable named "scheduler".

Execution:
To run the program, ensure that a "jobs.txt" file is present in the same directory. Then, execute the program using:

make run

This will compile the program (if necessary) and then run it. The scheduling events will be printed to the terminal and logged in "scheduler.log".

jobs.txt Format:
The "jobs.txt" file should adhere to the following format:

TimeSlice <time_quantum>
<job_name> <arrival_time> <priority> <execution_time>
<job_name> <arrival_time> <priority> <execution_time>
...

Example:
TimeSlice 3
jobA 0 1 6
jobB 2 2 9
jobC 4 1 4

- TimeSlice: Specifies the time quantum for the round-robin scheduler.
- job_name: The name of the job (e.g., jobA, jobB).
- arrival_time: The time at which the job arrives.
- priority: The priority of the job (lower values indicate higher priority).
- execution_time: The total execution time required for the job.

Output:
The program generates a "scheduler.log" file containing detailed scheduling events, including:
- Job forking and execution.
- Job time slice expiration and SIGSTOP signals.
- Job resumption and SIGCONT signals.
- Job completion and termination.

The same information is also printed to the terminal.

Cleanup:
To remove the generated executable and log file, use the following command:

make clean

This will delete the "scheduler" executable and the "scheduler.log" file.

Dependencies:
- A C compiler (gcc)
- Standard C library
- Basic Unix-like environment for fork(), signal(), and sleep() system calls.

Notes:
- The program simulates job execution using sleep() and does not execute external programs.
- The scheduling algorithm is a preemptive priority-based round-robin scheduler.
- Lower priority numbers indicate higher priority jobs.
- The scheduler prioritizes jobs with earlier arrival times and shorter remaining execution times in case of priority ties.
- Ensure that the "jobs.txt" file is correctly formatted.