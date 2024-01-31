// telnet 127.0.0.1 8080
// nc 127.0.0.1 8080
// nc -l 8080

#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>

typedef struct s_clients {
	int		fd;
	int		id;
} t_clients;

enum { NEW, LEFT, WRITE };

void errorStr(char* msg) {
	if (msg) {
		write(2, msg, strlen(msg));
	} else {
		write(2, "Fatal error", 12);
	}
	write(2, "\n", 1);
	exit(1);
}

void broadcast(t_clients* clients, int sender, const char* message, int client_count, int type) {
	char broadcast_message[4096];
	memset(broadcast_message, 0, sizeof(broadcast_message));

	if (type == NEW) {
		sprintf(broadcast_message, "server: client %d just arrived\n", clients[sender].id);
	} else if (type == LEFT) {
		sprintf(broadcast_message, "server: client %d just left\n", clients[sender].id);
	} else if (type == WRITE) {
		// if (message && *message && strstr(message, "\n") != message) {
			sprintf(broadcast_message, "client %d: %s", clients[sender].id, message);
		// } else {
		// 	return;
		// }
	}

	for (int i = 0; i < client_count; i++) {
		if (clients[i].fd != -1 && i != sender) {
			send(clients[i].fd, broadcast_message, strlen(broadcast_message), 0);
		}
	}
}

int main(int argc, char** argv) {
	if (argc != 2) {
		errorStr("Wrong number of argument");
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
	server_address.sin_addr.s_addr = htonl(2130706433); //127.0.0.1

	int last_id = 0;
	t_clients clients[client_count];
	for (int i = 0; i < client_count; i++) {
		clients[i].fd = -1;
		clients[i].id = 0;
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
			if (clients[i].fd != -1) {
				FD_SET(clients[i].fd, &read_fds);
				if (clients[i].fd > max_fd) {
					max_fd = clients[i].fd;
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
				if (clients[i].fd == -1) {
					clients[i].fd = new_client;
					clients[i].id = last_id;
					last_id++;
					broadcast(clients, i, NULL, client_count, NEW);
					break;
				}
			}
		}

		for (int i = 0; i < client_count; i++) {
			if (clients[i].fd != -1 && FD_ISSET(clients[i].fd, &read_fds)) {
				memset(buffer, 0, sizeof(buffer));
				ssize_t bytes_received = recv(clients[i].fd, buffer, sizeof(buffer), 0);

				if (bytes_received <= 0) {
					broadcast(clients, i, NULL, client_count, LEFT);
					close(clients[i].fd);
					clients[i].fd = -1;
				} else {
					broadcast(clients, i, buffer, client_count, WRITE);
				}
			}
		}
	}

	close(server_socket);
	return 0;
}
