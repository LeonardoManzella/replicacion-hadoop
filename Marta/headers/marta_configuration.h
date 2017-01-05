#ifndef MARTACONFIG_H_
#define MARTACONFIG_H_

#include <commons/collections/list.h>
#include <commons/log.h>
#include <stdbool.h>

#include "../../Biblioteca_Comun/Biblioteca_Comun.h"		//Para las Constantes

#define MARTA_CONFIG_PATH 		"MartaConfig.cfg"
//Para Pruebas, TODO Comentarlo y Descomentar el Anterior
//#define MARTA_CONFIG_PATH 		"MartaConfig.cfg"

#define ARCHIVO_DE_LOG 			"MartaLog.txt"			//Nombre del Archivo de Logs del Marta

#define RUTA_LONGITUD_APROX		1024

#define NEW 0
#define READY 1
#define MAPING 15
#define MAPPED 2
#define REDUCING 25
#define REDUCED_COMBINED 35
#define ERROR -1

typedef struct __attribute__ ((__packed__)) {
	char nodoNombre[NODO_LONGITUD_NOMBRE];
	char nodoIP[LONGITUD_CHAR_IP];
	char nodoPuerto[LONGITUD_CHAR_PUERTOS];
	int numeroDeBloqueInterno;
	int tamanioEnNodo;
	int nodoTrabajos;
	t_list* archivosAProcesar;
	int status;
} tipo_nodo_marta;

typedef struct __attribute__ ((__packed__)) {
		char jobIP[LONGITUD_CHAR_IP];
		char jobPuerto[LONGITUD_CHAR_PUERTOS];
		char jobRutaYNombreArchivoFinal[RUTA_LONGITUD_APROX]; //FIXME Si se cortan los nombres, cambiar por RUTA_LONGITUD_MAXIMA
		t_list* archivosAProcesar;
		bool tieneCombiner;
	} tipo_job_marta;

//verifica si el archivo de configuracion es valido o no. devuelve true si es valido, false si no lo es
bool isConfigValid();

//se le debe pasar por parametro el string y el int que va a usar para devolver los valores
void getFsIpAndPort(char ip[], char port[]);

//devuelve el puerto que utilizara marta para conexciones entrantes
int getMartaListeningPort();

//inicia un proceso a partir del job especificado
void iniciarProcesoJob(tipo_job_marta job);

/*	Funcion para Loguear(escribir) en el ARCHIVO_DE_LOG del Marta
 Nivles de Logueo Disponibles:
 LOG_LEVEL_TRACE
 LOG_LEVEL_DEBUG
 LOG_LEVEL_INFO
 LOG_LEVEL_WARNING
 LOG_LEVEL_ERROR
 NOTA: Permite pasar argumentos %s al estilo Printf  */
void loguear(t_log_level nivelDeLogueo, const char* nombreTareaFuncion, const char* textoPorLoguear, ...);

//Esta Funcion ademas de realizar el Logueo Imprime por Pantalla lo que Loguea
void loguearPantalla(t_log_level nivelDeLogueo, const char* nombreTareaFuncion, const char* textoPorLoguear, ...);

#endif /* MARTACONFIG_H_ */
