#include "../headers/marta_comunicacion_padre_hijo.h"

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <pthread.h>

int enviarOrdenAlPadre(int ordenTipo, tipo_nodo_marta *nodo, int canalPadre, int canalRetorno, pthread_mutex_t *mutex) {
	int controlDeError = 1;
	controlDeError = pthread_mutex_lock(mutex);
	if (controlDeError != 0) {
		loguear(LOG_LEVEL_ERROR, __func__, "Error al hacer lock al mutex comunicacion padre hijo");
	}

	loguear(LOG_LEVEL_TRACE, __func__, "Envio: %d", ordenTipo);
	int returnValue = -1;

	char *mensaje = serializarOrdenNodoMarta(ordenTipo, nodo);

	loguear(LOG_LEVEL_TRACE, __func__, "Enviando: %d a nodo %s", ordenTipo, nodo->nodoNombre);
#pragma GCC diagnostic ignored "-Wunused-result"
	controlDeError = write(canalPadre, mensaje, TAMANIO_MENSAJE);
	if (controlDeError < 0) {
		loguear(LOG_LEVEL_ERROR, __func__, "Error al hacer write al padre");
	} else {
		controlDeError = read(canalRetorno, &returnValue, sizeof(int32_t));
	}

	if (controlDeError < 0) {
		loguear(LOG_LEVEL_ERROR, __func__, "Error al hacer read al padre");
		returnValue = -1;
	}
#pragma GCC diagnostic pop

	Comun_LiberarMemoria((void**) &mensaje);

	loguear(LOG_LEVEL_TRACE, __func__, "Respondido: %d", returnValue);

	controlDeError = pthread_mutex_unlock(mutex);
	if (controlDeError != 0) {
		loguear(LOG_LEVEL_TRACE, __func__, "mutex error al liberar");
	} else {
		loguear(LOG_LEVEL_TRACE, __func__, "mutex liberado");
	}

	return returnValue;
}

char* serializarOrdenNodoMarta(int ordenTipo, tipo_nodo_marta *nodo) {
	loguear(LOG_LEVEL_TRACE, __func__, "Serializando");
	//reservo la memoria del buffer a enviar(orden + nodo + canal de retorno)

	//reservo la memoria e inicializo en 0 el mensaje completo , por las dudas
	char* mensajeSerializado = malloc(TAMANIO_MENSAJE);

	int offset = 0;

	//agrego orden
	memcpy(mensajeSerializado, &ordenTipo, sizeof(int32_t));
	offset += sizeof(int32_t);

	//agrego nombre de nodo
	memcpy(mensajeSerializado + offset, nodo->nodoNombre, NODO_LONGITUD_NOMBRE);
	offset += NODO_LONGITUD_NOMBRE;

	//agrego puerto de nodo
	memcpy(mensajeSerializado + offset, nodo->nodoPuerto, LONGITUD_CHAR_PUERTOS);
	offset += LONGITUD_CHAR_PUERTOS;

	//agrego ip de nodo
	memcpy(mensajeSerializado + offset, nodo->nodoIP, LONGITUD_CHAR_IP);
	offset += LONGITUD_CHAR_IP;

	//agrego estado de nodo
	memcpy(mensajeSerializado + offset, &nodo->status, sizeof(int32_t));
	offset += sizeof(int32_t);

	loguear(LOG_LEVEL_TRACE, __func__, "Fin Serializacion");
	return mensajeSerializado;
}

void deserializarOrdenNodoMarta(char ** mensaje, int* ordenToReturn, tipo_nodo_marta* nodoToReturn) {
	loguear(LOG_LEVEL_TRACE, __func__, "DesSerializando");
	int offset = 0;

	//parseo orden
	memcpy(&(*ordenToReturn), *mensaje, sizeof(int32_t));
	offset += sizeof(int32_t);

	//parseo nodo
	//parseo nombre de nodo
	memcpy((nodoToReturn->nodoNombre), *mensaje + offset, NODO_LONGITUD_NOMBRE);
	offset += NODO_LONGITUD_NOMBRE;
	nodoToReturn->nodoNombre[NODO_LONGITUD_NOMBRE - 1] = '\0';

	//parseo puerto de nodo
	memcpy((nodoToReturn->nodoPuerto), *mensaje + offset, LONGITUD_CHAR_PUERTOS);
	offset += LONGITUD_CHAR_PUERTOS;
	nodoToReturn->nodoPuerto[LONGITUD_CHAR_PUERTOS - 1] = '\0';

	//parseo puerto de nodo
	memcpy((nodoToReturn->nodoIP), *mensaje + offset, LONGITUD_CHAR_IP);
	offset += LONGITUD_CHAR_IP;
	nodoToReturn->nodoIP[LONGITUD_CHAR_IP - 1] = '\0';

	//parseo puerto de nodo
	memcpy(&(nodoToReturn->status), *mensaje + offset, sizeof(int32_t));
	offset += sizeof(int32_t);

	loguear(LOG_LEVEL_TRACE, __func__, "Fin DesSerializacion");
	return;
}
