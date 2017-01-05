#include "../headers/marta_hijo.h"

#include <commons/collections/list.h>
#include <commons/log.h>
#include <commons/string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>

#include "../../Biblioteca_Comun/Biblioteca_Comun.h"
#include "../../Serializador/Protocolo_Marta_FS.h"
#include "../../Serializador/Protocolo_Marta_JOB_Nodo.h" 	//Para el Destructor
#include "../headers/marta_configuration.h"
#include "../headers/marta_list_manipulation.h"
#include "../headers/marta_server.h"
#include "../headers/marta_victima.h"
#include "../headers/marta_comunicacion_padre_hijo.h"

#define INFINITO 1

tipo_job_marta mainJob;
pthread_mutex_t *mutexPadre;
int canalReceptorPadre;
int canalLectorHijo;

//Esta se usa en caso de combiner Y como tabla de resultados post reduce
t_list* trabajosEnNodo;

int pedirYTransformarListasFs(t_list** nodosPorBloque, t_list** trabajosEnNodo);
void transformarListaFsEnListaMarta(t_list **listaFs, t_list** nodosPorBloque, t_list** trabajosEnNodo);
void thread_orden_job(void* ordenEnviada);
char* generarNombreDeArchivo(char* referenciaBloque, char* sufijo);
t_list* generarListaDeNodosExternos();
void startCombinedReduce(sem_t *semaforoOrdenes);
void terminarJobPorErrorDeConexion(sem_t *semaforoOrdenes);

bool ordenFinalEnviada = false;
bool jobTerminado = false;

int comenzar_hijo(tipo_job_marta job, pthread_mutex_t *mutexListaPadre, int receptorPadre, int lectorPadreACerrar,
		int lectorHijos, int escritorHijosACerrar) {
	loguear(LOG_LEVEL_INFO, __func__, "Creando Hijo para Atender a un JOB");

	pid_t sonPid = fork();

	if (sonPid < 0) {
		//si es menor a 0, dio error el fork, devuelvo error
		return -1;
	} else if (sonPid != 0) {
		//si es distinto de 0 es el padre, no sigo corriendo la logica, pero si devuelvo mayor a 0
		//para que el padre sepa que se creo algo
		loguear(LOG_LEVEL_TRACE, __func__, "Hijo Creado");
		return 1;
	}

	jobTerminado = false;
	ordenFinalEnviada = false;

	//Se cierran en el hijo para evitarme problemas, los pipes tienen 2 extremos, lector y escritor. Aca yo tengo 2 pipes, por ende
	//hay 2 lectores y 2 escritores. Cuando uno hace fork, debe cerrar el lado que no va a usar. Del lado del padre no puedo hacer esto,
	//ya que cerraria los descriptores para futuros fork, pero si puedo del lado del hijo, para control interno y que nadie se confunda al usarlos
	close(lectorPadreACerrar);
	close(escritorHijosACerrar);

	char fsIp[LONGITUD_CHAR_IP];
	char fsPort[LONGITUD_CHAR_PUERTOS];

	getFsIpAndPort(fsIp, fsPort);

	if (!FileSystemActivo(fsIp, fsPort)) {
		Servidor_enviarOrden_EndJOB(job.jobIP, job.jobPuerto,
				"El FileSystem NO esta Disponible en este Momento, por favor intentelo mas tarde");
		//No hace falta Imprimir en Pantalla Porque la funcion de Arriba se Encarga

		list_destroy_and_destroy_elements(job.archivosAProcesar, (void*) &destructor_elementoListaArchivo);
		return 0;
	}

	//contiene los datos del job trabajado
	mainJob = job;

	//mutex de comunicacion, se lo paso a las funcion que lo wrapea para unlock y lock despues
	mutexPadre = mutexListaPadre;

	//file descriptor, donde recibe ordenes el padre
	canalReceptorPadre = receptorPadre;

	//file descriptor donde el padre manda respuestas(compartido entre todos, por eso el mutex)
	canalLectorHijo = lectorHijos;

	//Tiene los nodos agrupados por bloque
	t_list* nodosPorBloque = list_create();

	//Esta se usa en caso de combiner Y como tabla de resultados post reduce
	trabajosEnNodo = list_create();

	if (pedirYTransformarListasFs(&nodosPorBloque, &trabajosEnNodo) < 0) {
		jobTerminado = true;

		loguear(LOG_LEVEL_ERROR, __func__, "error pidiendo lista\n");

		Servidor_enviarOrden_EndJOB(job.jobIP, job.jobPuerto,
				"Uno de los archivos solicitados NO esta Disponible en el FileSystem. El proceso no pudo iniciarse");
		//No hace falta Imprimir en Pantalla Porque la funcion de Arriba se Encarga
	}

	int semaphoreStartValue = list_size(nodosPorBloque);
	if (!(semaphoreStartValue > 0)) {
		loguear(LOG_LEVEL_TRACE, __func__, "semaforo size menor a 1\n");

		semaphoreStartValue = 1; //Por si por algun motivo la lista estaba vacia, es porque dio error en algun punto de arriba, entonces lo iniciio en 1 al menos
	}

	if (semaphoreStartValue > 12) {
		semaphoreStartValue = 12;
	}

	loguear(LOG_LEVEL_TRACE, __func__, "semaforo inicializado en %d\n", semaphoreStartValue);

	sem_t semaforoOrdenes;
	sem_init(&semaforoOrdenes, 0, semaphoreStartValue);

	while (INFINITO) {
		loguear(LOG_LEVEL_TRACE, __func__, "semaforo wait en while\n");
		sem_wait(&semaforoOrdenes);
		loguear(LOG_LEVEL_TRACE, __func__, "semaforo siguio en while\n");

		loguear(LOG_LEVEL_TRACE, __func__, "termino job vale %d\n", jobTerminado);
		if (jobTerminado) {
			loguear(LOG_LEVEL_TRACE, __func__, "termino job\n");
			break;
		}

		sleep(1);

		int ordenCalculada = SIN_ORDEN;
		char* nombreArchivoResultado = NULL;
		tipo_nodo_marta *victima = NULL;

		int nodosBloqueValue = list_size(nodosPorBloque);
		loguear(LOG_LEVEL_TRACE, __func__, "tamanio lista pre victima %d", nodosBloqueValue);

		elegirVictimaYorden(&victima, &ordenCalculada, &nombreArchivoResultado, nodosPorBloque, &trabajosEnNodo,
				mainJob.tieneCombiner, canalReceptorPadre, canalLectorHijo, mutexPadre);

		if (victima) {
			loguear(LOG_LEVEL_TRACE, __func__, "post victima %s", victima->nodoNombre);
		}

		switch (ordenCalculada) {
			case ORDEN_ERROR_BLOQUE_SIN_COPIAS: {
				jobTerminado = true;

				Servidor_enviarOrden_EndJOB(job.jobIP, job.jobPuerto,
						"Uno de los bloques no esta disponible y Es Imposible Re-Planificar. El JOB no pudo continuar.\nPuede que se haya Cortado la Coenxion a Todos los Nodos que contenian los Bloques.\nO tambien puede ser que los Nodos esten tardando Mucho en Procesar.");
				//No hace falta Imprimir en Pantalla Porque la funcion de Arriba se Encarga

				loguear(LOG_LEVEL_TRACE, __func__, "signal semaphore");
				sem_post(&semaforoOrdenes);
				break;
			}

			case SIN_ORDEN: {
				break;
			}

			default: {
				if (ordenCalculada == ORDEN_REDUCE_FINAL) {
					if (ordenFinalEnviada) {
						//ya se mando orden final, ignoro
						break;
					} else {
						//no lo ignoro la primera vez
						ordenFinalEnviada = true;
					}
				}

				loguear(LOG_LEVEL_TRACE, __func__, "se va a mandar orden");

				tipo_orden_job_marta *orden = malloc(sizeof(tipo_orden_job_marta));

				orden->semaphore = &semaforoOrdenes;
				orden->nodo = victima;
				orden->orden = ordenCalculada;
				orden->archivoResultado = nombreArchivoResultado;

				pthread_t idThread;
				pthread_create(&idThread, NULL, (void*) &thread_orden_job, (void*) orden);

				pthread_detach(idThread);

				break;
			}
		}
	}

	loguear(LOG_LEVEL_INFO, __func__, "Salio del while, destroy semaforo");
	sem_destroy(&semaforoOrdenes);
	loguear(LOG_LEVEL_INFO, __func__, "destroyed semaforo");

	limpiar_y_destruir(nodosPorBloque, trabajosEnNodo);

	loguear(LOG_LEVEL_INFO, __func__, "FIN Hijo y Por Ende Se termino un JOB");

	list_destroy_and_destroy_elements(job.archivosAProcesar, (void*) &destructor_elementoListaArchivo);

	return 0;
}

//destruyo los elementos del final hacia adelante, sino el indice interno de la lista varia(o al menos asi deberia funcionar una lista XD)
void limpiar_y_destruir(t_list* nodosPorBloque, t_list* trabajosEnNodo) {
	int indiceListaNodos = 0;
	int sizeLsitaNodos = list_size(nodosPorBloque);

	for (indiceListaNodos = sizeLsitaNodos; indiceListaNodos > 0; indiceListaNodos--) {

		t_list* listaPorNumeroDeBloque = list_get(nodosPorBloque, indiceListaNodos - 1);

		int indicePorNumeroBloque = 0;
		int sizePorNumeroBloque = list_size(listaPorNumeroDeBloque);

		for (indicePorNumeroBloque = sizePorNumeroBloque; indicePorNumeroBloque > 0; indicePorNumeroBloque--) {
			tipo_nodo_marta* nodo = list_get(listaPorNumeroDeBloque, indicePorNumeroBloque - 1);

			Comun_LiberarMemoria((void**) &nodo);
		}

		list_destroy(listaPorNumeroDeBloque);
	}

	list_destroy(nodosPorBloque);

	int indiceTrabajo = 0;
	int sizeTrabajos = list_size(trabajosEnNodo);

	for (indiceTrabajo = sizeTrabajos; indiceTrabajo > 0; indiceTrabajo--) {
		tipo_nodo_marta* nodo = list_get(trabajosEnNodo, indiceTrabajo - 1);

		if (nodo->archivosAProcesar != NULL) {
			list_destroy_and_destroy_elements(nodo->archivosAProcesar, (void*) &destructor_elementoListaArchivo);
		}

		Comun_LiberarMemoria((void**) &nodo);
	}

	list_destroy(trabajosEnNodo);

}

void transformarListaFsEnListaMarta(t_list **listaFs, t_list** nodosPorBloque, t_list** trabajosEnNodo) {
	int bloqueActual;
	for (bloqueActual = 0; bloqueActual < list_size(*listaFs); bloqueActual++) {
		//Obtiene de la lista el puntero de la estructura triadaCopias
		tipo_triadaCopias* datosLista = list_get(*(t_list**) listaFs, bloqueActual);

		//Por cada triada Recorro las N Copias del bloque del archivo
		int copiaActual;
		t_list* subList = list_create();

		for (copiaActual = 0; copiaActual < datosLista->cantidadCopiasValidas; copiaActual++) {
			tipo_nodo_marta* nodoNuevo = malloc(sizeof(tipo_nodo_marta));
			memcpy(nodoNuevo->nodoNombre, datosLista->copia[copiaActual].nodoNombre,
					sizeof(datosLista->copia[copiaActual].nodoNombre));
			memcpy(nodoNuevo->nodoIP, datosLista->copia[copiaActual].nodoIP,
					sizeof(datosLista->copia[copiaActual].nodoIP));
			memcpy(nodoNuevo->nodoPuerto, datosLista->copia[copiaActual].nodoPuerto,
					sizeof(datosLista->copia[copiaActual].nodoIP));
			nodoNuevo->numeroDeBloqueInterno = datosLista->copia[copiaActual].numeroBloqueDentroNodo;
			nodoNuevo->tamanioEnNodo = datosLista->copia[copiaActual].sizeArchivoDentroDelBloque;
			nodoNuevo->status = NEW;

			int resultAdd = list_add(subList, nodoNuevo);

			loguear(LOG_LEVEL_TRACE, __func__,
					"Agregado Nodo a la lista a trabajar. NodoNombre:'%s'   nodoIP:'%s'  nodoPuerto:'%s'    ResuladoAdd:'%d'",
					nodoNuevo->nodoNombre, nodoNuevo->nodoIP, nodoNuevo->nodoPuerto, resultAdd);

			if (mainJob.tieneCombiner) {
				tipo_nodo_marta* nodoTrabajos = findListNodeFromRequestNode(trabajosEnNodo, nodoNuevo);
				nodoTrabajos->nodoTrabajos++;
			}
		}

		//Agrega la Lista de Nodos de las copias a la lista de listas de copias (una lista de bloques, y por cada bloque hay una lista de en que nodos estan las copias)
		int resultAdd = list_add(*nodosPorBloque, subList);

		loguear(LOG_LEVEL_TRACE, __func__, "add a la lista a trabajar.    ResultadoADD:'%d'", resultAdd);
	}

	int tamanio = list_size(*nodosPorBloque);

	loguear(LOG_LEVEL_TRACE, __func__,
			"Terminado de crear lista de Listas de Copias de Nodo. Tamanio de la Lista de Listas: %d", tamanio);
}

int pedirYTransformarListasFs(t_list** nodosPorBloque, t_list** trabajosEnNodo) {
	int returnValue = 1;

	char fsIp[LONGITUD_CHAR_IP];
	char fsPort[LONGITUD_CHAR_PUERTOS];

	getFsIpAndPort(fsIp, fsPort);

	t_list *listaAcumulada = list_create();

	int indiceArchivoPedido = 0;
	int sizeArchivos = list_size(mainJob.archivosAProcesar);

	for (indiceArchivoPedido = 0; indiceArchivoPedido < sizeArchivos; indiceArchivoPedido++) {
		char* pathArchivo = list_get(mainJob.archivosAProcesar, indiceArchivoPedido);

		loguear(LOG_LEVEL_TRACE, __func__, "Antes de pedir Archivo:'%s'\n", pathArchivo);

		t_list *requestedList = FileSystemCopiasBloqueNodo(fsIp, fsPort, pathArchivo);

		if (requestedList) {
			list_add_all(listaAcumulada, requestedList);
		} else {
			returnValue = -1;
			break;
		}

		list_destroy(requestedList); //No se le hace un Destroy Elementos porque los elementos se copiaron a la otra lista
	}

	if (returnValue > 0) {
		transformarListaFsEnListaMarta(&listaAcumulada, nodosPorBloque, trabajosEnNodo);
	}

	list_destroy_and_destroy_elements(listaAcumulada, (void*) &TriadaCopias_destruir);

	return returnValue;
}

void thread_orden_job(void* ordenEnviada) {
	loguear(LOG_LEVEL_INFO, __func__, "thread de orden comenzado");

	tipo_orden_job_marta* orden = ((tipo_orden_job_marta *) ordenEnviada);

	if (orden->nodo) {
		loguear(LOG_LEVEL_INFO, __func__, "orden status nodo %d", orden->nodo->status);
		loguear(LOG_LEVEL_INFO, __func__, "aumento orden");
		enviarOrdenAlPadre(MENSAJE_AGREGAR_ORDEN, orden->nodo, canalReceptorPadre, canalLectorHijo, mutexPadre);
	}

	loguear(LOG_LEVEL_INFO, __func__, "orden en thead: %d", orden->orden);
	switch (orden->orden) {

		case ORDEN_MAP: {
			char* nombreDeArchivoFinal = generarNombreDeArchivo(orden->archivoResultado, "M");

			loguear(LOG_LEVEL_TRACE, __func__, "nombre del temporal completo %s", nombreDeArchivoFinal);

			int requestResult = Servidor_enviarOrden_MAP_JOB(mainJob.jobIP, mainJob.jobPuerto, orden->nodo,
					nombreDeArchivoFinal);

			if (requestResult == TERMINACION_FALLIDA) {
				orden->nodo->status = ERROR;

				loguear(LOG_LEVEL_ERROR, __func__, "Error al hacer map sobre nodo %s", orden->nodo->nodoNombre);
			} else if (requestResult == NODO_DESCONECTADO) {
				orden->nodo->status = ERROR;

				enviarOrdenAlPadre(MENSAJE_DESACTIVAR_NODO, orden->nodo, canalReceptorPadre, canalLectorHijo,
						mutexPadre);

				loguear(LOG_LEVEL_ERROR, __func__, "Error al hacer map sobre nodo %s", orden->nodo->nodoNombre);
			} else if (requestResult == -3) {
				terminarJobPorErrorDeConexion(orden->semaphore);
			} else {
				orden->nodo->status = MAPPED;

				tipo_nodo_marta *nodoEnTrabajos = findListNodeFromRequestNode(&trabajosEnNodo, orden->nodo);
				nodoEnTrabajos->status = MAPPED;

				loguear(LOG_LEVEL_TRACE, __func__, "trabajos en nodo %s", nodoEnTrabajos->nodoNombre);

				if (!(nodoEnTrabajos->archivosAProcesar)) {
					loguear(LOG_LEVEL_TRACE, __func__, "adentro del if");
					nodoEnTrabajos->archivosAProcesar = list_create();
					loguear(LOG_LEVEL_TRACE, __func__, "despues de la lista");
				}

				list_add(nodoEnTrabajos->archivosAProcesar, nombreDeArchivoFinal);
				loguear(LOG_LEVEL_TRACE, __func__, "Guardado temporal %s en nodo %s", nombreDeArchivoFinal,
						nodoEnTrabajos->nodoNombre);
			}

			break;
		}

		case ORDEN_REDUCE_LOCAL: {
			if (orden->nodo) {

				int requestResult = Servidor_enviarOrden_REDUCE_LOCAL_JOB(mainJob.jobIP, mainJob.jobPuerto, orden->nodo,
						orden->nodo->archivosAProcesar, orden->archivoResultado);

				if (requestResult == TERMINACION_FALLIDA) {
					orden->nodo->status = ERROR;

					loguear(LOG_LEVEL_ERROR, __func__, "Error al hacer combine global sobre nodo %s",
							orden->nodo->nodoNombre);
				} else if (requestResult == NODO_DESCONECTADO) {
					orden->nodo->status = ERROR;

					enviarOrdenAlPadre(MENSAJE_DESACTIVAR_NODO, orden->nodo, canalReceptorPadre, canalLectorHijo,
							mutexPadre);

					loguear(LOG_LEVEL_ERROR, __func__, "Error al hacer combine global sobre nodo %s",
							orden->nodo->nodoNombre);
				} else if (requestResult == -3) {
					terminarJobPorErrorDeConexion(orden->semaphore);
				} else {
					orden->nodo->status = REDUCED_COMBINED;

					loguear(LOG_LEVEL_TRACE, __func__, "trabajo el nodo %s", orden->nodo->nodoNombre);

					int index = list_size(orden->nodo->archivosAProcesar);

					loguear(LOG_LEVEL_TRACE, __func__, "trabajos en nodo %d", index);

					int i = 0;
					for (i = index; i > 0; i--) {
						loguear(LOG_LEVEL_TRACE, __func__, "adentro del for %d", i - 1);

						void* removed = list_remove(orden->nodo->archivosAProcesar, i - 1);

						loguear(LOG_LEVEL_TRACE, __func__, "removido %s", removed);

						free(removed);
					}

					list_add(orden->nodo->archivosAProcesar, orden->archivoResultado);
				}

			} else {
				startCombinedReduce(orden->semaphore);
			}
			break;
		}

		case ORDEN_REDUCE_FINAL: {
			tipo_nodo_marta *victima_final = NULL;

			int cantidadArchivosFinales = 0;

			int indexTrabajosPorNodo = 0;
			int sizeTrabajosPorNodo = list_size(trabajosEnNodo);

			for (indexTrabajosPorNodo = 0; indexTrabajosPorNodo < sizeTrabajosPorNodo; indexTrabajosPorNodo++) {

				tipo_nodo_marta *nodoActual = list_get(trabajosEnNodo, indexTrabajosPorNodo);
				if (nodoActual->archivosAProcesar && (nodoActual->status != ERROR)) {

					if (list_size(nodoActual->archivosAProcesar) > cantidadArchivosFinales) {
						loguear(LOG_LEVEL_TRACE, __func__, "list_size(nodoActual->archivosAProcesar):'%d'",
								list_size(nodoActual->archivosAProcesar));

						cantidadArchivosFinales = list_size(nodoActual->archivosAProcesar);

						victima_final = nodoActual;
					} else if (list_size(nodoActual->archivosAProcesar) == cantidadArchivosFinales) {
						int trabajosVictimaActual = enviarOrdenAlPadre(MENSAJE_ORDENES_ACTUALES, victima_final,
								canalReceptorPadre, canalLectorHijo, mutexPadre);

						int trabajosVictimaPosible = enviarOrdenAlPadre(MENSAJE_ORDENES_ACTUALES, nodoActual,
								canalReceptorPadre, canalLectorHijo, mutexPadre);

						if (trabajosVictimaActual > trabajosVictimaPosible) {
							loguear(LOG_LEVEL_TRACE, __func__, "list_size(nodoActual->archivosAProcesar):'%d'",
									list_size(nodoActual->archivosAProcesar));

							cantidadArchivosFinales = list_size(nodoActual->archivosAProcesar);

							victima_final = nodoActual;
						}
					}
				}
			}

			t_list *listaExternos;

			listaExternos = generarListaDeNodosExternos(victima_final);

			char* nombreArchivo = basename(mainJob.jobRutaYNombreArchivoFinal);

			loguear(LOG_LEVEL_TRACE, __func__, "nombre del temporal completo:'%s'. Cantidad Archivos a Procesar:'%d'",
					nombreArchivo, list_size(victima_final->archivosAProcesar));

			int requestResult = Servidor_enviarOrden_REDUCE_FINAL_JOB(mainJob.jobIP, mainJob.jobPuerto, victima_final,
					victima_final->archivosAProcesar, listaExternos, nombreArchivo);

			char * mensajeFinalAlJob = NULL;

			if (requestResult == -2 || requestResult == -1) {
				loguear(LOG_LEVEL_ERROR, __func__, "Error al hacer reduce final sobre nodo %s",
						victima_final->nodoNombre);

				mensajeFinalAlJob = "Error al hacer reduce final";
			} else if (requestResult == -3) {
				terminarJobPorErrorDeConexion(orden->semaphore);
			} else {
				char fsIp[LONGITUD_CHAR_IP];
				char fsPort[LONGITUD_CHAR_PUERTOS];

				getFsIpAndPort(fsIp, fsPort);

				int result = FileSystemSubirArchivoNodo(fsIp, fsPort, mainJob.jobRutaYNombreArchivoFinal,
						victima_final->nodoNombre);

				if (result == -1) {
					mensajeFinalAlJob =
							"Se creo el archivo final, pero el fileSystem no pudo cargarlo.\nPuede ser que NO este creada la Ruta donde subir el Archivo Final\nPuede ser que el Archivo Ya existia Previamente\nPuede ser que haya un archivo en la misma ruta con el mismo Nombre.";
				} else {
					mensajeFinalAlJob = "Terminacion Correcta";
				}
			}

			jobTerminado = true;

			Servidor_enviarOrden_EndJOB(mainJob.jobIP, mainJob.jobPuerto, mensajeFinalAlJob);

			loguear(LOG_LEVEL_TRACE, __func__, "pre destroy");
			list_destroy_and_destroy_elements(listaExternos, free);
			loguear(LOG_LEVEL_TRACE, __func__, "post destroy");

			break;
		}
	}

	if (orden->nodo) {
		loguear(LOG_LEVEL_INFO, __func__, "disminuyo orden");
		enviarOrdenAlPadre(MENSAJE_TERMINO_UNA_ORDEN, orden->nodo, canalReceptorPadre, canalLectorHijo, mutexPadre);
	}

	loguear(LOG_LEVEL_TRACE, __func__, "signal semaphore");
	sem_post(orden->semaphore);

	Comun_LiberarMemoria((void**) &orden);
}

void terminarJobPorErrorDeConexion(sem_t *semaforoOrdenes) {
	jobTerminado = true;

	Servidor_enviarOrden_EndJOB(mainJob.jobIP, mainJob.jobPuerto,
			"Hubo un error de Conexion entre Marta y JOB, se cierra la conexion para prevenir futuros errores. Se Finaliza el JOB.");
	//No hace falta Imprimir en Pantalla Porque la funcion de Arriba se Encarga

	loguear(LOG_LEVEL_TRACE, __func__, "signal semaphore");
	sem_post(semaforoOrdenes);
}

//genera un nombre de archivo con el ip y puerto del job, asi como tambien la referencia pasada por la orden
//adicionalmente, una vez creado el nombre del archivo, hago free a la referencia de la orden
char* generarNombreDeArchivo(char* referenciaBloque, char* sufijo) {
//HACER FREE UNA VEZ TERMINADO DE USAR

	loguear(LOG_LEVEL_TRACE, __func__, "nombre recibido:'%s'", referenciaBloque);

	char template[100];
	sprintf(template, "%s%s%s%s.txt", mainJob.jobIP, mainJob.jobPuerto, referenciaBloque, sufijo);

	char *resultName = malloc(strlen(template) + 1);
	memcpy(resultName, template, strlen(template) + 1);

	//free(referenciaBloque);

	return resultName;
}

//genera la lista de nodos externos que posteriormente va a usar el dar la orden final, SIN el nodo pasado, que lo hace local
t_list* generarListaDeNodosExternos(tipo_nodo_marta* exeptionNode) {
	//HACER list_destroy y FREE  a los elementos UNA VEZ TERMINADO DE USAR
	t_list* listaExternos = list_create();

	int indiceListaTrabajados = 0;
	int sizeListaTrabajados = list_size(trabajosEnNodo);

	for (indiceListaTrabajados = 0; indiceListaTrabajados < sizeListaTrabajados; indiceListaTrabajados++) {
		tipo_nodo_marta *nodoActual = list_get(trabajosEnNodo, indiceListaTrabajados);

		int same_Name = 0;
		int same_ip = 0;
		int same_port = 0;

		if (string_equals_ignore_case(exeptionNode->nodoNombre, nodoActual->nodoNombre)) {
			same_Name = 1;
		}
		if (string_equals_ignore_case(exeptionNode->nodoIP, nodoActual->nodoIP)) {
			same_ip = 1;
		}
		if (string_equals_ignore_case(exeptionNode->nodoPuerto, nodoActual->nodoPuerto)) {
			same_port = 1;
		}

		if (same_port && same_ip && same_Name) {
			loguear(LOG_LEVEL_TRACE, __func__, "SKIP nodo victima en generar lista de nodos externos");
			continue;
		}

		if (nodoActual->status == ERROR) {
			loguear(LOG_LEVEL_TRACE, __func__, "SKIP nodo con errores");
			continue;
		}

		if (nodoActual->archivosAProcesar) {
			if (mainJob.tieneCombiner) {
				tipo_nodoExterno *nodoExternoNuevo = malloc(sizeof(tipo_nodoExterno));

				strcpy(nodoExternoNuevo->nodoIP, nodoActual->nodoIP);
				strcpy(nodoExternoNuevo->nodoPuerto, nodoActual->nodoPuerto);
				strcpy(nodoExternoNuevo->nombreArchivo, (char *) list_get(nodoActual->archivosAProcesar, 0));
				Macro_ImprimirParaDebug("NodoExternoNuevo->nombreArchivo:'%s'\n", nodoExternoNuevo->nombreArchivo);

				list_add(listaExternos, nodoExternoNuevo);
			} else {
				int indiceArchivos = 0;
				int sizeArchivos = list_size(nodoActual->archivosAProcesar);

				for (indiceArchivos = 0; indiceArchivos < sizeArchivos; indiceArchivos++) {
					tipo_nodoExterno *nodoExternoNuevo = malloc(sizeof(tipo_nodoExterno));

					strcpy(nodoExternoNuevo->nodoIP, nodoActual->nodoIP);
					strcpy(nodoExternoNuevo->nodoPuerto, nodoActual->nodoPuerto);
					strcpy(nodoExternoNuevo->nombreArchivo,
							(char *) list_get(nodoActual->archivosAProcesar, indiceArchivos));

					Macro_ImprimirParaDebug("NodoExternoNuevo->nombreArchivo:'%s'\n", nodoExternoNuevo->nombreArchivo);

					list_add(listaExternos, nodoExternoNuevo);
				}

			}
		}
	}

	return listaExternos;
}

void startCombinedReduce(sem_t *semaforoOrdenes) {
	int indiceListaTrabajados = 0;
	int sizeListTrabajados = list_size(trabajosEnNodo);

	for (indiceListaTrabajados = 0; indiceListaTrabajados < sizeListTrabajados; indiceListaTrabajados++) {
		tipo_nodo_marta *nodoActual = list_get(trabajosEnNodo, indiceListaTrabajados);
		if (nodoActual->archivosAProcesar && nodoActual->status != ERROR) {

			if (list_size(nodoActual->archivosAProcesar) == 1) {
				nodoActual->status = REDUCED_COMBINED;
				continue;
			}

			char* nombreArchivoResultado = malloc(strlen(list_get(nodoActual->archivosAProcesar, 0)) + 1);
			memcpy(nombreArchivoResultado, list_get(nodoActual->archivosAProcesar, 0),
					strlen(list_get(nodoActual->archivosAProcesar, 0)) + 1);

			nombreArchivoResultado[strlen(list_get(nodoActual->archivosAProcesar, 0))] = '\0';

			int strIndex = 0;
			for (strIndex = 0; strIndex < strlen(nombreArchivoResultado); strIndex++) {
				if (nombreArchivoResultado[strIndex] == 'M') {
					nombreArchivoResultado[strIndex] = 'C';
				}
			}

			nodoActual->status = REDUCING;

			loguear(LOG_LEVEL_TRACE, __func__, "enviando reduce local a %s con %d archivos y el nombre %s",
					nodoActual->nodoNombre, list_size(nodoActual->archivosAProcesar), nombreArchivoResultado);

			tipo_orden_job_marta *orden = malloc(sizeof(tipo_orden_job_marta));

			orden->semaphore = semaforoOrdenes;
			orden->nodo = nodoActual;
			orden->orden = ORDEN_REDUCE_LOCAL;
			orden->archivoResultado = nombreArchivoResultado;

			pthread_t idThread;
			pthread_create(&idThread, NULL, (void*) &thread_orden_job, (void*) orden);

			sleep(1);
		}
	}
}

