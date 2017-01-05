#ifndef MARTAHIJO_H_
#define MARTAHIJO_H_

#include <pthread.h>

#include "marta_configuration.h"

//Se encarga de comenzar un hijo de marta para el manejo de un job
//Devuelve 1 si comenzo bien, -1 si no se pudo crear el hijo
int comenzar_hijo(tipo_job_marta job, pthread_mutex_t *mutexListaPadre, int receptorPadre, int lectorPadreACerrar,
		int lectorHijos, int escritorHijosACerrar);

//Se encarga de destruir todos los datos globales creados en marta hijo para liberar la memoria
void limpiar_y_destruir();

#endif /* MARTAHIJO_H_ */
