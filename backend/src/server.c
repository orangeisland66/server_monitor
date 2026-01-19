#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include "monitor.h"

#define BUFFER_SIZE 4096

// Helper to parse query params
static void get_query_param(const char *request, const char *param, char *value, int max_len) {
    char key[64];
    snprintf(key, sizeof(key), "%s=", param);
    char *start = strstr(request, key);
    if (start) {
        start += strlen(key);
        int i = 0;
        while (i < max_len - 1 && start[i] && start[i] != ' ' && start[i] != '&') {
            value[i] = start[i];
            i++;
        }
        value[i] = '\0';
    } else {
        value[0] = '\0';
    }
}

static void handle_client(int client_fd) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
    if (bytes_read <= 0) {
        close(client_fd);
        return;
    }
    buffer[bytes_read] = '\0';

    // Check for CORS preflight
    if (strncmp(buffer, "OPTIONS", 7) == 0) {
        const char *response = "HTTP/1.1 204 No Content\r\n"
                               "Access-Control-Allow-Origin: *\r\n"
                               "Access-Control-Allow-Methods: GET, OPTIONS\r\n"
                               "Access-Control-Allow-Headers: Content-Type\r\n"
                               "\r\n";
        write(client_fd, response, strlen(response));
        close(client_fd);
        return;
    }

    if (strncmp(buffer, "GET /api/history", 16) == 0) {
        char span[16] = "realtime";
        get_query_param(buffer, "span", span, sizeof(span));
        
        char *json = db_query_history(span);
        if (json) {
            char header[256];
            snprintf(header, sizeof(header), 
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: application/json\r\n"
                "Access-Control-Allow-Origin: *\r\n"
                "Content-Length: %lu\r\n"
                "\r\n", strlen(json));
            write(client_fd, header, strlen(header));
            write(client_fd, json, strlen(json));
            free(json);
        } else {
            const char *response = "HTTP/1.1 500 Internal Server Error\r\nAccess-Control-Allow-Origin: *\r\n\r\n{\"error\":\"db error\"}";
            write(client_fd, response, strlen(response));
        }
    } else {
        const char *response = "HTTP/1.1 404 Not Found\r\nAccess-Control-Allow-Origin: *\r\n\r\n";
        write(client_fd, response, strlen(response));
    }
    
    close(client_fd);
}

static void *client_thread(void *arg) {
    int client_fd = *((int *)arg);
    free(arg);
    handle_client(client_fd);
    return NULL;
}

void server_start(int port) {
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", port);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addrlen = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addrlen);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }
        
        pthread_t thread_id;
        int *pclient = malloc(sizeof(int));
        *pclient = client_fd;
        
        if (pthread_create(&thread_id, NULL, client_thread, pclient) != 0) {
            perror("pthread_create");
            free(pclient);
            close(client_fd);
        } else {
            pthread_detach(thread_id);
        }
    }
}
