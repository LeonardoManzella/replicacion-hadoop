/*
 * main_client.c
 *
 *  Created on: May 18, 2015
 *      Author: vagan
 */

#include "../tests/Biblioteca_Sockets.h"
#include "serialized_package.h"

#define SERVER_IP   "127.0.0.1"
#define SERVER_PORT "3000"

int main() {

	int numeroError;
	char* packageReceived;
	char* header
	t_header* headerFinal;

	printf("%s [::Cliente::] Conectando al servidor en  %s  puerto  %s...\n",
			temporal_get_string_time(), SERVER_IP, SERVER_PORT);

	tipo_socket* socketConectadoAlServidor = Sockets_conectar_servidor(SERVER_IP, SERVER_PORT);
	if (socketConectadoAlServidor==NULL) {
		return NULL;
	}

	printf("%s [::Cliente::] Se establecio la conexion con el servidor: %s:%s",
			temporal_get_string_time(), SERVER_IP, SERVER_PORT);

	char* header = malloc(HEADER_SIZE);

	numeroError = Sockets_recibir_Header(socketConectadoAlServidor, header);
	if (numeroError <= 0) {
		return NULL;
	}

	headerFinal = header_create(header);

	free(header);

	packageReceived = (char*)malloc(headerFinal->payload_size);

	numeroError = Sockets_recibir_Datos(socketConectadoAlServidor, packageReceived, headerFinal->payload_size);
	if (numeroError <= 0) {
		return NULL;
	}

}
