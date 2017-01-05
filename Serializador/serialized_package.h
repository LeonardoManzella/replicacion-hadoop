#ifndef SERIALIZED_PACKAGE_H
#define SERIALIZED_PACKAGE_H


#include <stdbool.h>


#define HEADER_SIZE 43 /* 4 bytes +  4 bytes + 35 bytes */ // revisar que esté bien el tamaño...



/**
*	Enum que define el emisor del mensaje
*/
typedef enum {

	MARTA = 0,
	FILESYSTEM = 1,
	JOB = 2,
	NODO = 3

} t_sender_id;

/**
*	Estructura que define el header del paquete
*/
typedef struct __attribute__ ((__packed__)) {
	
	uint32_t payload_size; //tamaño del paquete completo
	t_sender_id sender_id;  //identificador del emisor del mensaje
	char order[35]; //string que define la orden o acción a realizar. La debe interpretar el receptor del paquete.

} t_header;

/**
*	Estructura que define el paquete completo.
*/
typedef struct __attribute__ ((__packed__)) {

	t_header* package_header; //header del mensaje
	char* package_payload; //contenido del mensaje

} t_package;

/**
*	@NAME: package_create
*	@DESC: Función que crea un paquete, inicializándolo correctamente y devolviendo el mismo serializado.
*	Recibe como parámetros el payload (contenido a enviar), size del payload, la orden a ejecutar, el tipo de mensaje y el id del emisor.
*
*	En caso de error debe devolver NULL
*/
char* package_create(char* payload, uint32_t payload_size, char* order, t_sender_id sender_id);

/**
*	@NAME: package_destroy
*	@DESC: Función que elimina el paquete y libera sus recursos.
*/
void package_destroy(char* serialized_package);

/**
*	@NAME: package_get_size
*	@DESC: Función que devuelve el tamaño completo de un paquete para facilitar su envío.
*/
unsigned long package_get_size(char* serialized_package);

/**
*	@NAME: header_create
*	@DESC: Función que crea un header, inicializándolo correctamente y devolviéndolo.
*	Recibe como parámetro el flujo de datos que lo componen.
*/
t_header header_create(char* header_data);

/**
*	@NAME: header_esOrden
*	@DESC: Función que dado un header se fija si la orden coincide con el string pasado como parámetro.
*/
bool header_esOrden(t_header header, const char* ordenPorComparar);

/**
*	@NAME: header_destroy
*	@DESC: Función que elimina el header y libera sus recursos.
*/
//void header_destroy(t_header* header); --Inecesario, eliminar

/**
 *	@NAME: get_payload_size
 *	@DESC: Función que devuelve el tamaño del payload.
 */
uint32_t get_payload_size(t_header* header);

/**
 *	@NAME: reservar_memoria_payload
 *	@DESC: Función que reserva la memoria necesaria para meter el payload y devuelve un puntero a la misma.
 */
char* reservar_memoria_payload(uint32_t payload_size);

/**
 *	@NAME: liberar_memoria_payload
 *	@DESC: Función que libera la memoria utilizada por el payload.
 */
void liberar_memoria_payload(char* payload);

#endif
//SERIALIZED_PACKAGE_H
