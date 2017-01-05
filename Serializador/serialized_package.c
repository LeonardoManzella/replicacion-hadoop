#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "serialized_package.h"

char* package_serialize(t_package* package, uint32_t payload_size);


char* package_create(char* payload, uint32_t payload_size, char* order, t_sender_id sender_id) {

	//obtengo la memoria para el paquete y el header
	t_package* package = malloc(sizeof(t_package));

	if(package == NULL) {
		perror("Error en malloc.");
		return NULL;
	}

	t_header* header = malloc(sizeof(t_header));

	if(header == NULL) {
		perror("Error en malloc.");
		return NULL;
	}

	//Armo el header

	//seteo el size del payload
	header->payload_size = payload_size;

	uint32_t package_size = HEADER_SIZE + header->payload_size;

	strcpy(header->order, order);

	header->sender_id = sender_id;

	//seteo header y payload
	package->package_header = header;

	package->package_payload = payload;

	// FIXME: bytes inside a block of size
	char* serialized_package = malloc(package_size);

	if(serialized_package == NULL) {
		perror("Error en malloc.");
		return NULL;
	}

	char* package_serialize_return = package_serialize(package, payload_size);
	memcpy(serialized_package, package_serialize_return, package_size);
	free(package_serialize_return);
	//memcpy(serialized_package, package_serialize(package, payload_size), package_size);

	free(header);

	free(package);

	//finalmente devuelvo el paquete creado serializado
	return serialized_package;

}

char* package_serialize(t_package* package, uint32_t payload_size) {

	//creo un buffer para "serializar"
	char* buffer = malloc(package->package_header->payload_size + HEADER_SIZE);

	char* offset_buffer;

	if(buffer == NULL) {
		perror("Error en malloc.");
		return NULL;
	}

	//paso los elementos del paquete al buffer, mempcpy me devuelve un puntero al byte inmediato de donde copié anteriormente.
	offset_buffer = mempcpy(buffer, &package->package_header->payload_size, sizeof(package->package_header->payload_size));
	offset_buffer = mempcpy(offset_buffer, &package->package_header->sender_id, sizeof(package->package_header->sender_id));
	offset_buffer = mempcpy(offset_buffer, package->package_header->order, sizeof(package->package_header->order));
	offset_buffer = mempcpy(offset_buffer, package->package_payload, payload_size);

	return buffer;

}

void package_destroy(char* serialized_package) {

	free(serialized_package);

}

/* NO SE USA EN NINGUN LADO
unsigned long package_get_size(char* serialized_package) {
	return (unsigned long) sizeof(serialized_package);

}
*/

t_header header_create(char* header_data) {

	t_header header;

	strcpy( header.order, ((t_header*) header_data) -> order );

	header.sender_id = ((t_header*) header_data) -> sender_id;

	//Payload Size faltaba cargar! Me Daba Cualquier Cosa
	header.payload_size = ((t_header*) header_data) -> payload_size;

	return header;

}

bool header_esOrden(t_header header, const char* ordenPorComparar){

	//Me fijo si son iguales los strings
	if (strcmp(header.order, ordenPorComparar) == 0) {
		return true;
	} else {
		return false;
	}

}

uint32_t get_payload_size(t_header* header) {
	return header->payload_size;
}

char* reservar_memoria_payload(uint32_t payload_size) {

	char* memoria_payload = malloc(payload_size * sizeof(char));

	if(memoria_payload == NULL) {
		perror("Error en malloc.");
		return NULL;
	}

	return memoria_payload;

}

void liberar_memoria_payload(char* payload) {

	free(payload);

}
