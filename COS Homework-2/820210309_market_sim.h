#ifndef MARKET_SIM_H
#define MARKET_SIM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>

#define MAX_PRODUCTS 100
#define MAX_REQUESTS 500
#define MAX_GROUPS 100

#define LOG(...) do { \
    fprintf(log_fp, __VA_ARGS__); \
    fflush(log_fp); \
    printf(__VA_ARGS__); \
} while (0)

typedef struct {
    int customer_id;
    int product_id;
    int quantity;
} ProductRequest;

typedef struct {
    ProductRequest requests[50];
    int request_count;
} RequestGroup;

typedef struct {
    int customer_id;
    int product_id;
    int quantity;
    int reserved;
    long reservation_time;
    int purchase_attempt_logged;
    unsigned int rand_seed;
    int retry_attempted;
} ThreadArg;

extern FILE *log_fp;
extern int num_customers;
extern int num_products;
extern int reservation_timeout_ms;
extern int max_concurrent_payments;
extern int initial_stock[MAX_PRODUCTS];
extern RequestGroup request_groups[MAX_GROUPS];
extern int group_count;
extern int stock[MAX_PRODUCTS];
extern int current_cashiers;

pthread_mutex_t stock_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t cashier_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cashier_cond = PTHREAD_COND_INITIALIZER;

long get_current_time_ms();
void parse_input(const char *filename);
void* reserve_thread(void *arg);
void* log_purchase_attempt(void *arg);
void* do_actual_purchase(void *arg);

#endif
