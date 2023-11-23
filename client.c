#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 4096

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server_ip> <server_port>\n", argv[0]);
        exit(1);
    }

    const char *server_ip = argv[1];
    int server_port = atoi(argv[2]);

    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("Error creating client socket");
        exit(1);
    }

    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(server_port);

    if (inet_pton(AF_INET, server_ip, &server_address.sin_addr) <= 0) {
        perror("Error converting server IP address");
        close(client_socket);
        exit(1);
    }

    if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
        perror("Error connecting to server");
        close(client_socket);
        exit(1);
    }

    char buffer[BUFFER_SIZE];

    while (1) {
        printf("Enter a message (or 'exit' to quit): ");
        fgets(buffer, sizeof(buffer), stdin);

        if (send(client_socket, buffer, strlen(buffer), 0) == -1) {
            perror("Error sending message");
            break;
        }

        if (strcmp(buffer, "exit\n") == 0) {
            printf("Exiting...\n");
            break;
        }
    }

    close(client_socket);

    return 0;
}
