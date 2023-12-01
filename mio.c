#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>

typedef struct s_c {
	int fd;
	int id;
} t_c;

enum { NEW, LEFT, WRITE };

void errorStr(char* msg) {
	if (msg) {
		write(2, msg, strlen(msg));
	} else {
		write(2, "Fatal error\n", 12);
	}
	exit(1);
}

void broadcast(t_c* c, int s, const char* m, int c_c, int t) {
	char br_m[4096];
	if (t == 0)
		sprintf(br_m, "server: client %d just arrived\n", c[s].id);
	else if (t == 1)
		sprintf(br_m, "server: client %d just left\n", c[s].id);
	else if (t == 2)
		sprintf(br_m, "client %d: %s", c[s].id, m);
	for (int i = 0; i < c_c; i++) {
		if (c[i].fd != -1 && i != s)
			send(c[i].fd, br_m, strlen(br_m), 0);
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
	s_add.sin_addr.s_addr = htonl(2130706433);

	int l_id = 0;
	t_c c[c_c];
	for (int i = 0; i < c_c; i++) {
		c[i].fd = -1;
		c[i].id = -1;
	}

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
			if (c[i].fd != -1) {
				FD_SET(c[i].fd, &read_fds);
				if (c[i].fd > max_fd)
					max_fd = c[i].fd;
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
				if (c[i].fd == -1) {
					c[i].fd = new_c;
					c[i].id = l_id;
					l_id++;
					broadcast(c, i, NULL, c_c, 0);
					break;
				}
			}
		}

		for (int i = 0; i < c_c; i++) {
			if (c[i].fd != -1 && FD_ISSET(c[i].fd, &read_fds)) {
				memset(buff, 0, sizeof(buff));
				ssize_t b_r = recv(c[i].fd, buff, sizeof(buff), 0);
				if (b_r <= 0) {
					broadcast(c, i, NULL, c_c, 1);
					close(c[i].fd);
					c[i].fd = -1;
				} else {
					broadcast(c, i, buff, c_c, 2);
				}
			}
		}
	}
	close(s_sock);
	return 0;

}
