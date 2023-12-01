// telnet 127.0.0.1 8080

#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>

void errorStr(char* msg) {
	if (msg) {
		write(2, msg, strlen(msg));
	} else {
		write(2, "Fatal error\n", 12);
	}
	exit(1);
}

void broadcast(int* clients, int sender, const char* message, int client_count, int type) {
	char broadcast_message[4096];

	if (type == 0) {
		sprintf(broadcast_message, "server: client %d just arrived\n", sender);
	} else if (type == 1) {
		sprintf(broadcast_message, "server: client %d just left\n", sender);
	} else if (type == 2) {
		sprintf(broadcast_message, "client %d: %s", sender, message);
	}

	for (int i = 0; i < client_count; i++) {
		if (clients[i] != -1 && i != sender) {
			send(clients[i], broadcast_message, strlen(broadcast_message), 0);
		}
	}
}

int main(int argc, char** argv) {
	if (argc != 2) {
		errorStr("Wrong number of argument\n");
	}
	int client_count = 10;
	int port = atoi(argv[1]);
	char buffer[4096];

	int server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket == -1) {
		errorStr(NULL);
	}

	struct sockaddr_in server_address, client_address;
	socklen_t client_address_len = sizeof(client_address);
	memset(&server_address, 0, sizeof(server_address));
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(port);
	server_address.sin_addr.s_addr = INADDR_ANY;

	int clients[client_count];
	for (int i = 0; i < client_count; i++) {
		clients[i] = -1;
	}

	fd_set read_fds;
	int max_fd;

	if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
		close(server_socket);
		errorStr(NULL);
	}

	if (listen(server_socket, client_count) == -1) {
		close(server_socket);
		errorStr(NULL);
	}

	printf("Server listening on port %d...\n", port);

while (1) {
		FD_ZERO(&read_fds);
		FD_SET(server_socket, &read_fds);
		max_fd = server_socket;

		for (int i = 0; i < client_count; i++) {
			if (clients[i] != -1) {
				FD_SET(clients[i], &read_fds);
				if (clients[i] > max_fd) {
					max_fd = clients[i];
				}
			}
		}

		if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) == -1) {
			close(server_socket);
			errorStr(NULL);
		}

		if (FD_ISSET(server_socket, &read_fds)) {
			int new_client = accept(server_socket, (struct sockaddr *)&client_address, &client_address_len);
			if (new_client == -1) {
				close(server_socket);
				errorStr(NULL);
			}
			for (int i = 0; i < client_count; i++) {
				if (clients[i] == -1) {
					clients[i] = new_client;
					broadcast(clients, i, NULL, client_count, 0);
					break;
				}
			}
		}

		for (int i = 0; i < client_count; i++) {
			if (clients[i] != -1 && FD_ISSET(clients[i], &read_fds)) {
				memset(buffer, 0, sizeof(buffer));
				ssize_t bytes_received = recv(clients[i], buffer, sizeof(buffer), 0);

				if (bytes_received <= 0) {
					broadcast(clients, i, NULL, client_count, 1);
					close(clients[i]);
					clients[i] = -1;
				} else {
					broadcast(clients, i, buffer, client_count, 2);
				}
			}
		}
	}

	close(server_socket);
	return 0;
}
