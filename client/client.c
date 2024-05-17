#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>


#define BUFFER_SIZE 1024

int server_socket; // Global variable for the server socket
volatile sig_atomic_t running = 1; // Global variable to control the heartbeat thread

void handle_server_response(int server_socket);
void signal_handler(int signum);
void *heartbeat(void *arg);

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server_ip> <server_port>\n", argv[0]);
        exit(1);
    }

    char *server_ip = argv[1];
    int server_port = atoi(argv[2]);

    // Register signal handler for SIGINT
    signal(SIGINT, signal_handler);

    // Check if the results directory exists, if not, create it
    struct stat st = {0};
    if (stat("results", &st) == -1) {
        mkdir("results", 0700);
    }


    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("ERROR opening socket");
        exit(1);
    }

    // Set a timeout for socket read operations
    struct timeval timeout;
    timeout.tv_sec = 10; // Timeout after 10 seconds
    timeout.tv_usec = 0;
    if (setsockopt(server_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
        perror("ERROR setting socket timeout");
        exit(1);
    }

    // Set up server address structure
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("ERROR converting server IP address");
        exit(1);
    }

    // Connect to server
    if (connect(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("ERROR connecting to server");
        exit(1);
    }

    printf("Connected to server\n");

    // Start the heartbeat thread
    pthread_t hb_thread;
    if (pthread_create(&hb_thread, NULL, heartbeat, NULL) != 0) {
        perror("ERROR creating heartbeat thread");
        exit(1);
    }

    // Main loop for client-server interaction
    while (running) {
        char command[BUFFER_SIZE];

        // Get user input for server command
        printf("Enter a command for the server: ");
        if (fgets(command, sizeof(command), stdin) == NULL) {
            printf("Input error.\n");
            continue;
        }

        // Send command to server
        if (write(server_socket, command, strlen(command)) < 0) {
            perror("ERROR writing to socket");
            break;
        }

        // Handle server response
        handle_server_response(server_socket);
    }

    // Close the socket
    close(server_socket);

    // Wait for the heartbeat thread to finish
    pthread_join(hb_thread, NULL);

    return 0;
}


void *heartbeat(void *arg) {

    (void)arg;

    while (running) {
        // Send heartbeat message to server
        char *message = "heartbeat\n";
        if (write(server_socket, message, strlen(message)) < 0) {
            perror("ERROR writing to socket");
            running = 0; // Indicate that the server is down
            break;
        }

        // Wait for server response
        char buffer[BUFFER_SIZE];
        ssize_t n = read(server_socket, buffer, sizeof(buffer) - 1);
        if (n <= 0) {
            // Server is not responding
            printf("Server is down. Exiting client.\n");
            running = 0; // Indicate that the server is down
            break;
        }

        // Wait before sending the next heartbeat message
        sleep(5);
    }

    // Close the socket if the heartbeat fails
    if (!running && server_socket >= 0) {
        close(server_socket);
    }

    return NULL;
}

void handle_server_response(int server_socket) {
    char buffer[BUFFER_SIZE];
    int n;
    char *end_of_message = "END_OF_MESSAGE\n";
    char response[BUFFER_SIZE * 10] = {0};
    char *response_ptr = response;

    // Read server response until the end-of-message indicator is found
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        n = read(server_socket, buffer, BUFFER_SIZE - 1);

        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // Read operation timed out
                printf("Read operation timed out. Server may be down.\n");
                break;
            } else {
                // Other errors
                perror("ERROR reading from socket");
                break;
            }
        } else if (n == 0) {
            // Server closed the connection
            printf("Server closed the connection\n");
            break;
        }

        // Check if the end-of-message indicator is part of the response
        if (strstr(buffer, end_of_message) != NULL) {
            // Find the position of the indicator
            char *end_position = strstr(buffer, end_of_message);
            // Calculate the length of the message up to the indicator
            size_t message_length = end_position - buffer;
            // Copy the part of the buffer before the indicator
            memcpy(response_ptr, buffer, message_length);
            response_ptr += message_length;
            break;
        } else {
            // Copy the received buffer to the response buffer
            memcpy(response_ptr, buffer, n);
            response_ptr += n; // Move the pointer forward
        }
    }

    // Null-terminate the response
    *response_ptr = '\0';

    // Print the complete server response to the terminal
    printf("Server response: %s\n", response);

    // Generate a unique filename using the current timestamp
    char filename[50]; // Increased buffer size to accommodate directory path
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    strftime(filename, 50, "results/server_response_%Y%m%d%H%M%S.txt", tm_info); // Prepend directory to filename

    // Write the complete server response to the new file in the results directory
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        perror("ERROR opening file");
        return;
    }
    fprintf(file, "%s", response);
    fclose(file);

    printf("Response written to file: %s\n", filename);
}


void signal_handler(int signum) {
    // Handle user interrupt
    if (signum == SIGINT) {
        printf("\nUser interrupted the program. Closing socket and exiting.\n");
        if (server_socket >= 0) {
            close(server_socket);
        }
        exit(0);
    }
}




