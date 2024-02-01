// To test the server use, in a separate terminal, the following command:
// nc 127.0.0.1 9000
// or the client

#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>

// Structure to store client data
typedef struct s_clients {
	int		fd;
	int		id;
} t_clients;

// Broadcast, message, types
enum { NEW, LEFT, WRITE };
// File descriptors sets.
// The read set is used to check for activity on the server socket and client sockets.
// The write set is used to check if a client is ready to receive a message.
fd_set read_fds, write_fds;
// Number of clients and last client id
int client_count = 10;
int last_id = 0;


/**
 * @brief Prints an error message to the standard error stream and exits the program.
 * 
 * @param msg The error message to be printed.
 */
void errorStr(char* msg) {
	if (msg) {
		write(2, msg, strlen(msg));
	} else {
		write(2, "Fatal error", 11);
	}
	write(2, "\n", 1);
	exit(1);
}

/**
 * Broadcasts a message to all connected clients, excluding the sender.
 *
 * @param clients The array of connected clients.
 * @param sender The index of the sender in the clients array.
 * @param message The message to be broadcasted.
 * @param type The type of the message (0-NEW: client arrival, 1-LEFT: client close connection, 2-WRITE: regular message).
 */
void broadcast(t_clients* clients, int sender, const char* message, int type) {
	// The message to be broadcasted
	char broadcast_message[200000];
	memset(broadcast_message, 0, sizeof(broadcast_message));

	// The format of the message to be broadcasted
	char* broadcast_formats[] = {
		"server: client %d just arrived\n",
		"server: client %d just left\n",
		"client %d: %s"
	};
	sprintf(broadcast_message, broadcast_formats[type], clients[sender].id, message);

	// Broadcast the message to all connected clients, if they are ready to receive, excluding the sender
	for (int i = 0; i < client_count; i++) {
		if (clients[i].fd != -1 && i != sender && FD_ISSET(clients[i].fd, &write_fds)) {
			send(clients[i].fd, broadcast_message, strlen(broadcast_message), 0);
		}
	}
}

/**
 * @file mini_serv.c
 * @brief A simple server program that listens for incoming connections and broadcasts messages to connected clients.
 *
 * This program creates a server socket, binds it to a specified port, and listens for incoming connections.
 * It uses the select() function to handle multiple client connections simultaneously.
 * When a new client connects, it assigns a unique ID to the client and broadcasts the connection to all other clients.
 * It also broadcasts any messages received from clients to all other connected clients.
 * If a client disconnects, it broadcasts the disconnection to all other clients and closes the client socket.
 *
 * @param argc The number of command-line arguments.
 * @param argv An array of command-line arguments.
 * @return 0 on success, 1 on error.
 */
int main(int argc, char** argv) {
	// Check the number of command-line arguments
	if (argc != 2)
		errorStr("Wrong number of arguments");

	// Get the port number from the command-line arguments
	int port = atoi(argv[1]);
	char buffer[200000];

	// Create the server socket
	int server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket == -1)
		errorStr(NULL);

	// Set the server socket to non-blocking
	struct sockaddr_in server_address, client_address;
	socklen_t client_address_len = sizeof(client_address);
	memset(&server_address, 0, sizeof(server_address));
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(port);
	server_address.sin_addr.s_addr = htonl(2130706433); //127.0.0.1

	// Bind the server socket to the specified port
	if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
		close(server_socket);
		errorStr(NULL);
	}

	// Listen for incoming connections
	if (listen(server_socket, client_count) == -1) {
		close(server_socket);
		errorStr(NULL);
	}

	// Initialize the array of connected clients
	t_clients clients[client_count];
	memset(clients, -1, sizeof(clients));
	int max_fd;

	while (1) {
		// Initialize the file descriptors sets and the maximum file descriptor
		FD_ZERO(&read_fds);
		FD_SET(server_socket, &read_fds);
		max_fd = server_socket;

		// Add all connected clients to the file descriptors sets
		// and update the maximum file descriptor
		for (int i = 0; i < client_count; i++) {
			if (clients[i].fd != -1) {
				FD_SET(clients[i].fd, &read_fds);
				if (clients[i].fd > max_fd)
					max_fd = clients[i].fd;
			}
		}

		// Update the write file descriptor. Both will be modified by select()
		write_fds = read_fds;

		// Wait for activity on any of the file descriptors
		if (select(max_fd + 1, &read_fds, &write_fds, NULL, NULL) == -1)
			continue;

		// If there is activity on the server socket, accept the new connection
		if (FD_ISSET(server_socket, &read_fds)) {
			// Accept the new connection
			int new_client = accept(server_socket, (struct sockaddr *)&client_address, &client_address_len);
			if (new_client == -1)
				continue;
			// Search for an empty slot in the array of connected clients,
			// add the new client and broadcast the connection
			for (int i = 0; i < client_count; i++) {
				if (clients[i].fd == -1) {
					clients[i].fd = new_client;
					clients[i].id = last_id++;
					broadcast(clients, i, NULL, NEW);
					break;
				}
			}
			continue;
		}

		// Check all connected clients for activity
		for (int i = 0; i < client_count; i++) {
			// If there is activity on a client socket, receive the message
			if (clients[i].fd != -1 && FD_ISSET(clients[i].fd, &read_fds)) {
				// Read the message 1 byte at a time until a newline is received or the message is fully received
				memset(buffer, 0, sizeof(buffer));
				ssize_t bytes_received = 1;
				while (bytes_received == 1 && buffer[strlen(buffer) - 1] != '\n')
					bytes_received = recv(clients[i].fd, buffer + strlen(buffer), 1, 0);

				// If the client disconnected, broadcast the disconnection and close the client socket
				if (bytes_received <= 0) {
					broadcast(clients, i, NULL, LEFT);
					close(clients[i].fd);
					clients[i].fd = -1;
				} else {
					// Otherwise, broadcast the message
					broadcast(clients, i, buffer, WRITE);
				}
			}
		}
	}
	return 0;
}
