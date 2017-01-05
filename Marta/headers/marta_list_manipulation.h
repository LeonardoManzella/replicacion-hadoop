#ifndef SRC_MARTALISTMANIPULATION_H_
#define SRC_MARTALISTMANIPULATION_H_

#include <commons/collections/list.h>
#include "marta_configuration.h"

//trae los trabajos actuales de un nodo.
int trabajosActuales(t_list **lista, tipo_nodo_marta *nodo);

//aumenta el trabajo de un nodo en 1
int comenzarTrabajo(t_list **lista, tipo_nodo_marta *nodo);

//reduce el trabajo de un nodo en 1, si lo deja menor a 0, lo setea en 0
int terminarTrabajo(t_list **lista, tipo_nodo_marta *nodo);

//busca el nodo y si no lo encuentra, lo crea en la lista
tipo_nodo_marta* findListNodeFromRequestNode(t_list **lista, tipo_nodo_marta *nodo);

//trae la posicion de un nodo dentro de una lista
int getNodePositionFromList(t_list *lista, tipo_nodo_marta *nodo);

//desactiva un nodo por un fallo de trabajo
void desactivarNodo(t_list **lista, tipo_nodo_marta *nodo);

#endif /* SRC_MARTALISTMANIPULATION_H_ */
