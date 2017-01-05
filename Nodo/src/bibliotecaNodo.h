#ifndef BIBLIOTECA_NODO_H_
#define BIBLIOTECA_NODO_H_

#include <stdint.h>

#include <commons/config.h>
#include <commons/collections/list.h>
#include <commons/log.h>

#include "../../Biblioteca_Comun/Biblioteca_Comun.h"

#define EXIT_ERROR 	-1
#define INFINITO 1
#define RUTA_ARCHIVO_CONFIG "config.cfg"
#define ARCHIVO_DE_LOG "LogNodo.txt"
#define TASK_RUNNER "taskRunner"
#define CHAR_BUFFER_SIZE 512
#define ARCHIVO_CERRADO 2
#define SIN_ELEMENTOS 3

#define REDUCE_SIN_COMBINER 0
#define REDUCE_CON_COMBINER 1
#define MAP 2
#define REDUCE 3

typedef struct {

	uint32_t 	puerto_fs;  					 // puerto del Filesystem
	char 		ip_fs[LONGITUD_CHAR_IP];		 	 // ip del Filesystem
	char 		nombre_bin[NODO_LONGITUD_NOMBRE];  // nombre del archivo data.bin
	char 		dir_temp[NODO_LONGITUD_NOMBRE];	 // directorio para los archivos temporales
	bool 		is_nuevo;	 		 			 // true - nodo nuevo, false - nodo ya existente
	uint32_t 	puerto_nodo;		 			 // puerto del Filesystem
	char 		ip_nodo[LONGITUD_CHAR_IP];	 	 // ip del Filesystem
	char 		nombre_nodo[NODO_LONGITUD_NOMBRE]; // nombre del nodo
	uint32_t	tiempo_reintento_fork;			 // tiempo de reintento ante fallo de fork en rutina

} t_nodo_config;

 //estructura para procesar los elementos del apareamiento
typedef struct{

	char linea[CHAR_BUFFER_SIZE];
	FILE* archivo;
	uint32_t posicionSiguiente;

} t_elemento_a_procesar;

//La configuracion del nodo para que la puedan utilizar todos los threads creados
t_nodo_config GLOBAL_Nodo_Config;

/* Función que intercambia los flujos standard, sdtin y stdout con flujos ingresados por parámetro y ejecuta el script indicado.
 *
 * Nota: si filesList no es null entonces asumo que esa lista de archivos son el input para el proceso (ignoro contentBlock)
 *
 * Argumentos:
 * 	contentBlock contenido del bloque de datos de entrada del script a ejecutar.
 * 	outputFile   ruta al archivo que reemplazará a stdout.
 * 	scriptFile   ruta al script sh a ejecutar.
 * 	filesList    lista de archivos a procesar en caso de ser reduce
 * 	scriptType	 tipo de rutina a ejecutar (MAP o REDUCE)
 *
 * 	En caso de error devuelve -1.
 *  En caso de éxito devuelve el valor que retorna la ejecución del script.  */
int executeScript(char* contentBlock,  char* outputFile,   char* scriptFile,  t_list* filesList, int scriptType);

 //Lee el archivo de configuracion y guarda los datos en la estructura correspondiente
int leerArchivoConfig(t_nodo_config* nodo_config);

//Determina la cantidad de bloques del nodo. Recibe el path al bin como parámetro
uint32_t calcularCantidadBloques(char* archivo_bin);

//Genera un archivo en el path indicado en base al contenido ingresado y le asigna permisos de ejecución
int guardarScriptEnDisco(char* nombreArchivo, char* contenidoDelScript, char* pathDestino, uint32_t scriptSize);

/*Función que lee un archivo de texto a partir de un path y devuelve su contenido.
 *
 * Argumentos:
 * 	filePath ruta al archivo a leer.
 *
 * En caso de error devuelve NULL.
 * En caso de éxito devuelve el contenido del archivo.  */
char* readFile(const char* filePath);

/*	Funcion para Loguear(escribir) en el ARCHIVO_DE_LOG del JOB
			Nivles de Logueo Disponibles:
										LOG_LEVEL_TRACE
										LOG_LEVEL_DEBUG
										LOG_LEVEL_INFO
										LOG_LEVEL_WARNING
										LOG_LEVEL_ERROR
NOTA: Permite pasar argumentos %s al estilo Printf  */
void loguear(t_log_level nivelDeLogueo, const char* nombreTareaFuncion, const char* textoPorLoguear, ... );

//Esta Funcion ademas de realizar el Logueo Imprime por Pantalla lo que Loguea
void loguearPantalla(t_log_level nivelDeLogueo, const char* nombreTareaFuncion, const char* textoPorLoguear, ... );

//Handler para eliminar zombies
void Handler_Eliminar_Zombies(int valorQueNoUso);

//Inicializo mutex en mem compartida
int initLogMutex();

#endif /* BIBLIOTECA_NODO_H_*/
