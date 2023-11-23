CC = gcc
CFLAGS = -Wall -Wextra -Werror

all: server client

server: mini_serv.c
	$(CC) $(CFLAGS) -o server mini_serv.c

client: client.c
	$(CC) $(CFLAGS) -o client client.c

clean:
	rm -f server client

re: clean all
