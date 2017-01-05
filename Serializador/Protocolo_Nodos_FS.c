#include <stdlib.h>
#include <string.h>

#include "Protocolo_Nodos_FS.h"


char* serializar_datos_archivo_final(t_datos_archivo_final datos_archivo_final, uint32_t *tamanioSerializacion){

	//creo un buffer para serializar
	char* buffer = malloc(sizeof(t_datos_archivo_final));

	//creo un puntero para guardar la referencia de la siguiente posición libre del buffer
	char* offset_buffer;

	//copio datos al buffer
	offset_buffer = mempcpy(buffer, &datos_archivo_final.tamanio, sizeof(datos_archivo_final.tamanio));
	offset_buffer = mempcpy(offset_buffer, &datos_archivo_final.cantidad_bloques, sizeof(datos_archivo_final.cantidad_bloques));
	offset_buffer = mempcpy(offset_buffer, datos_archivo_final.md5, sizeof(datos_archivo_final.md5));

	*tamanioSerializacion = sizeof(t_datos_archivo_final);

	//devuelvo el buffer
	return buffer;
}

t_datos_archivo_final deserializar_datos_archivo_final(const char* datos_archivo_final_serializados){

	//creo la variable resultado
	t_datos_archivo_final datos_archivo_final;

	//casteo los datos serializados a un puntero para realizar la copia al resultado
	t_datos_archivo_final* datos_serializados = ((t_datos_archivo_final*) datos_archivo_final_serializados);

	//copio los campos...
	datos_archivo_final.tamanio = datos_serializados->tamanio;

	datos_archivo_final.cantidad_bloques = datos_serializados->cantidad_bloques;

	strcpy(datos_archivo_final.md5, datos_serializados->md5);

	//devuelvo el resultado
	return datos_archivo_final;
}


void datos_archivo_final_liberar_recursos(char* datos_archivo_final_serializados) {

	free(datos_archivo_final_serializados);
}



char* serializar_datos_nodo(t_datos_nodo datos_nodo) {

	//creo un buffer para serializar
	char* buffer = malloc(sizeof(t_datos_nodo));

	//creo un puntero para guardar la referencia de la siguiente posición libre del buffer
	char* offset_buffer;

	//copio datos al buffer
	offset_buffer = mempcpy(buffer, datos_nodo.nombre_nodo, sizeof(datos_nodo.nombre_nodo));
	offset_buffer = mempcpy(offset_buffer, datos_nodo.ip, sizeof(datos_nodo.ip));
	offset_buffer = mempcpy(offset_buffer, &datos_nodo.puerto, sizeof(datos_nodo.puerto));
	offset_buffer = mempcpy(offset_buffer, &datos_nodo.numero_bloques, sizeof(datos_nodo.numero_bloques));

	//devuelvo el buffer
	return buffer;
}

t_datos_nodo deserializar_datos_nodo(const char* datos_nodo_serializados) {

	//creo la variable resultado
	t_datos_nodo datos_nodo;

	//casteo los datos serializados a un puntero para realizar la copia al resultado
	t_datos_nodo* datos_serializados = ((t_datos_nodo*) datos_nodo_serializados);

	//copio los campos...
	strcpy(datos_nodo.nombre_nodo, datos_serializados->nombre_nodo);

	strcpy(datos_nodo.ip, datos_serializados->ip);

	datos_nodo.puerto = datos_serializados->puerto;

	datos_nodo.numero_bloques = datos_serializados->numero_bloques;

	//devuelvo el resultado
	return datos_nodo;
}

void datos_nodo_liberar_recursos(char* datos_nodo_serializados) {
	free(datos_nodo_serializados);
}

char* serializar_orden_copiar(t_datos_orden_copiar_bloque datos, uint32_t* tamanioSerializacion){
	//creo un buffer para serializar
	char* buffer = malloc(sizeof(t_datos_orden_copiar_bloque));

	//creo un puntero para guardar la referencia de la siguiente posición libre del buffer
	char* offset_buffer;

	//copio datos al buffer
	offset_buffer = mempcpy(buffer, &datos.nodoOrigenNumeroBloque , sizeof(datos.nodoOrigenNumeroBloque) );
	offset_buffer = mempcpy(offset_buffer, datos.nodoDestinoIP , sizeof(datos.nodoDestinoIP) );
	offset_buffer = mempcpy(offset_buffer, &datos.nodoDestinoPuerto , sizeof(datos.nodoDestinoPuerto) );
	offset_buffer = mempcpy(offset_buffer, &datos.nodoDestinoNumeroBloque , sizeof(datos.nodoDestinoNumeroBloque) );
	offset_buffer = mempcpy(offset_buffer, &datos.tamanioBloque , sizeof(datos.tamanioBloque) );

	//devuelvo el buffer y el tamanio por referencia
	*tamanioSerializacion = sizeof(t_datos_orden_copiar_bloque);
	return buffer;
}

t_datos_orden_copiar_bloque deserializar_orden_copiar(const char* datosSerializados){
	//Creo la variable resultado
	t_datos_orden_copiar_bloque resultadoDesSerializado;

	//Casteo los datos serializados a un puntero para realizar la copia al resultado
	t_datos_orden_copiar_bloque* punteroDatosSerializados = (t_datos_orden_copiar_bloque*) datosSerializados;

	//Copio los campos...
	resultadoDesSerializado.nodoOrigenNumeroBloque = punteroDatosSerializados->nodoOrigenNumeroBloque;
	strcpy( resultadoDesSerializado.nodoDestinoIP,  punteroDatosSerializados->nodoDestinoIP);
	resultadoDesSerializado.nodoDestinoPuerto = punteroDatosSerializados->nodoDestinoPuerto;
	resultadoDesSerializado.nodoDestinoNumeroBloque =  punteroDatosSerializados->nodoDestinoNumeroBloque;
	resultadoDesSerializado.tamanioBloque =  punteroDatosSerializados->tamanioBloque;

	//Devuelvo el resultado
	return resultadoDesSerializado;
}

void orden_copiar_liberar_recursos(char* datosSerializados){
	free(datosSerializados);
}

// Serializador para la orden pedirBloqueMD5 en el nodo
char* serializarNodoPedirBloqueMD5(char* md5Bloque, uint32_t* tamanioSerializacion) {
	//creo un buffer para serializar
	char* buffer = malloc(MD5_LENGTH);

	//copio datos al buffer
	mempcpy(buffer, &md5Bloque, MD5_LENGTH);

	*tamanioSerializacion = MD5_LENGTH;

	//devuelvo el buffer
	return buffer;
}

// Deserializador de la orden pedirBloqueMD5 en el nodo
t_datos_orden_pedir_md5_bloque deserializarNodoPedirBloqueMD5(const char* ordenSerializada) {


	//creo la variable resultado
	t_datos_orden_pedir_md5_bloque datos_orden_bloque;

	//casteo los datos serializados a un puntero para realizar la copia al resultado
	t_datos_orden_pedir_md5_bloque* datos_serializados = ((t_datos_orden_pedir_md5_bloque*) ordenSerializada);

	//copio los campos...
	datos_orden_bloque.nodoDestinoNumeroBloque = datos_serializados->nodoDestinoNumeroBloque;

	datos_orden_bloque.tamanioBloque = datos_serializados->tamanioBloque;

	//devuelvo el resultado
	return datos_orden_bloque;

}

// Serializador para la orden pedirBloqueMD5 en FS
char* serializarFSPedirBloqueMD5(const t_datos_orden_pedir_md5_bloque datosBloque, uint32_t* tamanioSerializacion) {
	//creo un buffer para serializar
	char* buffer = malloc(sizeof(uint32_t));
	char* offset_buffer;

	//copio datos al buffer
	offset_buffer = mempcpy(buffer, &datosBloque.nodoDestinoNumeroBloque, sizeof(datosBloque.nodoDestinoNumeroBloque));
	offset_buffer = mempcpy(offset_buffer, &datosBloque.tamanioBloque, sizeof(datosBloque.tamanioBloque));
	//copio datos al buffer

	*tamanioSerializacion = sizeof(t_datos_orden_pedir_md5_bloque);

	//devuelvo el buffer
	return buffer;
}

// Deserializador de la orden pedirBloqueMD5 en FS
//char* deserializarFSPedirBloqueMD5(const char* bloqueMD5Serializado) {
//
//	char* bloqueMD5 = NULL;
//
//	bloqueMD5 = bloqueMD5Serializado;
//
//	return bloqueMD5;
//
//}

char* serializar_orden_leer_bloque(t_datos_orden_leer_bloque datos_orden_leer_bloque, uint32_t *tamanioSerializacion) {

	//creo un buffer para serializar
	char* buffer = malloc(sizeof(t_datos_orden_leer_bloque));

	//creo un puntero para guardar la referencia de la siguiente posición libre del buffer
	char* offset_buffer;

	//copio datos al buffer
	offset_buffer = mempcpy(buffer, &datos_orden_leer_bloque.nodoDestinoNumeroBloque, sizeof(datos_orden_leer_bloque.nodoDestinoNumeroBloque));
	offset_buffer = mempcpy(offset_buffer, &datos_orden_leer_bloque.tamanioBloque, sizeof(datos_orden_leer_bloque.tamanioBloque));

	*tamanioSerializacion = sizeof(t_datos_orden_copiar_bloque);

	//devuelvo el buffer
	return buffer;
}

t_datos_orden_leer_bloque deserializar_orden_leer_bloque(const char* datos_orden_leer_bloque_serializados) {

	//creo la variable resultado
	t_datos_orden_leer_bloque datos_orden_leer_bloque;

	//casteo los datos serializados a un puntero para realizar la copia al resultado
	t_datos_orden_leer_bloque* datos_serializados = ((t_datos_orden_leer_bloque*) datos_orden_leer_bloque_serializados);

	//copio los campos...
	datos_orden_leer_bloque.nodoDestinoNumeroBloque = datos_serializados->nodoDestinoNumeroBloque;

	datos_orden_leer_bloque.tamanioBloque = datos_serializados->tamanioBloque;

	//devuelvo el resultado
	return datos_orden_leer_bloque;
}

void datos_orden_leer_bloque_liberar_recursos(char* datos_orden_leer_bloque_serializados) {
	free(datos_orden_leer_bloque_serializados);
	return;
}
