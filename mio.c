#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>

int main(int argc, char** argv) {
	if (argc != 2) {
		write(STDERR_FILENO, "Wrong number of arguments\n", 26);
		exit(1);
	}
	int port = atoi(argv[1]);

	int serv_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (serv_socket == -1) {
		write(STDERR_FILENO, "Fatal error\n", 12);
		exit(1);
	}
	
}
