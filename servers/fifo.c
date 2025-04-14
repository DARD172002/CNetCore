#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <dirent.h>
#include <asm-generic/socket.h>

#define PORT 8080 //Port define for listen
#define BUFFER_SIZE 1024 // size of buffer
#define ROOT_DIR "./www" //Directory where the files shared are located
 
int server_fd;

void handle_signal(int sig) {
    printf("\nShutting down server...\n");
    close(server_fd);
    exit(0);
}

void send_response(int client_socket, const char *status, const char *content_type, const char *content, int content_length) {
    char headers[BUFFER_SIZE];
    snprintf(headers, BUFFER_SIZE, 
             "HTTP/1.1 %s\r\n"
             "Content-Type: %s\r\n"
             "Content-Length: %d\r\n"
             "Connection: close\r\n\r\n",
             status, content_type, content_length);
    
    write(client_socket, headers, strlen(headers));
    write(client_socket, content, content_length);
}

void send_error_response(int client_socket, const char *status, const char *message) {
    char content[BUFFER_SIZE];
    snprintf(content, BUFFER_SIZE, 
             "<html><body><h1>%s</h1><p>%s</p></body></html>", 
             status, message);
    
    send_response(client_socket, status, "text/html", content, strlen(content));
}

void list_directory(int client_socket, const char *path) {
    DIR *dir;
    struct dirent *ent;
    char full_path[512];
    char content[BUFFER_SIZE * 4] = "<html><body><h1>Directory Listing</h1><ul>";
    
    snprintf(full_path, sizeof(full_path), "%s%s", ROOT_DIR, path);
    
    if ((dir = opendir(full_path)) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0) {
                strcat(content, "<li><a href=\"");
                strcat(content, ent->d_name);
                strcat(content, "\">");
                strcat(content, ent->d_name);
                strcat(content, "</a></li>");
            }
        }
        closedir(dir);
        strcat(content, "</ul></body></html>");
        send_response(client_socket, "200 OK", "text/html", content, strlen(content));
    } else {
        send_error_response(client_socket, "500 Internal Server Error", "Could not list directory");
    }
}

void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE * 2] = {0};
    read(client_socket, buffer, sizeof(buffer) - 1);

    // Parse the HTTP request
    char method[16], path[256], protocol[16];
    sscanf(buffer, "%s %s %s", method, path, protocol);

    // Handle GET requests only
    if (strcmp(method, "GET") != 0) {
        send_error_response(client_socket, "405 Method Not Allowed", "Only GET method is supported");
        close(client_socket);
        return;
    }

    // Handle root path
    if (strcmp(path, "/") == 0) {
        strcpy(path, "/index.html");
    }

    // Build file path
    char file_path[512];
    snprintf(file_path, sizeof(file_path), "%s%s", ROOT_DIR, path);

    // Check if path is a directory
    struct stat path_stat;
    if (stat(file_path, &path_stat) == 0) {
        if (S_ISDIR(path_stat.st_mode)) {
            list_directory(client_socket, path);
            close(client_socket);
            return;
        }
    }

    // Open the file rb mean read binary
    FILE *file = fopen(file_path, "rb");
    if (!file) {
        send_error_response(client_socket, "404 Not Found", "The requested file was not found");
        close(client_socket);
        return;
    }

    const char *content_type = "text/plain";
    if (strstr(path, ".html")) content_type = "text/html";
    else if (strstr(path, ".css")) content_type = "text/css";
    else if (strstr(path, ".js")) content_type = "application/javascript";
    else if (strstr(path, ".jpg") || strstr(path, ".jpeg")) content_type = "image/jpeg";
    else if (strstr(path, ".png")) content_type = "image/png";
    else if (strstr(path, ".gif")) content_type = "image/gif";
    else if (strstr(path, ".pdf")) content_type = "application/pdf";

    // Read file contents
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char *file_content = malloc(file_size);
    if (!file_content) {
        send_error_response(client_socket, "500 Internal Server Error", "Memory allocation failed");
        fclose(file);
        close(client_socket);
        return;
    }
    
    fread(file_content, 1, file_size, file);
    fclose(file);

    // Send response
    send_response(client_socket, "200 OK", content_type, file_content, file_size);
    free(file_content);
    close(client_socket);
}

int main() {
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Create socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

   
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for connections
    if (listen(server_fd, 10) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("FIFO Server listening on port %d\n", PORT);

    // Create root directory if it doesn't exist
    struct stat st = {0};
    if (stat(ROOT_DIR, &st) == -1) {
        if (mkdir(ROOT_DIR, 0755) == -1) {
            perror("mkdir failed");
            exit(EXIT_FAILURE);
        }
        printf("Created root directory: %s\n", ROOT_DIR);
    }

    // Set up signal handler for graceful shutdown
    signal(SIGINT, handle_signal);

   
    while (1) {
        int client_socket;
        if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            continue;
        }

        printf("New connection from %s:%d\n", 
               inet_ntoa(address.sin_addr), ntohs(address.sin_port));

        // Handle client request (sequentially)
        handle_client(client_socket);
    }

    return 0;
}