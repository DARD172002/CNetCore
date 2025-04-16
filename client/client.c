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
#define SERVER_PORT 8080

#define SERVER_IP "127.0.0.1"
#define BUFFER_SIZE 65536
#define DOWNLOAD_DIR "./downloads"
//This fucntion create the directory for download files
void create_download_dir(int server_port) {
    // if directory exist it does not create, but otherwise create the directory
    printf("El puerto es %d ",server_port);
    struct stat st = {0};
    if (stat(DOWNLOAD_DIR, &st) == -1) {
        if (mkdir(DOWNLOAD_DIR, 0777) == -1) {
            perror("mkdir failed");
            exit(EXIT_FAILURE);
        }
        printf("Created downloads directory: %s\n", DOWNLOAD_DIR);
    }
}

void download_file(const char *filename, int server_port) {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    
    // Crear socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(server_port);
    
    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        perror("invalid address");
        exit(EXIT_FAILURE);
    }
    
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connection failed");
        exit(EXIT_FAILURE);
    }
    
    // Enviar solicitud GET
    char request[BUFFER_SIZE];
    snprintf(request, BUFFER_SIZE, "GET /%s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", filename, SERVER_IP);
    send(sock, request, strlen(request), 0);
    
    // Leer respuesta inicial (cabecera)
    int bytes_received = recv(sock, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received <= 0) {
        perror("failed to receive response");
        close(sock);
        return;
    }
    buffer[bytes_received] = '\0';

    // Verificar si es 404
    if (strstr(buffer, "404 Not Found")) {
        printf("Error: El archivo \"%s\" no fue encontrado en el servidor.\n", filename);
        close(sock);
        return;
    }

    // Crear ruta para guardar archivo
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/%s", DOWNLOAD_DIR, filename);
    
    FILE *file = fopen(filepath, "wb");
    if (!file) {
        perror("file creation failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // Buscar fin de cabecera y escribir solo el contenido
    char *header_end = strstr(buffer, "\r\n\r\n");
    if (header_end) {
        int body_offset = (header_end - buffer) + 4;
        int body_length = bytes_received - body_offset;
        if (body_length > 0) {
            fwrite(buffer + body_offset, 1, body_length, file);
        }
    }

    // Leer y escribir el resto del cuerpo
    while ((bytes_received = recv(sock, buffer, BUFFER_SIZE, 0)) > 0) {
        fwrite(buffer, 1, bytes_received, file);
    }

    fclose(file);
    close(sock);
    printf("Archivo descargado correctamente: %s\n", filepath);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <filename1> [filename2]\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    int server_port= atoi(argv[1]);

    
    create_download_dir(server_port);
    
    for (int i = 2; i < argc; i++) {
        printf("Descargando archivo: %s\n", argv[i]);
        download_file(argv[i],server_port);
    }
    
    return EXIT_SUCCESS;
}