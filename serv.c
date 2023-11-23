#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 12345
#define BUFFER_SIZE 4096
#define MAX_CLIENTS 5

int main() {
	int server_socket, client_sockets[MAX_CLIENTS];
	struct sockaddr_in server_address, client_address;
	socklen_t client_address_len = sizeof(client_address);
	char buffer[BUFFER_SIZE];

	// Creazione del socket
	if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("Errore durante la creazione del socket");
		exit(EXIT_FAILURE);
	}

	// Configurazione dell'indirizzo del server
	memset(&server_address, 0, sizeof(server_address));
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(PORT);
	server_address.sin_addr.s_addr = INADDR_ANY;

	// Binding del socket
	if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
		perror("Errore durante il binding del socket");
		close(server_socket);
		exit(EXIT_FAILURE);
	}

	// Attivazione della modalità di ascolto
	if (listen(server_socket, MAX_CLIENTS) == -1) {
		perror("Errore durante l'ascolto");
		close(server_socket);
		exit(EXIT_FAILURE);
	}

	printf("Server in ascolto sulla porta %d...\n", PORT);

	int client_count = 0;

	// Loop per accettare connessioni
	while (1) {
		// Accettazione della connessione
		if ((client_sockets[client_count] = accept(server_socket, (struct sockaddr *)&client_address, &client_address_len)) == -1) {
			perror("Errore durante l'accettazione della connessione");
			close(server_socket);
			exit(EXIT_FAILURE);
		}

		printf("Connessione accettata da %s:%d. Client ID: %d\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port), client_count);

		// Creazione di un processo figlio per gestire il client
		if (fork() == 0) {
			close(server_socket); // Il processo figlio non ha bisogno del socket di ascolto

			// Loop per ricevere e stampare i messaggi del client
			while (1) {
				// Ricezione dei dati dal client
				memset(buffer, 0, sizeof(buffer));
				if (recv(client_sockets[client_count], buffer, sizeof(buffer), 0) == -1) {
					perror("Errore durante la ricezione dei dati");
					break;
				}

				// Verifica se il client ha chiuso la connessione
				if (strlen(buffer) == 0) {
					printf("Client ID %d ha chiuso la connessione.\n", client_count);
					break;
				}

				printf("Client ID %d: %s\n", client_count, buffer);
			}

			// Chiusura del socket del client nel processo figlio
			close(client_sockets[client_count]);
			exit(EXIT_SUCCESS);
		} else {
			close(client_sockets[client_count]); // Il processo padre non ha bisogno del socket del client
			client_count++;

			if (client_count >= MAX_CLIENTS) {
				printf("Numero massimo di client raggiunto. Non si accetteranno ulteriori connessioni.\n");
				break;
			}
		}
	}

	// Chiusura del socket del server
	close(server_socket);

	return 0;
}


