/*
 * main_server.c
 *
 *  Created on: May 18, 2015
 *      Author: vagan
 */

#include "../tests/Biblioteca_Sockets.h"
#include "serialized_package.h"

#define PORT_LISTENER "3000"

int main() {

	int numeroError;

	char* message = "Es el primer mensaje de prueba!!";
	char* order = "orderDeEjemplo";

	char* serialized_package = package_create(message, order, NODO);

	tipo_socket* socketParaEscuchar = Sockets_ponerme_escuchar(PORT_LISTENER);
	if (socketParaEscuchar==NULL) {
		return NULL;
	}
	printf("%s [::Server::] El servidor esta escuchando en el puerto %s", temporal_get_string_time(), PORT_LISTENER);

	while(1) {
		tipo_socket* socketConectadoAlCliente = Sockets_aceptar_cliente(socketParaEscuchar);
		if (socketConectadoAlCliente==NULL) {
			return NULL;
		}
		printf("%s [::Server::] Se conecto un cliente nuevo, el socket generado : '%d'",
				temporal_get_string_time(), (int*)socketConectadoAlCliente->tDireccionClienteRemoto->sin_port);

		printf("%s [::Server::] El mensaje a enviar : '%s'", temporal_get_string_time(), message);
		printf("%s [::Server::] La orden a ejecutar : '%s'", temporal_get_string_time(), order);

		//Envio Mensaje
		numeroError = Sockets_enviar_datos(socketConectadoAlCliente, serialized_package);
		if (numeroError <= 0) {
			return NULL;
		}
	}

	package_destroy(serialized_package);

	return 0;
}
