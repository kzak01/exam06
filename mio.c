#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <arpa/inet.h>

typedef struct s_c {
	int fd;
	int id;
} t_c;

enum { NEW, LEFT, WRITE};
fd_set r_fds, w_fds, a_fds;
int c_c = 10;
int l_id = 0;

void errorStr(char* msg) {
	if (msg)
		write(2, msg, strlen(msg));
	else
		write(2, "Fatal error", 11);
	write(2, "\n", 1);
	exit(1);
}

void broadcast(t_c* c, int s, const char* m, int t) {
	char br_msg[4096];
	memset(br_msg, 0, sizeof(br_msg));

	char* br_format[] = {
		"server: client %d just arrived\n",
		"server: client %d just left\n",
		"client %d: %s"
	};
	sprintf(br_msg, br_format[t], c[s].id, m);

	for (int i = 0; i < c_c; i++) {
		if (c[i].fd != -1 && i != s && FD_ISSET(c[i].fd, &w_fds)) {
			send(c[i].fd, br_msg, strlen(br_msg), 0);
		}
	}
}

int main(int ac, char** av) {
	if (ac != 2)
		errorStr("Wrong number of arguments");
	
	int port = atoi(av[1]);
	char buffer[4096];

	int s_s = socket(AF_INET, SOCK_STREAM, 0);
	if (s_s == -1)
		errorStr(NULL);

	struct sockaddr_in s_a, c_a;
	socklen_t c_a_len = sizeof(c_a);
	memset(&s_a, 0, sizeof(s_a));
	s_a.sin_family = AF_INET;
	s_a.sin_port = htons(port);
	s_a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	t_c c[c_c];
	memset(c, -1, sizeof(c));

	if (bind(s_s, (struct sockaddr *)&s_a, sizeof(s_a)) == -1) {
		close(s_s);
		errorStr(NULL);
	}
	if (listen(s_s, c_c) == -1) {
		close(s_s);
		errorStr(NULL);
	}

	int max_fd = s_s;
	FD_ZERO(&a_fds);
	FD_SET(s_s, &a_fds);

	while(1) {
		r_fds = w_fds = a_fds;
		max_fd = s_s;

		for (int i = 0; i < c_c; i++) {
			if (c[i].fd != -1) {
				if (c[i].fd > max_fd)
					max_fd = c[i].fd;
			}
		}

		if (select(max_fd + 1, &r_fds, &w_fds, NULL, NULL) == -1)
			continue;

		if (FD_ISSET(s_s, &r_fds)) {
			int n_c = accept(s_s, (struct sockaddr *)&c_a, &c_a_len);
			if (n_c == -1)
				continue;
			for (int i = 0; i < c_c; i++) {
				if (c[i].fd == -1) {
					c[i].fd = n_c;
					c[i].id = l_id;
					FD_SET(n_c, &a_fds);
					l_id++;
					broadcast(c, i, NULL, NEW);
					break;
				}
			}
			continue;
		}

		for (int i = 0; i < c_c; i++) {
			if (c[i].fd != -1 && FD_ISSET(c[i].fd, &r_fds)) {
				memset(buffer, 0, sizeof(buffer));
				ssize_t b_r = recv(c[i].fd, buffer, sizeof(buffer), 0);

				if (b_r <= 0) {
					broadcast(c, i, NULL, LEFT);
					FD_CLR(c[i].fd, &a_fds);
					close(c[i].fd);
					c[i].fd = -1;
				} else {
					broadcast(c, i, buffer, WRITE);
				}
			}
		}
	}
	close(s_s);
	return 0;
}
