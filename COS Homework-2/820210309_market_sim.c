#include "820210309_market_sim.h"

FILE *log_fp;
int num_customers;
int num_products;
int reservation_timeout_ms;
int max_concurrent_payments;
int initial_stock[MAX_PRODUCTS];
RequestGroup request_groups[MAX_GROUPS];
int group_count = 0;
int stock[MAX_PRODUCTS];
int current_cashiers = 0;

long get_current_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000L + tv.tv_usec / 1000;
}

void parse_input(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("input.txt not found");
        exit(EXIT_FAILURE);
    }

    char line[256];
    int config_lines = 0;
    RequestGroup current_group;
    current_group.request_count = 0;

    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\r\n")] = 0;

        if (strlen(line) == 0) {
            if (current_group.request_count > 0) {
                request_groups[group_count++] = current_group;
                current_group.request_count = 0;
            }
            continue;
        }

        if (config_lines < 5) {
            char *equal_sign = strchr(line, '=');
            if (!equal_sign) continue;
            char *value = equal_sign + 1;

            if (strncmp(line, "num_customers=", 14) == 0) {
                num_customers = atoi(value);
            } else if (strncmp(line, "num_products=", 13) == 0) {
                num_products = atoi(value);
            } else if (strncmp(line, "reservation_timeout_ms=", 23) == 0) {
                reservation_timeout_ms = atoi(value);
            } else if (strncmp(line, "max_concurrent_payments=", 24) == 0) {
                max_concurrent_payments = atoi(value);
            } else if (strncmp(line, "initial_stock=", 14) == 0) {
                char *token = strtok(value, ",");
                int i = 0;
                while (token && i < MAX_PRODUCTS) {
                    initial_stock[i++] = atoi(token);
                    token = strtok(NULL, ",");
                }
            }
            config_lines++;
        } else {
            ProductRequest pr;
            sscanf(line, "%d,%d,%d", &pr.customer_id, &pr.product_id, &pr.quantity);
            current_group.requests[current_group.request_count++] = pr;
        }
    }

    if (current_group.request_count > 0) {
        request_groups[group_count++] = current_group;
    }

    fclose(fp);
}

void* reserve_thread(void *arg) {
    ThreadArg *req = (ThreadArg*) arg;
    pthread_mutex_lock(&stock_mutex);
    int can_reserve = stock[req->product_id] >= req->quantity;
    long ts = get_current_time_ms();
    if (can_reserve) {
        stock[req->product_id] -= req->quantity;
        req->reserved = 1;
        req->reservation_time = ts;
        LOG("[%ld] Customer %d tried to add product %d (qty: %d) to cart | Stock: [",
               ts, req->customer_id, req->product_id, req->quantity);
        for (int i = 0; i < num_products; i++)
            LOG("product %d: %d%s", i, stock[i], i < num_products - 1 ? ", " : "] // succeed\n");
    } else {
        LOG("[%ld] Customer %d tried to add product %d (qty: %d) but only %d available // failed\n",
               ts, req->customer_id, req->product_id, req->quantity, stock[req->product_id]);
        req->reserved = 0;
    }
    pthread_mutex_unlock(&stock_mutex);
    return NULL;
}

void* log_purchase_attempt(void *arg) {
    ThreadArg *req = (ThreadArg *)arg;
    long ts = get_current_time_ms();
    pthread_mutex_lock(&stock_mutex);
    LOG("[%ld] Customer %d attempted to purchase product %d (qty: %d) | Stock: [",
           ts, req->customer_id, req->product_id, req->quantity);
    for (int i = 0; i < num_products; i++)
        LOG("product %d: %d%s", i, stock[i], i < num_products - 1 ? ", " : "]\n");
    pthread_mutex_unlock(&stock_mutex);
    req->purchase_attempt_logged = 1;
    return NULL;
}

void* do_actual_purchase(void *arg) {
    ThreadArg *req = (ThreadArg *) arg;
    long deadline = req->reservation_time + reservation_timeout_ms;
    unsigned int local_rand_seed = req->rand_seed;
    int will_purchase = rand() % 2;

    if (will_purchase == 0) {
        usleep(reservation_timeout_ms * 1000);
        pthread_mutex_lock(&stock_mutex);
        stock[req->product_id] += req->quantity;
        long ts = get_current_time_ms();
        LOG("[%ld] Customer %d could not purchase product %d (qty: %d) in time. Timeout expired! Returned to stock.\n",
               ts, req->customer_id, req->product_id, req->quantity);
        pthread_mutex_unlock(&stock_mutex);
        return NULL;
    }

    while (get_current_time_ms() < deadline) {
        pthread_mutex_lock(&cashier_mutex);
        if (current_cashiers < max_concurrent_payments) {
            current_cashiers++;
            pthread_mutex_unlock(&cashier_mutex);
            usleep((rand_r(&local_rand_seed) % 10) * 1000);
            pthread_mutex_lock(&stock_mutex);
            long ts = get_current_time_ms();
            LOG("[%ld] Customer %d purchased product %d (qty: %d) | Stock: [",
                   ts, req->customer_id, req->product_id, req->quantity);
            for (int i = 0; i < num_products; i++)
                LOG("product %d: %d%s", i, stock[i], i < num_products - 1 ? ", " : "]\n");
            pthread_mutex_unlock(&stock_mutex);
            pthread_mutex_lock(&cashier_mutex);
            current_cashiers--;
            pthread_cond_signal(&cashier_cond);
            pthread_mutex_unlock(&cashier_mutex);
            return NULL;
        } else {
            long ts = get_current_time_ms();
            if (!req->retry_attempted) {
                LOG("[%ld] Customer %d couldn't purchase product %d (qty: %d) and had to wait! (maximum number of concurrent payments reached!)\n",
                       ts, req->customer_id, req->product_id, req->quantity);
                req->retry_attempted = 1;
            } else {
                LOG("[%ld] Customer %d (automatically) retried purchasing product %d (qty: %d) | Stock: [",
                       ts, req->customer_id, req->product_id, req->quantity);
                for (int i = 0; i < num_products; i++)
                    LOG("product %d: %d%s", i, stock[i], i < num_products - 1 ? ", " : "]\n");
            }
            pthread_cond_wait(&cashier_cond, &cashier_mutex);
            usleep(10000);
        }
        pthread_mutex_unlock(&cashier_mutex);
    }

    pthread_mutex_lock(&stock_mutex);
    stock[req->product_id] += req->quantity;
    long ts = get_current_time_ms();
    LOG("[%ld] Customer %d could not purchase product %d (qty: %d) in time. Timeout expired! Returned to stock.\n",
           ts, req->customer_id, req->product_id, req->quantity);
    pthread_mutex_unlock(&stock_mutex);
    return NULL;
}

int main() {
    log_fp = fopen("log.txt", "w");
    if (!log_fp) {
        perror("Could not open log.txt");
        exit(EXIT_FAILURE);
    }

    srand((unsigned)time(NULL));
    parse_input("input.txt");
    for (int i = 0; i < num_products; i++) {
        stock[i] = initial_stock[i];
    }
    long ts = get_current_time_ms();
    LOG("[%ld] Initial Stock: [", ts);
    for (int i = 0; i < num_products; i++) {
        LOG("product %d: %d%s", i, stock[i], i < num_products - 1 ? ", " : "]\n");
    }

    ThreadArg thread_args[50];
    for (int g = 0; g < group_count; g++) {
        int thread_count = request_groups[g].request_count;
        pthread_t res_threads[50];
        for (int r = 0; r < thread_count; r++) {
            thread_args[r].customer_id = request_groups[g].requests[r].customer_id;
            thread_args[r].product_id = request_groups[g].requests[r].product_id;
            thread_args[r].quantity = request_groups[g].requests[r].quantity;
            thread_args[r].reserved = 0;
            thread_args[r].purchase_attempt_logged = 0;
            thread_args[r].retry_attempted = 0;
            struct timeval tv;
            gettimeofday(&tv, NULL);
            thread_args[r].rand_seed = tv.tv_sec ^ tv.tv_usec ^ (r * 7919 + g * 1237);
            pthread_create(&res_threads[r], NULL, reserve_thread, &thread_args[r]);
        }
        for (int r = 0; r < thread_count; r++) {
            pthread_join(res_threads[r], NULL);
        }

        pthread_t attempt_threads[50], purchase_threads[50];
        for (int r = 0; r < thread_count; r++) {
            if (thread_args[r].reserved) {
                pthread_create(&attempt_threads[r], NULL, log_purchase_attempt, &thread_args[r]);
            }
        }
        for (int r = 0; r < thread_count; r++) {
            if (thread_args[r].reserved) {
                pthread_join(attempt_threads[r], NULL);
            }
        }
        for (int r = 0; r < thread_count; r++) {
            if (thread_args[r].reserved) {
                pthread_create(&purchase_threads[r], NULL, do_actual_purchase, &thread_args[r]);
            }
        }
        for (int r = 0; r < thread_count; r++) {
            if (thread_args[r].reserved) {
                pthread_join(purchase_threads[r], NULL);
            }
        }
        usleep(150000);
    }

    fclose(log_fp);
    return 0;
}
