#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>

#define BUFFER_SIZE 4096

int is_stdin_ready() {
	fd_set read_fds;
	FD_ZERO(&read_fds);
	FD_SET(STDIN_FILENO, &read_fds);

	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;

	return select(STDIN_FILENO + 1, &read_fds, NULL, NULL, &timeout) > 0;
}

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

	if (fcntl(client_socket, F_SETFL, O_NONBLOCK) == -1) {
		perror("Error setting socket to non-blocking");
		close(client_socket);
		exit(1);
	}

	struct sockaddr_in server_address;
	memset(&server_address, 0, sizeof(server_address));
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(server_port);

	if (inet_pton(AF_INET, server_ip, &server_address.sin_addr) <= 0) {
		perror("Error converting server IP address");
		fprintf(stderr, "Invalid IP address: %s\n", server_ip);
		close(client_socket);
		exit(1);
	}

	if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1 && errno != EINPROGRESS) {
		perror("Error connecting to server");
		close(client_socket);
		exit(1);
	}

	char buffer[BUFFER_SIZE];

	while (1) {
		if (is_stdin_ready()) {
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

		memset(buffer, 0, sizeof(buffer));
		ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);

		if (bytes_received > 0) {
			printf("[Server response] %s", buffer);
		}
	}

	close(client_socket);

	return 0;
}
