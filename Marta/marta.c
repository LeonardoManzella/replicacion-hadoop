#include <assert.h>
#include <sys/mman.h>
#include <commons/collections/list.h>
#include <commons/log.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

#include "../Biblioteca_Comun/Biblioteca_Comun.h"		//Para los Colores al Imprimir
#include "../Serializador/Protocolo_Marta_JOB_Nodo.h" 	//Para el Destructor
#include "headers/marta_comunicacion_padre_hijo.h"
#include "headers/marta_configuration.h"
#include "headers/marta_hijo.h"
#include "headers/marta_list_manipulation.h"
#include "headers/marta_server.h"

#define INFINITO 1

t_list* listaNodosTrabajando;

pthread_mutex_t *parentComunicationMutex = NULL;

int parentWriter;
int parentReader;
int childReader;
int childWriter;

void initSharedMutex();

int main() {
	uint32_t iNumeroError = 0; //Variable para Manejo de Errores. Por defecto es 0 que significa sin Errores

	loguear(LOG_LEVEL_INFO, __func__, "----------------------------------------------------------");
	loguear(LOG_LEVEL_INFO, __func__, "----------------------------------------------------------");
	
	loguear(LOG_LEVEL_INFO, __func__, ">>Iniciando Marta");
	Comun_Pantalla_Separador_Destacar("Iniciando Marta");

	Macro_ImprimirEstadoInicio("Verificando Archivo Config..");
	if (!isConfigValid()) {
		Macro_ImprimirEstadoFinal(EJECUCION_FALLIDA);
		printf("El archivo de configuracion no es valido, el MARTA no puede arrancar sin una configuracion valida\n");
		printf(ANSI_COLOR_RED "Marta debe Cerrarse.." ANSI_COLOR_RESET "\n");
		return EXIT_FAILURE;
	}
	Macro_ImprimirEstadoFinal(EJECUCION_CORRECTA);

	listaNodosTrabajando = list_create();

	loguear(LOG_LEVEL_TRACE, __func__, "Creando Pipes de Comunicacion Interna");

	int pipeFather[2];
	int pipeSons[2];

	Macro_ImprimirEstadoInicio("Creando Pipes de Comunicacion Interna..");
#pragma GCC diagnostic ignored "-Wunused-result"
	iNumeroError = pipe(pipeFather);
	if( iNumeroError==-1 ){
		Macro_ImprimirEstadoFinal(EJECUCION_FALLIDA);
		printf("No se pudo Crear los Pipes de Marta Padre\n");
		printf(ANSI_COLOR_RED "Marta debe Cerrarse.." ANSI_COLOR_RESET "\n");
		return EXIT_FAILURE;
	}
		
	iNumeroError = pipe(pipeSons);
	if( iNumeroError==-1 ){
		Macro_ImprimirEstadoFinal(EJECUCION_FALLIDA);
		printf("No se pudo Crear los Pipes de Marta Padre\n");
		printf(ANSI_COLOR_RED "Marta debe Cerrarse.." ANSI_COLOR_RESET "\n");
		return EXIT_FAILURE;
	}
#pragma GCC diagnostic pop
	Macro_ImprimirEstadoFinal(EJECUCION_CORRECTA);

	parentWriter = pipeFather[1];
	parentReader = pipeFather[0];
	childWriter = pipeSons[1];
	childReader = pipeSons[0];

	loguear(LOG_LEVEL_TRACE, __func__, "Inicializando Mutex de Comunicacion Interna");
	//inicio el mutex para la comunicacion de los hijos con el padre
	initSharedMutex();

	//Preparo las cosas Necesarias para el Servidor de JOBs
	pthread_t threadServerID;
	int martaPuerto = getMartaListeningPort();

	
	loguear(LOG_LEVEL_INFO, __func__, "Iniciando Servidor de JOBs...");
	Macro_ImprimirEstadoInicio("Iniciando Servidor de JOBs..."); 		//El de Estado Final Correcto esta dentro de "Marta_Servidor_Iniciar" para no cagar lo visual

	//Se inicia Servidor de Escucha de JOBs
	iNumeroError = pthread_create(&threadServerID, NULL, (void*) &Marta_Servidor_Iniciar, (void*) &martaPuerto);
	if (iNumeroError != 0) {
		Macro_ImprimirEstadoFinal(EJECUCION_FALLIDA);
		printf("Ocurrio un Error al Crear el Thread del Servidor de JObs\n");
		printf(ANSI_COLOR_RED "Debido a un Fallo Critico el Marta debio Cerrarse.." ANSI_COLOR_RESET "\n");
		return EXIT_FAILURE;
	}
	iNumeroError = pthread_detach(threadServerID);
	if (iNumeroError != 0) {
		printf("Ocurrio un Error al hacer Detach sobre el Thread del Servidor de JObs\n");
		printf(ANSI_COLOR_RED "Debido a un Fallo Critico el Marta debio Cerrarse.." ANSI_COLOR_RESET "\n");
		return EXIT_FAILURE;
	}

	//Marta Padre se pone a escuchar a Marta Hijos
	loguear(LOG_LEVEL_TRACE, __func__, "Marta Padre se pone a escuchar a Marta Hijos");

	// read at least one byte from the pipe.
	while (INFINITO) {
		int controlDeError = 1;
		char *buffer = malloc(TAMANIO_MENSAJE + 1);
		buffer[TAMANIO_MENSAJE] = '\0';

#pragma GCC diagnostic ignored "-Wunused-result"
		controlDeError = read(parentReader, buffer, TAMANIO_MENSAJE);
#pragma GCC diagnostic pop

		loguear(LOG_LEVEL_TRACE, __func__, "Llego un Mensaje a Marta Padre. Se Procesa el Mensaje");

		int responseValue = -1;

		if (controlDeError < 0) {
			loguear(LOG_LEVEL_ERROR, __func__, "Error del read del marta padre(main)");
		} else {
			int orden = -1;
			tipo_nodo_marta nodo;

			loguear(LOG_LEVEL_TRACE, __func__, "Des-Serializando Mensaje a Marta Padre");
			deserializarOrdenNodoMarta(&buffer, &orden, &nodo);

			switch (orden) {
			case MENSAJE_ORDENES_ACTUALES:
				responseValue = trabajosActuales(&listaNodosTrabajando, &nodo);
				break;
			case MENSAJE_AGREGAR_ORDEN:
				responseValue = comenzarTrabajo(&listaNodosTrabajando, &nodo);
				break;
			case MENSAJE_TERMINO_UNA_ORDEN:
				responseValue = terminarTrabajo(&listaNodosTrabajando, &nodo);
				break;
			case MENSAJE_DESACTIVAR_NODO:
				desactivarNodo(&listaNodosTrabajando, &nodo);
				responseValue = 1;
				break;
			default:
				//Valor default para orden no reconocida, cualquier valor que no sea los demas del switch pasa por aca
				responseValue = -2;
				break;
			}
			loguear(LOG_LEVEL_TRACE, __func__, "Mensaje Procesado, escribiendo Respuesta %d a Marta Hijo",
					responseValue);
		}

		Comun_LiberarMemoria((void**) &buffer);
#pragma GCC diagnostic ignored "-Wunused-result"
		controlDeError = write(childWriter, &responseValue, sizeof(int32_t));
#pragma GCC diagnostic pop

		if (controlDeError < 0) {
			loguear(LOG_LEVEL_ERROR, __func__, "Error del write del marta padre(main), reinteno");
			controlDeError = write(childWriter, &controlDeError, sizeof(int32_t));

			if (controlDeError < 0) {
				loguear(LOG_LEVEL_ERROR, __func__,
						"Error del write del marta padre(main), libero mutex para otros mensajes que lo necesiten");
				pthread_mutex_unlock(parentComunicationMutex);
			}
		}
	}

	return EXIT_SUCCESS;
}

void iniciarProcesoJob(tipo_job_marta job) {
	int resultado = comenzar_hijo(job, parentComunicationMutex, parentWriter, parentReader, childReader, childWriter);

	if (resultado == -1) {
		//Error al enviar datos al hijo, respondo al job por orden de terminarlo

		Servidor_enviarOrden_EndJOB(job.jobIP, job.jobPuerto,
				"El job no puede procesarse en este momento, por favor intentelo mas tarde");

		printf("Ocurrio un Error al crear un proceso hijo para administrar el job, mando orden de cerrado de job\n");
		printf(
				ANSI_COLOR_RED "Ocurrio un Error al crear un proceso hijo para administrar el job, cerrando job" ANSI_COLOR_RESET "\n");

		//Soy el padre, libero la memoria de los char* de archivos.
		list_destroy_and_destroy_elements(job.archivosAProcesar, (void*) &destructor_elementoListaArchivo);
	} else if (resultado != 0) {

		//Soy el padre, libero la memoria de los char* de archivos.
		list_destroy_and_destroy_elements(job.archivosAProcesar, (void*) &destructor_elementoListaArchivo);
	}

	//Aca soy el hijo, no quiero destruir esto sino rompo mi propia lista
}

//inicia el mutex a usar por los hijos para comunicarse con el padre en la memoria compartida
void initSharedMutex() {
	int numeroError = 0;

	// place our shared data in shared memory
	int prot = PROT_READ | PROT_WRITE;
	int flags = MAP_SHARED | MAP_ANONYMOUS;
	parentComunicationMutex = mmap(NULL, sizeof(pthread_mutex_t), prot, flags, -1, 0);
	assert(parentComunicationMutex);

	// initialise mutex so it works properly in shared memory
	pthread_mutexattr_t attr;
	numeroError = pthread_mutexattr_init(&attr);
	if (numeroError != 0) {
		loguear(LOG_LEVEL_WARNING, __func__,
				"No se Inicializo Bien los Atributos del Mutex de Comunizacion Padre-Hijo");
	}

	numeroError = pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
	if (numeroError != 0) {
		loguear(LOG_LEVEL_WARNING, __func__,
				"No se Establecieron Bien los Atributos del Mutex de Comunizacion Padre-Hijo");
	}

	numeroError = pthread_mutex_init(parentComunicationMutex, &attr);
	if (numeroError != 0) {
		loguear(LOG_LEVEL_WARNING, __func__, "El Mutex de Comunizacion Padre-Hijo no se Inicializo Bien");
	}
}
