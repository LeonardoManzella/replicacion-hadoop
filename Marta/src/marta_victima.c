#include "../headers/marta_victima.h"

#include "../headers/marta_comunicacion_padre_hijo.h"
#include "../headers/marta_list_manipulation.h"
#include <stdlib.h>

int elegirVictimaYorden(tipo_nodo_marta **victima, int *ordenAMandar, char **resultName, t_list *nodosPorBloque,
		t_list** trabajosPorNodos, bool tieneCombiner, int canalPadre, int canalRetorno, pthread_mutex_t *mutexPadre) {

	//error negativo, la respuesta es negativa hasta que se cambie
	int returnValue = -1;

	*ordenAMandar = SIN_ORDEN;
	loguear(LOG_LEVEL_TRACE, __func__, "orden prevalue %d", *ordenAMandar);

	int indiceListaNodos = 0;
	int sizeListaNodos = list_size(nodosPorBloque);
	int totalReadyForReduce = 0;

	loguear(LOG_LEVEL_TRACE, __func__, "lista nodos tamanio %d", sizeListaNodos);
	for (indiceListaNodos = 0; indiceListaNodos < sizeListaNodos; indiceListaNodos++) {
		t_list* listaPorNumeroDeBloque = list_get(nodosPorBloque, indiceListaNodos);
		loguear(LOG_LEVEL_TRACE, __func__, "lista pedida %d", indiceListaNodos);

		int indicePorNumeroBloque = 0;
		int sizePorNumeroBloque = list_size(listaPorNumeroDeBloque);
		double rantingMasAlto = 0;

		loguear(LOG_LEVEL_TRACE, __func__, "lista pedida tamanio %d", sizeListaNodos);

		//Hay un bloque sin nodos, por ende hay un bloque de un archivo inaccesible
		if (sizePorNumeroBloque == 0) {
			*ordenAMandar = ORDEN_ERROR_BLOQUE_SIN_COPIAS;
			returnValue = indicePorNumeroBloque;
			break;
		}

		bool alMenosUnNodoValido = false;

		for (indicePorNumeroBloque = 0; indicePorNumeroBloque < sizePorNumeroBloque; indicePorNumeroBloque++) {
			tipo_nodo_marta* nodoActual = list_get(listaPorNumeroDeBloque, indicePorNumeroBloque);

			switch (nodoActual->status) {
			case NEW:
			case READY: {
				loguear(LOG_LEVEL_TRACE, __func__, "status switch: READY nodo %s", nodoActual->nodoNombre);

				int posicionEnTrabajosNodo = getNodePositionFromList(*trabajosPorNodos, nodoActual);
				tipo_nodo_marta* nodoEnTrabajos = findListNodeFromRequestNode(trabajosPorNodos, nodoActual);

				//Si la posicion es menor a 0, no estaba en la lista, es la primera vez que lo pido para este job,
				//va con new y si el global estaba en error, lo resetea. Si ya lo habia pedido, va en ready, y no resetea
				if (posicionEnTrabajosNodo > 0 && !tieneCombiner) {
					nodoActual->status = READY;
				}

				//Si el nodo en trabajos esta new, dejo el nodo actual en new, pero el de trabajos pasa a estar ready para
				//el proximo request. Si no, significa que ya lo pedi y pongo el actual en ready
				if (nodoEnTrabajos->status == NEW) {
					nodoEnTrabajos->status = READY;
				} else {
					nodoActual->status = READY;
				}

				int trabajosActuales = enviarOrdenAlPadre(MENSAJE_ORDENES_ACTUALES, nodoActual, canalPadre,
						canalRetorno, mutexPadre);

				if (trabajosActuales < 0) {
					nodoActual->status = ERROR;
					break;
				}

				alMenosUnNodoValido = true;

				double ratingActual = calcularPrioridad(trabajosActuales, tieneCombiner, nodoEnTrabajos->nodoTrabajos);

				if (ratingActual > rantingMasAlto) {
					rantingMasAlto = ratingActual;
					*victima = nodoActual;
					*ordenAMandar = ORDEN_MAP;

					loguear(LOG_LEVEL_TRACE, __func__, "orden a %s", nodoActual->nodoNombre);
					loguear(LOG_LEVEL_TRACE, __func__, "orden a %s", (*victima)->nodoNombre);

					char template[50];
					sprintf(template, "-%d-%d", indiceListaNodos, nodoActual->numeroDeBloqueInterno);

					*resultName = malloc(strlen(template) + 1);
					memcpy(*resultName, template, strlen(template) + 1);
				}

				break;
			}
			case ERROR: {
				loguear(LOG_LEVEL_TRACE, __func__, "status switch: %d", ERROR);

				break;
			}
			default: {
				loguear(LOG_LEVEL_TRACE, __func__, "status switch deffault: %d", nodoActual->status);

				int trabajosActuales = enviarOrdenAlPadre(MENSAJE_ORDENES_ACTUALES, nodoActual, canalPadre,
						canalRetorno, mutexPadre);

				if (trabajosActuales > -1) {
					alMenosUnNodoValido = true;
				} else {
					nodoActual->status = ERROR;
					break;
				}

				if (nodoActual->status == MAPPED || nodoActual->status == REDUCED_COMBINED) {
					if (nodoActual->status == MAPPED) {
						tipo_nodo_marta* nodoEnTrabajos = findListNodeFromRequestNode(trabajosPorNodos, nodoActual);

						if (nodoEnTrabajos->status == REDUCED_COMBINED) {
							nodoActual->status = REDUCED_COMBINED;
						}
					}

					totalReadyForReduce += 1;
				}

				*victima = NULL;
				*ordenAMandar = ORDEN_SKIP;

				if (*resultName) {
					Comun_LiberarMemoria((void**) resultName);
				}

				break;
			}
			}

			if (*ordenAMandar != SIN_ORDEN && *ordenAMandar != ORDEN_MAP) {
				if (*ordenAMandar == ORDEN_SKIP) {
					*ordenAMandar = SIN_ORDEN;
				}

				break;
			}
		}

		if (*ordenAMandar != SIN_ORDEN) {
			break;
		}

		if (alMenosUnNodoValido) {
			continue;
		} else {
			*ordenAMandar = ORDEN_ERROR_BLOQUE_SIN_COPIAS;
			returnValue = indicePorNumeroBloque;
			break;
		}
	}

	if (sizeListaNodos == 1 && totalReadyForReduce == 1 && *ordenAMandar == SIN_ORDEN) {
		*ordenAMandar = ORDEN_REDUCE_FINAL;
	}

	loguear(LOG_LEVEL_TRACE, __func__, "tamano de la lista de listos para reduce %d", totalReadyForReduce);

	if (totalReadyForReduce > 0 && totalReadyForReduce == sizeListaNodos && *ordenAMandar == SIN_ORDEN) {
		loguear(LOG_LEVEL_TRACE, __func__, "adentro if final mas de 1 nodo");

		if (tieneCombiner) {
			int combinerFinishedCount = 0;
			int combinerWaitingCount = 0;
			int combinerWorkingCount = 0;

			int trabajadosSize = list_size(*trabajosPorNodos);
			int indiceTrabajados = 0;

			for (indiceTrabajados = 0; indiceTrabajados < trabajadosSize; indiceTrabajados++) {
				tipo_nodo_marta *nodoTrabajado = list_get(*trabajosPorNodos, indiceTrabajados);

				loguear(LOG_LEVEL_TRACE, __func__, "recorro trabajos: %s", nodoTrabajado->nodoNombre);

				if (nodoTrabajado->status == REDUCED_COMBINED) {
					combinerFinishedCount++;
				}

				if (nodoTrabajado->status == REDUCING) {
					combinerWorkingCount++;
				}

				if (nodoTrabajado->archivosAProcesar) {
					if (list_size(nodoTrabajado->archivosAProcesar) > 0 && nodoTrabajado->status != ERROR) {
						combinerWaitingCount++;
					}
				}
			}

			if (combinerWorkingCount == 0) {
				if ((combinerFinishedCount == combinerWaitingCount) && *ordenAMandar == SIN_ORDEN) {
					*ordenAMandar = ORDEN_REDUCE_FINAL;
					loguear(LOG_LEVEL_TRACE, __func__, "orden final 1");
				} else {
					*ordenAMandar = ORDEN_REDUCE_LOCAL;
				}
			} else {
				*ordenAMandar = SIN_ORDEN;
			}
		} else {
			*ordenAMandar = ORDEN_REDUCE_FINAL;
			loguear(LOG_LEVEL_TRACE, __func__, "orden final 2");
		}
	}

	if (*ordenAMandar != ORDEN_ERROR_BLOQUE_SIN_COPIAS) {
		returnValue = *ordenAMandar;
	}

	if (!*resultName) {
		*resultName = string_duplicate("");
	}

	loguear(LOG_LEVEL_TRACE, __func__, "Orden a enviar: %d ,nombre temporal:'%s'", *ordenAMandar, *resultName);

	if (*ordenAMandar == ORDEN_MAP) {
		(*victima)->status = MAPING;
	}

	return returnValue;
}

double calcularPrioridad(int trabajosActuales, bool combiner, int aparicionesTotalesEnJob) {
	double primerTermino = 1.0 / (trabajosActuales + 1);

	if (combiner) {
		double segundoTermino = 1.0 - 1.0 / (aparicionesTotalesEnJob);
		return primerTermino + segundoTermino;
	} else {
		return primerTermino;
	}
}
