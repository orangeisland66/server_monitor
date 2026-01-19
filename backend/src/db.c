#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include "monitor.h"
#include "sqlite3.h"

static sqlite3 *db = NULL;

int db_init(const char *db_path) {
    int rc = sqlite3_open(db_path, &db);
    if (rc) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    const char *sql = 
        "CREATE TABLE IF NOT EXISTS stats ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "timestamp INTEGER,"
        "cpu_usage REAL,"
        "memory_usage REAL,"
        "net_rx_rate INTEGER,"
        "net_tx_rate INTEGER,"
        "disk_read_rate INTEGER,"
        "disk_write_rate INTEGER"
        ");"
        "CREATE INDEX IF NOT EXISTS idx_timestamp ON stats(timestamp);";

    char *zErrMsg = 0;
    rc = sqlite3_exec(db, sql, 0, 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        return -1;
    }
    
    // Set PRAGMAs for performance
    sqlite3_exec(db, "PRAGMA synchronous = NORMAL; PRAGMA journal_mode = WAL;", 0, 0, 0);

    return 0;
}

int db_insert_metrics(SystemMetrics *metrics) {
    if (!db) return -1;

    char sql[512];
    snprintf(sql, sizeof(sql), 
        "INSERT INTO stats (timestamp, cpu_usage, memory_usage, net_rx_rate, net_tx_rate, disk_read_rate, disk_write_rate) "
        "VALUES (%ld, %.2f, %.2f, %lu, %lu, %lu, %lu);",
        metrics->timestamp, 
        metrics->cpu_usage, 
        metrics->memory_usage,
        metrics->net_rx_rate,
        metrics->net_tx_rate,
        metrics->disk_read_rate,
        metrics->disk_write_rate
    );

    char *zErrMsg = 0;
    int rc = sqlite3_exec(db, sql, 0, 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        return -1;
    }
    return 0;
}

// Helper to append JSON string
static void json_append(char **json, size_t *len, size_t *cap, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    
    // Determine needed size
    va_list args_copy;
    va_copy(args_copy, args);
    int needed = vsnprintf(NULL, 0, fmt, args_copy) + 1; // +1 for null terminator
    va_end(args_copy);
    
    if (*len + needed >= *cap) {
        *cap = (*cap + needed) * 2;
        *json = realloc(*json, *cap);
    }
    
    vsnprintf(*json + *len, needed, fmt, args);
    *len += needed - 1; // Subtract null terminator from length count
    va_end(args);
}

char* db_query_history(const char *span) {
    if (!db) return NULL;

    time_t now = time(NULL);
    time_t start_time = now;
    int interval = 1; // Group by seconds

    if (strcmp(span, "30d") == 0) {
        start_time = now - 30 * 24 * 3600;
        interval = 3600 * 4; // 4 hour points? or 1 day? Let's say 6 hours.
    } else if (strcmp(span, "7d") == 0) {
        start_time = now - 7 * 24 * 3600;
        interval = 3600; // 1 hour
    } else if (strcmp(span, "1d") == 0) {
        start_time = now - 24 * 3600;
        interval = 300; // 5 mins
    } else if (strcmp(span, "1h") == 0) {
        start_time = now - 3600;
        interval = 60; // 1 min
    } else if (strcmp(span, "1m") == 0) {
        start_time = now - 60;
        interval = 1; // 1 sec
    } else { // Realtime or default
        start_time = now - 60; // Last 60 seconds
        interval = 1;
    }

    char sql[1024];
    // Use SQLite aggregation to downsample
    snprintf(sql, sizeof(sql),
        "SELECT "
        "  (timestamp / %d) * %d as ts_group, "
        "  AVG(cpu_usage), "
        "  AVG(memory_usage), "
        "  AVG(net_rx_rate), "
        "  AVG(net_tx_rate), "
        "  AVG(disk_read_rate), "
        "  AVG(disk_write_rate) "
        "FROM stats "
        "WHERE timestamp >= %ld "
        "GROUP BY ts_group "
        "ORDER BY ts_group ASC;",
        interval, interval, start_time
    );

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK) {
        return NULL;
    }

    size_t cap = 4096;
    size_t len = 0;
    char *json = malloc(cap);
    json[0] = '[';
    len = 1;

    int first = 1;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (!first) {
            json_append(&json, &len, &cap, ",");
        }
        first = 0;
        
        time_t ts = sqlite3_column_int64(stmt, 0);
        double cpu = sqlite3_column_double(stmt, 1);
        double mem = sqlite3_column_double(stmt, 2);
        double net_rx = sqlite3_column_double(stmt, 3);
        double net_tx = sqlite3_column_double(stmt, 4);
        double disk_read = sqlite3_column_double(stmt, 5);
        double disk_write = sqlite3_column_double(stmt, 6);

        json_append(&json, &len, &cap, 
            "{\"timestamp\":%ld,\"cpu\":%.2f,\"memory\":%.2f,\"net_rx\":%.0f,\"net_tx\":%.0f,\"disk_read\":%.0f,\"disk_write\":%.0f}",
            ts, cpu, mem, net_rx, net_tx, disk_read, disk_write
        );
    }

    json_append(&json, &len, &cap, "]");
    sqlite3_finalize(stmt);
    
    return json;
}

void db_close() {
    if (db) sqlite3_close(db);
}
