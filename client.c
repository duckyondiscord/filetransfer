#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define SERVER_IP argv[1] // Replace with server IP
#define SERVER_PORT argv[2] // Replace with server port
#define BUFFER_SIZE 1024 // Size of the chunks we will split the file up into

int main(int argc, char* argv[]) {

    setbuf(stdout, NULL);
    if(argc < 4) {
        printf("Usage:\n%s <server IP> <server port> <output file name or path>\n", argv[0]);
        return 1;
    } // If there's less than 4 arguments, we print the usage text.

    // Define the variable that will be used for the client socket
    int client_socket;
    struct sockaddr_in server_address;

    // Create the client socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Convert the port argument into an integer
    int port = atoi(SERVER_PORT);

    // Set up server address
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    if (inet_pton(AF_INET, SERVER_IP, &(server_address.sin_addr)) <= 0) {
        perror("Invalid address or address not supported");
        exit(EXIT_FAILURE);
    }

    // Connect to the server
    if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    
    // Open the file
    FILE* file;
    file = fopen(argv[3], "w");
    if (file == NULL) {
        perror("Failed to open file");
        exit(EXIT_FAILURE);
    }

    // Receive the file from the server
    char buffer[BUFFER_SIZE];
    ssize_t numBytesReceived;
    size_t totalBytesReceived = 0;
    size_t bytesReceivedSoFar = 0;


    // Keep receiving shit until we reach the amount of bytes we need to receive, constantly printing the progress
    while ((numBytesReceived = recv(client_socket, buffer, BUFFER_SIZE, 0)) > 0) {
        fwrite(buffer, sizeof(char), numBytesReceived, file);
        totalBytesReceived += numBytesReceived;
        bytesReceivedSoFar += numBytesReceived;
        printf("Received %ld megabytes so far", bytesReceivedSoFar / 1000000);
        printf("\r");
    }

    if (numBytesReceived < 0) {
        perror("File receiving failed");
        exit(EXIT_FAILURE);
    }

    printf("\rReceived %zu megabytes from the server.\n", totalBytesReceived / 1000000);

    // Close the file
    fclose(file);

    // Close the socket
    close(client_socket);

    return 0;
}

