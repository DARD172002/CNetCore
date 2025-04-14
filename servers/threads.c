#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>

#define PORT 8080
#define BUFFER_SIZE 65536
#define FILES_DIR "./files"

void *handle_client(void *arg);

int main() {
    int server_fd, client_fd;
    struct sockaddr_in address;
    socklen_t addr_len = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;  // Accept connections on any interface
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    while (1) {
        client_fd = accept(server_fd, (struct sockaddr *)&address, &addr_len);
        if (client_fd < 0) {
            perror("Accept failed");
            continue;
        }

        pthread_t tid;
        int *pclient = malloc(sizeof(int));
        *pclient = client_fd;
        pthread_create(&tid, NULL, handle_client, pclient);
        pthread_detach(tid);  // Automatically free thread resources on finish
    }

    close(server_fd);
    return 0;
}

void *handle_client(void *arg) {
    int client_fd = *((int *)arg);
    free(arg);

    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    recv(client_fd, buffer, BUFFER_SIZE - 1, 0);

    char method[5], filename[256];
    if (sscanf(buffer, "%s /%s", method, filename) != 2) {
        fprintf(stderr, "Failed to parse request\n");
        close(client_fd);
        return NULL;
    }

    printf("Client requested file: %s\n", filename);

    char filepath[512];
    snprintf(filepath, sizeof(filepath), "%s/%s", FILES_DIR, filename);

    FILE *fp = fopen(filepath, "rb");
    if (!fp) {
        const char *not_found = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
        send(client_fd, not_found, strlen(not_found), 0);
        close(client_fd);
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    long filesize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char header[256];
    snprintf(header, sizeof(header),
             "HTTP/1.1 200 OK\r\nContent-Length: %ld\r\nContent-Type: application/octet-stream\r\n\r\n",
             filesize);
    send(client_fd, header, strlen(header), 0);

    size_t n;
    while ((n = fread(buffer, 1, BUFFER_SIZE, fp)) > 0) {
        send(client_fd, buffer, n, 0);
        usleep(500000); //Sleep para que la descarga dure un poco mas y se puedan testear los threads  
    }

    fclose(fp);
    close(client_fd);
    printf("Finished sending file: %s\n", filename);
    return NULL;
}
