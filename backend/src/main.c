#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "monitor.h"

#define DB_PATH "metrics.db"
#define PORT 8080

void *server_thread_func(void *arg) {
    (void)arg;
    server_start(PORT);
    return NULL;
}

int main() {
    printf("Starting Server Monitor Backend...\n");

    // Initialize Database
    if (db_init(DB_PATH) != 0) {
        fprintf(stderr, "Failed to initialize database.\n");
        return 1;
    }

    // Initialize Monitor
    monitor_init();

    // Start Server in a separate thread
    pthread_t server_tid;
    if (pthread_create(&server_tid, NULL, server_thread_func, NULL) != 0) {
        fprintf(stderr, "Failed to start server thread.\n");
        db_close();
        return 1;
    }
    pthread_detach(server_tid);

    // Main loop: Collect metrics every second
    printf("Monitoring started. Press Ctrl+C to stop.\n");
    while (1) {
        SystemMetrics metrics;
        if (monitor_collect(&metrics) == 0) {
            if (db_insert_metrics(&metrics) != 0) {
                fprintf(stderr, "Failed to insert metrics.\n");
            }
            // Optional: Print to console
            // printf("CPU: %.1f%% Mem: %.1f%%\n", metrics.cpu_usage, metrics.memory_usage);
        }
        sleep(1);
    }

    db_close();
    return 0;
}
