#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>

void errorStr(char* str) {
	if (str)
		write(2, str, strlen(str));
	else
		write(2, "Fatal error\n", 12);
	exit(1);
}

void broadcast(int* clients, int s, const char* m, int c_count) {
	char br_m[4096];
	sprintf(br_m, "client %d: %s\n", s, m);

	for (int i = 0; i < c_count; i++) {
		if (clients[i] != -1 && i != s)
			send(clients[i], br_m, sizeof(br_m), 0);
	}
}

int main(int argc, char** argv) {
	if (argc != 2)
		errorStr("Wrong number of argument\n");

	int c_count = 10;
	int port = atoi(argv[1]);
	char buff[4096];

	int s_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (s_sock == -1)
		errorStr(NULL);
	struct sockaddr_in s_addr, c_addr;
	socklen_t c_addr_len = sizeof(c_addr);
	memset(&s_addr, 0, sizeof(s_addr));
	s_addr.sin_family = AF_INET;
	s_addr.sin_port = htons(port);
	s_addr.sin_addr.s_addr = INADDR_ANY;

	int clients[c_count];
	for (int i = 0; i < c_count; i++)
		clients[i] = -1;
	
	fd_set read_fds;
	int max_fd;

	if (bind(s_sock, (struct sockaddr*)&s_addr, sizeof(s_addr)) == -1
		|| listen(s_sock, c_count) == -1) {
		close(s_sock);
		errorStr(NULL);
	}

	while(1) {
		FD_ZERO(&read_fds);
		FD_SET(s_sock, &read_fds);
		max_fd = s_sock;

		for (int i = 0; i < c_count; i++) {
			if (clients[i] != -1) {
				FD_SET(clients[i], &read_fds);
				if (clients[i] > max_fd)
					max_fd = clients[i];
			}
		}
		if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) == -1) {
			close(s_sock);
			errorStr(NULL);
		}
		if (FD_ISSET(s_sock, &read_fds)) {
			int new_c = accept(s_sock, (struct sockaddr *)& c_addr, &c_addr_len);
			if (new_c == -1) {
				close(s_sock);
				errorStr(NULL);
			}
			for (int i = 0; i < c_count; i++) {
				if (clients[i] == -1) {
					clients[i] = new_c;
					printf("server: client %d just arrived\n", i);
					break;
				}
			}
		}
		for (int i = 0; i < c_count; i++) {
			if (clients[i] != -1 && FD_ISSET(clients[i], &read_fds)) {
				memset(buff, 0, sizeof(buff));
				ssize_t b_r = recv(clients[i], buff, sizeof(buff), 0);
				if (b_r <= 0) {
					printf("server: client %d just left\n", i);
					close(clients[i]);
					clients[i] = -1;
				} else {
					broadcast(clients, i, buff, c_count);
				}
			}
		}
	}
	close(s_sock);
	return 0;
}
