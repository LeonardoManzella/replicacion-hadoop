#ifndef SRC_MARTA_VICTIMA_H_
#define SRC_MARTA_VICTIMA_H_

#include "marta_configuration.h"

#define ORDEN_ERROR_BLOQUE_SIN_COPIAS -5
#define SIN_ORDEN 0
#define ORDEN_MAP 1
#define ORDEN_REDUCE_LOCAL 3
#define ORDEN_REDUCE_FINAL 4
#define ORDEN_SKIP 5

int elegirVictimaYorden(tipo_nodo_marta **victima, int *ordenAMandar, char **resultName, t_list *nodosDelJob,
		t_list** trabajosPorNodo, bool combiner, int canalPadre, int canalRetorno, pthread_mutex_t *mutexPadre);

double calcularPrioridad(int trabajosActuales, bool combiner, int aparicionesTotalesEnJob);

#endif /* SRC_MARTA_VICTIMA_H_ */
