Project: 820210309_market_sim
Author: 820210309

Description:
------------
This project simulates a concurrent online market where customers attempt to reserve and purchase products.
The simulation reads configuration and request data from input.txt, handles reservations and purchases using threads, 
and logs all activity to both the terminal and log.txt.

Files:
------
- 820210309_market_sim.c      : Main source code with simulation logic
- 820210309_market_sim.h      : Header file with shared definitions and declarations
- input.txt                   : Input file with market configuration and grouped customer requests
- log.txt                     : Output log file generated at runtime
- Makefile                    : To compile, run, or clean the project
- README.txt                  : This file

How to Compile:
---------------
Open terminal in the project directory and run:
    make

How to Run:
-----------
After compilation:
    ./market_sim

Or use:
    make run

How to Clean:
-------------
To remove compiled files and logs:
    make clean

Notes:
------
- Ensure input.txt exists and is properly formatted before running.
- Logs are printed both to the terminal and saved in log.txt.
- Threads within each group start simultaneously. Groups are executed sequentially with a delay.

