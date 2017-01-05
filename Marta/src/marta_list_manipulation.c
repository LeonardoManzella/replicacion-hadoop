#include "../headers/marta_list_manipulation.h"

#include <commons/log.h>
#include <commons/string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

int getNodePositionFromList(t_list *lista, tipo_nodo_marta *nodo) {
	loguear(LOG_LEVEL_TRACE, __func__, "Buscando al Nodo: %s", nodo->nodoNombre);

	int returnValue = -1;

	int listSize = list_size(lista);
	int indexEnLista = 0;

	for (indexEnLista = 0; indexEnLista < listSize; ++indexEnLista) {
		tipo_nodo_marta *nodoActual = list_get(lista, indexEnLista);

		bool same_Name = false;
		bool same_ip = false;
		bool same_port = false;

		if (string_equals_ignore_case(nodo->nodoNombre, nodoActual->nodoNombre)) {
			same_Name = true;
		}

		if (string_equals_ignore_case(nodo->nodoIP, nodoActual->nodoIP)) {
			same_ip = true;
		}

		if (string_equals_ignore_case(nodo->nodoPuerto, nodoActual->nodoPuerto)) {
			same_port = true;
		}

		if (same_Name && same_ip && same_port) {
			returnValue = indexEnLista;
			loguear(LOG_LEVEL_TRACE, __func__, "Encontre al Nodo!");
			break;
		}
	}

	if (returnValue == -1) {
		loguear(LOG_LEVEL_TRACE, __func__, "No Encontre al Nodo ");
	}

	return returnValue;
}

tipo_nodo_marta* findListNodeFromRequestNode(t_list **lista, tipo_nodo_marta *nodo) {

	int positionInList = getNodePositionFromList(*lista, nodo);

	tipo_nodo_marta *returnNode;

	if (positionInList > -1) {
		loguear(LOG_LEVEL_TRACE, __func__, "Se encontro al Nodo Buscado, devolviendo sus Datos");
		returnNode = (tipo_nodo_marta *) list_get(*lista, positionInList);

	} else {
		loguear(LOG_LEVEL_TRACE, __func__, "No Existe el Nodo, se Agrega a la Lista");

		returnNode = malloc(sizeof(tipo_nodo_marta));

		memcpy(returnNode->nodoNombre, nodo->nodoNombre, strlen(nodo->nodoNombre) + 1);
		memcpy(returnNode->nodoIP, nodo->nodoIP, strlen(nodo->nodoIP) + 1);
		memcpy(returnNode->nodoPuerto, nodo->nodoPuerto, strlen(nodo->nodoPuerto) + 1);

		returnNode->nodoTrabajos = 0;
		returnNode->status = NEW;
		returnNode->archivosAProcesar = NULL;
		//FIXME Esto es NULL, estas seguro que esta bien? Que almenos no deberia ser un puntero a un list create?

		int added = list_add(*lista, returnNode);
		loguear(LOG_LEVEL_TRACE, __func__, "agrego %d", added);
	}

	return returnNode;
}

int trabajosActuales(t_list **lista, tipo_nodo_marta *nodo) {
	tipo_nodo_marta* currentNode = findListNodeFromRequestNode(lista, nodo);

	if (nodo->status == NEW) {
		loguear(LOG_LEVEL_TRACE, __func__, "Hay un Nuevo Nodo %s, me fijo si ya estaba para Re-Activarlo",
				nodo->nodoNombre);

		if (currentNode->status == ERROR) {
			loguear(LOG_LEVEL_TRACE, __func__, "Se Reactivo el Nodo");
			currentNode->status = READY;
			currentNode->nodoTrabajos = 0;
		}
	}

	return currentNode->nodoTrabajos;
}

int comenzarTrabajo(t_list **lista, tipo_nodo_marta *nodo) {
	loguear(LOG_LEVEL_TRACE, __func__, "Comenzando Trabajo en Nodo: %s", nodo->nodoNombre);

	int positionInList = getNodePositionFromList(*lista, nodo);

	tipo_nodo_marta* returnNode;

	if (positionInList > -1) {
		returnNode = (tipo_nodo_marta *) list_get(*lista, positionInList);

		if (returnNode->nodoTrabajos >= 0) {
			returnNode->nodoTrabajos = returnNode->nodoTrabajos + 1;
		}
	} else {
		loguear(LOG_LEVEL_ERROR, __func__, "No se pudo Comenzar el Trabajo porque No se encuentra al Nodo");
		return -1;

	}

	return returnNode->nodoTrabajos;
}

int terminarTrabajo(t_list **lista, tipo_nodo_marta *nodo) {
	loguear(LOG_LEVEL_TRACE, __func__, "Terminando Trabajo en Nodo: %s", nodo->nodoNombre);

	int positionInList = getNodePositionFromList(*lista, nodo);

	tipo_nodo_marta* returnNode;

	if (positionInList > -1) {
		returnNode = (tipo_nodo_marta *) list_get(*lista, positionInList);

		if (returnNode->nodoTrabajos >= 0) {
			returnNode->nodoTrabajos = returnNode->nodoTrabajos - 1;
			if (returnNode->nodoTrabajos < 0) {
				returnNode->nodoTrabajos = 0;
			}
		}
	} else {
		loguear(LOG_LEVEL_ERROR, __func__, "No se pudo Terminar el Trabajo porque No se encuentra al Nodo");
		return -1;
	}

	return returnNode->nodoTrabajos;
}

void desactivarNodo(t_list **lista, tipo_nodo_marta *nodo) {
	tipo_nodo_marta* currentNode = findListNodeFromRequestNode(lista, nodo);

	loguear(LOG_LEVEL_TRACE, __func__, "El Nodo %s se Desactivo", nodo->nodoNombre);
	currentNode->status = ERROR;
	currentNode->nodoTrabajos = -2;
}
