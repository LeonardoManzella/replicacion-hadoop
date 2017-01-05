//	Prueba Basica con 3 Nodos que Solamente Se Conectan y Pueden Recibir Bloques de Archivos
//Para no Complicarla, hacemos que el Nombre de cada Nodo sea el mismo numero de Puerto

#include <stdio.h>
#include <pthread.h>

#include "Servidor_Nodos.h"

#define PUERTO_PRIMER_NODO "7001"

#define PUERTO_SEGUNDO_NODO "7002"

#define PUERTO_TERCER_NODO "7003"

int main(){
	//Variable para Manejo de Errores
	int iNumeroError = 0;

	Servidor_loguear(__func__, "----------------------------------------------------------", LOG_LEVEL_INFO);
	Servidor_loguear(__func__, "----------------------------------------------------------", LOG_LEVEL_INFO);
	printf("Iniciando Test...\n");


	pthread_t idThread;

	//Primer Nodo
	iNumeroError = pthread_create(&idThread, NULL, (void*) &threadNodo,(void*) PUERTO_PRIMER_NODO);
	//Chequeo Errores al crear el thread
	if (iNumeroError != 0) {
		perror("Ocurrio un Error al crear el Thread del Primer Nodo. El Error es");
		return 0;
	}

	//Hago Tiempo, por los Mensajes en Consola
	sleep(2);

	//Segundo Nodo
	iNumeroError = pthread_create(&idThread, NULL, (void*) &threadNodo,(void*) PUERTO_SEGUNDO_NODO);
	//Chequeo Errores al crear el thread
	if (iNumeroError != 0) {
		perror("Ocurrio un Error al crear el Thread del Segundo Nodo. El Error es");
		return 0;
	}

	//Hago Tiempo, por los Mensajes en Consola
	sleep(2);

	//Tercer Nodo
	iNumeroError = pthread_create(&idThread, NULL, (void*) &threadNodo,(void*) PUERTO_TERCER_NODO);
	//Chequeo Errores al crear el thread
	if (iNumeroError != 0) {
		perror("Ocurrio un Error al crear el Thread del Tercer Nodo. El Error es");
		return 0;
	}


	//Aca Irian Peticiones Especiales para Casos de Prueba


	//Para que no se Termine EL proceso y me cierre los Threads
	pthread_join(idThread, NULL);

	return 1;
}
