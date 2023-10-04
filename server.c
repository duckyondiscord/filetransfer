#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <string.h>

#define SA struct sockaddr
#define CHUNK_SIZE 1024

int sockfd;
FILE* file;
char* source = NULL;

void handle_sigint(int sig) {
    printf("SIGINT received, terminating gracefully.");
    close(sockfd);
    fclose(file);
    free(source);
    exit(0);
}

int main(int argc, char* argv[]) {
    struct sockaddr_in servaddr = {};

    setbuf(stdout, NULL);
    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("socket creation failed...\n");
        exit(0);
    }

    if (argc < 3) {
        printf("Usage:\n%s <port> <filename>", argv[0]);
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[1]);
    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);

    // Binding newly created socket to given IP and port
    if (bind(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) {
        printf("Could not bind to address/port\n");
        exit(EXIT_FAILURE);
    }
    // Now server is ready to listen

    // Register signal handler for SIGINT
    signal(SIGINT, handle_sigint);

    listen(sockfd, 1);
    printf("Server listening on port %d\n", port);

    // Open the file for reading
    file = fopen(argv[2], "rb");

    if (file == NULL) {
        perror("Failed to open file");
        exit(EXIT_FAILURE);
    }

    fseek(file, 0L, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0L, SEEK_SET);

    source = malloc(fileSize);
    if (source == NULL) {
        printf("Error allocating memory for file...\n");
        exit(EXIT_FAILURE);
    }

    size_t numBytesRead = fread(source, sizeof(char), fileSize, file);
    if (numBytesRead != fileSize) {
        printf("Error reading file into memory...\n");
        exit(EXIT_FAILURE);
    }

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_size = sizeof(client_addr);
        int client_sock = accept(sockfd, (SA*)&client_addr, &client_size);
        if (client_sock == -1) {
            printf("Error accepting connection...\n");
            continue;
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);

        char* sendbuf = source;
        size_t remaining_bytes = fileSize;

        while (remaining_bytes > 0) {
            size_t chunk_size = (remaining_bytes < CHUNK_SIZE) ? remaining_bytes : CHUNK_SIZE;
            ssize_t bytes_sent = send(client_sock, sendbuf, chunk_size, 0);
            if (bytes_sent == -1) {
                printf("Error sending file chunk...\n");
                exit(EXIT_FAILURE);
            }

            sendbuf += bytes_sent;
            remaining_bytes -= bytes_sent;
        }

        printf("File sent to client %s.\n", client_ip);

        close(client_sock);
    }

    close(sockfd);
    fclose(file);

    return 0;
}

