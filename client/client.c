#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>  // For mkdir()
#include <fcntl.h>
#include <errno.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define BUFFER_SIZE 1024
#define DOWNLOAD_DIR "./downloads"
//This fucntion create the directory for download files
void create_download_dir() {
    // if directory exist it does not create, but otherwise create the directory
    struct stat st = {0};
    if (stat(DOWNLOAD_DIR, &st) == -1) {
        if (mkdir(DOWNLOAD_DIR, 0777) == -1) {
            perror("mkdir failed");
            exit(EXIT_FAILURE);
        }
        printf("Created downloads directory: %s\n", DOWNLOAD_DIR);
    }
}

void download_file(const char *filename) {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    
    // this part create the socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    serv_addr.sin_family = AF_INET;// this tell us working with IPv4
    serv_addr.sin_port = htons(SERVER_PORT);
    
    // Convert IPv4 address from text to binary form
    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        perror("invalid address");
        exit(EXIT_FAILURE);
    }
    
    // Connect to server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connection failed");
        exit(EXIT_FAILURE);
    }
    
    // Send HTTP GET request
    char request[BUFFER_SIZE];
    snprintf(request, BUFFER_SIZE, "GET /%s HTTP/1.1\r\nHost: %s\r\n\r\n", filename, SERVER_IP);
    send(sock, request, strlen(request), 0);
    
    // Create file to save the downloaded content
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/%s", DOWNLOAD_DIR, filename);
    
    FILE *file = fopen(filepath, "wb");
    if (!file) {
        perror("file creation failed");
        close(sock);
        exit(EXIT_FAILURE);
    }
    
    // Read response and save to file
    int bytes_received;
    int header_ended = 0;
    while ((bytes_received = recv(sock, buffer, BUFFER_SIZE, 0)) > 0) {
        if (!header_ended) {
            // Check for end of headers (empty line)
            char *header_end = strstr(buffer, "\r\n\r\n");
            if (header_end) {
                header_ended = 1;
                fwrite(header_end + 4, 1, bytes_received - (header_end - buffer) - 4, file);
            }
        } else {
            fwrite(buffer, 1, bytes_received, file);
        }
    }
    
    fclose(file);
    close(sock);
    printf("File downloaded successfully: %s\n", filepath);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <filename1> [filename2]\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    create_download_dir();
    
    for (int i = 1; i < argc; i++) {
        printf("Downloading file: %s\n", argv[i]);
        download_file(argv[i]);
    }
    
    return EXIT_SUCCESS;
}