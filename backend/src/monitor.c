#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "monitor.h"

// Previous states for calculating rates
static uint64_t prev_cpu_user = 0, prev_cpu_nice = 0, prev_cpu_system = 0, prev_cpu_idle = 0;
static uint64_t prev_cpu_iowait = 0, prev_cpu_irq = 0, prev_cpu_softirq = 0, prev_cpu_steal = 0;
static uint64_t prev_net_rx = 0, prev_net_tx = 0;
static uint64_t prev_disk_read = 0, prev_disk_write = 0;
static int first_run = 1;

void monitor_init() {
    // Initial reading to set baseline
    SystemMetrics m;
    monitor_collect(&m);
    first_run = 0;
}

static double get_cpu_usage() {
    FILE *fp = fopen("/proc/stat", "r");
    if (!fp) return 0.0;

    char buffer[1024];
    if (!fgets(buffer, sizeof(buffer), fp)) {
        fclose(fp);
        return 0.0;
    }
    fclose(fp);

    uint64_t user, nice, system, idle, iowait, irq, softirq, steal;
    // cpu  user nice system idle iowait irq softirq steal guest guest_nice
    sscanf(buffer, "cpu  %lu %lu %lu %lu %lu %lu %lu %lu",
           &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal);

    uint64_t prev_idle_total = prev_cpu_idle + prev_cpu_iowait;
    uint64_t idle_total = idle + iowait;

    uint64_t prev_non_idle = prev_cpu_user + prev_cpu_nice + prev_cpu_system + prev_cpu_irq + prev_cpu_softirq + prev_cpu_steal;
    uint64_t non_idle = user + nice + system + irq + softirq + steal;

    uint64_t prev_total = prev_idle_total + prev_non_idle;
    uint64_t total = idle_total + non_idle;

    uint64_t total_d = total - prev_total;
    uint64_t idle_d = idle_total - prev_idle_total;

    // Update prev
    prev_cpu_user = user; prev_cpu_nice = nice; prev_cpu_system = system;
    prev_cpu_idle = idle; prev_cpu_iowait = iowait; prev_cpu_irq = irq;
    prev_cpu_softirq = softirq; prev_cpu_steal = steal;

    if (total_d == 0) return 0.0;
    return (double)(total_d - idle_d) / total_d * 100.0;
}

static double get_memory_usage() {
    FILE *fp = fopen("/proc/meminfo", "r");
    if (!fp) return 0.0;

    char buffer[256];
    uint64_t total = 0, available = 0;
    
    while (fgets(buffer, sizeof(buffer), fp)) {
        if (strncmp(buffer, "MemTotal:", 9) == 0) {
            sscanf(buffer, "MemTotal: %lu", &total);
        } else if (strncmp(buffer, "MemAvailable:", 13) == 0) {
            sscanf(buffer, "MemAvailable: %lu", &available);
        }
    }
    fclose(fp);

    if (total == 0) return 0.0;
    return (double)(total - available) / total * 100.0;
}

static void get_network_stats(uint64_t *rx, uint64_t *tx) {
    FILE *fp = fopen("/proc/net/dev", "r");
    if (!fp) {
        *rx = 0; *tx = 0;
        return;
    }

    char buffer[1024];
    // Skip headers
    fgets(buffer, sizeof(buffer), fp);
    fgets(buffer, sizeof(buffer), fp);

    uint64_t total_rx = 0, total_tx = 0;
    
    while (fgets(buffer, sizeof(buffer), fp)) {
        char *interface = buffer;
        while (*interface == ' ') interface++; // Skip leading spaces
        
        // Very basic parsing, assuming typical format
        // Interface name ends with :
        char *colon = strchr(interface, ':');
        if (!colon) continue;
        
        uint64_t r, t, dummy;
        // face |bytes    packets errs drop fifo frame compressed multicast|bytes    packets errs drop fifo colls carrier compressed
        // Using %*d to skip fields
        if (sscanf(colon + 1, "%lu %lu %lu %lu %lu %lu %lu %lu %lu",
            &r, &dummy, &dummy, &dummy, &dummy, &dummy, &dummy, &dummy, &t) == 9) {
            total_rx += r;
            total_tx += t;
        }
    }
    fclose(fp);
    *rx = total_rx;
    *tx = total_tx;
}

static void get_disk_stats(uint64_t *read_sectors, uint64_t *write_sectors) {
    FILE *fp = fopen("/proc/diskstats", "r");
    if (!fp) {
        *read_sectors = 0; *write_sectors = 0;
        return;
    }

    char buffer[1024];
    uint64_t total_read = 0, total_write = 0;

    while (fgets(buffer, sizeof(buffer), fp)) {
        uint32_t major, minor;
        char name[32];
        uint64_t r_ios, r_merges, r_sectors, r_ticks;
        uint64_t w_ios, w_merges, w_sectors, w_ticks;
        uint64_t ios_in_progress, total_ticks, weighted_ticks;

        // 8 0 sda 123 0 1234 100 0 0 0 0 0 0 0
        if (sscanf(buffer, "%u %u %s %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu",
            &major, &minor, name,
            &r_ios, &r_merges, &r_sectors, &r_ticks,
            &w_ios, &w_merges, &w_sectors, &w_ticks,
            &ios_in_progress, &total_ticks, &weighted_ticks) >= 11) {
            
            // Only count physical devices (e.g., sda, nvme0n1) and ignore partitions/loop/ram
            if (strncmp(name, "sd", 2) == 0 && !strchr(name + 2, '0') && !strchr(name + 2, '1') /* rough check for no digits */) {
                // Actually, simple check: if it ends with digit, it might be partition.
                // But sda is a disk. sda1 is partition.
                // Let's just sum everything that looks like a disk.
                // A better heuristic: if minor % 16 == 0 for sd* devices.
            }
            // For simplicity, sum everything that is likely a physical disk.
            // Or just sum all. But partitions are subsets of disks, so double counting.
            // Let's filter common names: sda, vda, xvda, nvme0n1.
            // If name ends in digit, skip if it's sd/vd/xvd
            
            int is_disk = 0;
            if (strncmp(name, "sd", 2) == 0 || strncmp(name, "vd", 2) == 0) {
                 // Check if last char is digit
                 char last = name[strlen(name)-1];
                 if (last < '0' || last > '9') is_disk = 1; 
            } else if (strncmp(name, "nvme", 4) == 0) {
                // nvme0n1 is disk, nvme0n1p1 is partition
                if (strstr(name, "p") == NULL) is_disk = 1;
            } else if (strncmp(name, "mmcblk", 6) == 0) {
                 if (strstr(name, "p") == NULL) is_disk = 1;
            }

            if (is_disk) {
                total_read += r_sectors;
                total_write += w_sectors;
            }
        }
    }
    fclose(fp);
    *read_sectors = total_read;
    *write_sectors = total_write;
}

int monitor_collect(SystemMetrics *metrics) {
    metrics->timestamp = time(NULL);
    
    // CPU and Mem are instantaneous (or based on internal kernel counters since boot)
    // CPU usage calculation depends on prev counters which are updated in get_cpu_usage
    metrics->cpu_usage = get_cpu_usage();
    metrics->memory_usage = get_memory_usage();

    uint64_t rx, tx, dread, dwrite;
    get_network_stats(&rx, &tx);
    get_disk_stats(&dread, &dwrite);

    metrics->net_rx_bytes = rx;
    metrics->net_tx_bytes = tx;
    metrics->disk_read_sectors = dread;
    metrics->disk_write_sectors = dwrite;

    // Calculate rates
    if (first_run) {
        metrics->net_rx_rate = 0;
        metrics->net_tx_rate = 0;
        metrics->disk_read_rate = 0;
        metrics->disk_write_rate = 0;
    } else {
        // Assume 1 second interval roughly
        // Handle overflow (unlikely for 64bit counters in short time, but possible for wrap around)
        metrics->net_rx_rate = (rx >= prev_net_rx) ? (rx - prev_net_rx) : 0;
        metrics->net_tx_rate = (tx >= prev_net_tx) ? (tx - prev_net_tx) : 0;
        metrics->disk_read_rate = (dread >= prev_disk_read) ? (dread - prev_disk_read) : 0;
        metrics->disk_write_rate = (dwrite >= prev_disk_write) ? (dwrite - prev_disk_write) : 0;
    }

    prev_net_rx = rx;
    prev_net_tx = tx;
    prev_disk_read = dread;
    prev_disk_write = dwrite;

    return 0;
}
