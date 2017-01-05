#ifndef SRC_MARTACOMUNICACIONPADREHIJO_H_
#define SRC_MARTACOMUNICACIONPADREHIJO_H_

#define MENSAJE_ORDENES_ACTUALES 11
#define MENSAJE_AGREGAR_ORDEN 12
#define MENSAJE_TERMINO_UNA_ORDEN 13
#define MENSAJE_DESACTIVAR_NODO 14

#define TAMANIO_MENSAJE (sizeof(int32_t) + NODO_LONGITUD_NOMBRE + LONGITUD_CHAR_PUERTOS + LONGITUD_CHAR_IP + sizeof(int32_t))

#include "marta_configuration.h"

//Serializa el nodo para hacerle un request a marta padre. Hacer free del retorno de la funcion una vez enviado
char* serializarOrdenNodoMarta(int ordenTipo, tipo_nodo_marta *nodo);

//Deserializa el mensaje de un hijo desde el lado del marta padre
void deserializarOrdenNodoMarta(char ** mensaje, int* ordenToReturn, tipo_nodo_marta* nodoToReturn);

//Envia una orden al padre a travez del canal del padre y queda a la espera de la respuesta en canalDeRetorno.
//Si es positiva, la respuesta esta en el return value, si es negativa, es un error y tambien esta en returnValue
int enviarOrdenAlPadre(int ordenTipo, tipo_nodo_marta *nodo, int canalPadre, int canalRetorno, pthread_mutex_t *mutex);

#endif /* SRC_MARTACOMUNICACIONPADREHIJO_H_ */
