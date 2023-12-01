#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>

void errorStr(char* msg) {
	if (msg)
		write(2, msg, strlen(msg));
	else
		write(2, "Fatal error\n", 12);
	exit(1);
}

void broadcast(int* c, int s, const char* m, int c_c, int t) {
	char br_m[4096];

	if (t == 0)
		sprintf(br_m, "server: client %d just arrived\n", s);
	else if (t == 1)
		sprintf(br_m, "server: client %d just left\n", s);
	else if (t == 2)
		sprintf(br_m, "client %d: %s", s, m);

	for (int i = 0; i < c_c; i++) {
		if (c[i] != -1 && i != s)
			send(c[i], br_m, strlen(br_m), 0);
	}
}

int main(int argc, char** argv) {
	if (argc != 2)
		errorStr("Wrong number of argument\n");
	int c_c = 10;
	int port = atoi(argv[1]);
	char buff[4096];

	int s_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (s_sock == -1)
		errorStr(NULL);
	struct sockaddr_in s_add, c_add;
	socklen_t c_add_len = sizeof(c_add);
	memset(&s_add, 0, sizeof(s_add));
	s_add.sin_family = AF_INET;
	s_add.sin_port = htons(port);
	s_add.sin_addr.s_addr = INADDR_ANY;

	int c[c_c];
	for (int i = 0; i < c_c; i++)
		c[i] = -1;

	fd_set read_fds;
	int max_fd;

	if (bind(s_sock, (struct sockaddr *)&s_add, sizeof(s_add)) == -1
		|| listen(s_sock, c_c) == -1) {
		close(s_sock);
		errorStr(NULL);
	}

	while (1) {
		FD_ZERO(&read_fds);
		FD_SET(s_sock, &read_fds);
		max_fd = s_sock;

		for (int i = 0; i < c_c; i++) {
			if (c[i] != -1) {
				FD_SET(c[i], &read_fds);
				if (c[i] > max_fd)
					max_fd = c[i];
			}
		}

		if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) == -1) {
			close(s_sock);
			errorStr(NULL);
		}

		if (FD_ISSET(s_sock, &read_fds)) {
			int new_c = accept(s_sock, (struct sockaddr *)&c_add, &c_add_len);
			if (new_c == -1) {
				close(s_sock);
				errorStr(NULL);
			}
			for (int i = 0; i < c_c; i++) {
				if (c[i] == -1) {
					c[i] = new_c;
					broadcast(c, i, NULL, c_c, 0);
					break;
				}
			}
		}

		for (int i = 0; i < c_c; i++) {
			if (c[i] != -1 && FD_ISSET(c[i], &read_fds)) {
				memset(buff, 0, sizeof(buff));
				ssize_t b_r = recv(c[i], buff, sizeof(buff), 0);
				if (b_r <= 0) {
					broadcast(c, i, NULL, c_c, 1);
					close(c[i]);
					c[i] = -1;
				} else {
					broadcast(c, i, buff, c_c, 2);
				}
			}
		}
	}
	close(s_sock);
	return 0;
}
