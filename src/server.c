#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

int soln_counter = 1;

void handle_client(int client_socket, int client_id);

int main(int argc, char *argv[]) {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_size;
    pid_t pid;
    int port_number;
    int i;

    // Check command line arguments
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port_number>\n", argv[0]);
        exit(1);
    }
    port_number = atoi(argv[1]);

    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("ERROR opening socket");
        exit(1);
    }

    // Set up the server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port_number);

    // Bind the socket to the address
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("ERROR on binding");
        exit(1);
    }

    // Listen for connections
    listen(server_socket, MAX_CLIENTS);
    printf("Listening for clients...\n");

    client_addr_size = sizeof(client_addr);

    // Accept connections from clients
    for (i = 0; i < MAX_CLIENTS; i++) {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_size);
        if (client_socket < 0) {
            perror("ERROR on accept");
            exit(1);
        }

        printf("Connected with client %d\n", i + 1);

        // Fork a child process to handle the client
        pid = fork();
        if (pid < 0) {
            perror("ERROR on fork");
            exit(1);
        }

        if (pid == 0) {
            // This is the child process
            close(server_socket); // Child doesn't need the listener
            handle_client(client_socket, i + 1);
            exit(0);
        } else {
            // This is the parent process
            close(client_socket); // Parent doesn't need this
        }
    }

    // Cleanup
    close(server_socket);
    for (i = 0; i < MAX_CLIENTS; i++) {
        wait(NULL); // Wait for all child processes
    }

    return 0;
}

void handle_client(int client_socket, int client_id) {
    char buffer[BUFFER_SIZE];
    const char *end_of_message = "END_OF_MESSAGE\n";

    while (1) {
        ssize_t n = read(client_socket, buffer, sizeof(buffer) - 1);
        if (n < 0) {
            perror("ERROR reading from socket");
            exit(1);
        } else if (n == 0) {
            printf("Client %d closed the connection\n", client_id);
            break;
        }

        buffer[n] = '\0'; // Ensure the buffer is null-terminated

        char *command = strtok(buffer, " \n");
        if (command == NULL) {
            write(client_socket, "Empty command\n", 14);
            continue;
        }

        FILE *pipe_fp;
        char output_buffer[BUFFER_SIZE];

        if (strcmp(command, "matinvpar") == 0) {
            pipe_fp = popen("./matinvpar", "r");
        } else if (strcmp(command, "kmeanspar") == 0) {
            pipe_fp = popen("./kmeanspar", "r");
        } else {
            write(client_socket, "Unknown command\n", 16);
            continue;
        }

        if (pipe_fp == NULL) {
            perror("ERROR opening pipe");
            exit(1);
        }

        // Send the output of the command to the client immediately
        while (fgets(output_buffer, BUFFER_SIZE, pipe_fp) != NULL) {
            write(client_socket, output_buffer, strlen(output_buffer));
        }

        pclose(pipe_fp);

        // Send the solution file name to the client
        char response[BUFFER_SIZE];
        snprintf(response, sizeof(response), "Received the solution: %s_client%d_soln%d.txt\n", command, client_id, soln_counter++);
        write(client_socket, response, strlen(response));

        // Send the end-of-message indicator
        write(client_socket, end_of_message, strlen(end_of_message));

        printf("Client %d commanded: %s\n", client_id, command);
        printf("sending solution: %s.txt\n" , command);
    }

    close(client_socket);
}


