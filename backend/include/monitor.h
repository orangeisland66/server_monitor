#ifndef MONITOR_H
#define MONITOR_H

#include <stdint.h>
#include <time.h>
#include "sqlite3.h"

// System metrics structure
typedef struct {
    double cpu_usage;       // Percentage (0-100)
    double memory_usage;    // Percentage (0-100)
    uint64_t net_rx_bytes;  // Total bytes received
    uint64_t net_tx_bytes;  // Total bytes transmitted
    uint64_t disk_read_sectors;  // Total sectors read
    uint64_t disk_write_sectors; // Total sectors written
    
    // Calculated rates (per second)
    uint64_t net_rx_rate;
    uint64_t net_tx_rate;
    uint64_t disk_read_rate;
    uint64_t disk_write_rate;
    
    time_t timestamp;
} SystemMetrics;

// Database functions
int db_init(const char *db_path);
int db_insert_metrics(SystemMetrics *metrics);
// Query metrics: returns JSON string that must be freed by caller
char* db_query_history(const char *span); 
void db_close();

// Monitor functions
void monitor_init();
int monitor_collect(SystemMetrics *metrics);

// Server functions
void server_start(int port);

#endif
