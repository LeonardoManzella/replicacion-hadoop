#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>		//Para Open y Close
#include <sys/stat.h>
#include <sys/mman.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <commons/string.h>
#include <commons/log.h>
#include <assert.h>

#include "../../FileSystem/headers/Biblioteca_Bloques.h"
#include "../../Biblioteca_Comun/Biblioteca_Comun.h"
#include "../../Serializador/Protocolo_Nodos_FS.h"


#include "bibliotecaNodo.h"
//NOTA: el archivo datos.bin lo creamos con el comando "dd if=/dev/zero of=datos.bin  bs=1M  count=100" (100mb)

//Always in a pipe[], pipe[0] is for read and pipe[1] is for write
#define READ_FD  0
#define WRITE_FD 1

//Función que envía los datos de un bloque de a líneas para ejecutar un mapping
int enviarBloqueDeDatos(char* contentBlock, int pipeInput);

//Función que abre los archivos a reducir y los va apareando para mandar de a una línea (la menor)
int enviarArchivosApareados(t_list* lista, int pipeInput);

//Función que agrega la siguiente línea a procesar en la lista para el archivo indicado en el elemento pasado por parámetro
int agregarSiguienteLinea(t_elemento_a_procesar* elemento);

//comparador de dos elementos t_elemento_a_procesar. Debe devolver si elem1 es menor a eleme2 (comparo por atributo linea)
//Es para el ordenamiento de list de las Commons, se usa al aparear archivos
bool compararElementosAProcesar(void* elem1, void* elem2);

//Función que elimina un elemento de la list de procesamiento para apareo. Es para el list_destroy_and_destroy_elements de las Commons
void destructor_elementoListaProcesamiento(t_elemento_a_procesar* elemento);

//Segundo log para loguear mensajes que paso a los pipes en caso de tener que debuguear
void loguear2(const char* nombreTareaFuncion, const char* textoPorLoguear, ... );

//Función que cuenta las líneas en un string
int countStrLines(char* str);

//Handler para ignorar sigpipe
void Handler_Sigpipe(int arg);

//Inicializo un Mutex Global para los Logs, asi se puede loguear desde donde sea sin problemas.
//pthread_mutex_t GLOBAL_mutex_interno_paraLogs = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t *loggingMutex = NULL;

int executeScript(char* contentBlock,char* outputFile,char* scriptFile, t_list* filesList, int scriptType) {

	int scriptPipe[2];
	int childResult = -5;
	int outputFileFD = -5;
	int tempFileFD = -5;
	char* tempFile = NULL;

	//Indico que Cada vez que me llegue la señal SIGCHLD (se termino un proceso hijo) revise si esta en estado Zombie y lo mata correctamente
	//Voy a definir la accion para la señal SIGCHLD
	/*	Creo un struct necesario para la funcion "sigaction"
	 NOTA: Es necesario poner "struct" porque hay una funcion que se llama igual al tipo de dato (sino, entiende cualquier cosa el compilador)
	 */
	struct sigaction accionSenial_SIGCHLD;

	struct sigaction accionSenial_SIGPIPE;

	//Le especifico que la accion sera llamar a la funcion "Handler_Eliminar_Zombies"
	accionSenial_SIGCHLD.sa_handler = Handler_Eliminar_Zombies;

	accionSenial_SIGPIPE.sa_handler = Handler_Sigpipe;

	//Limpio la lista de Señales que atrapa por Defecto y Establesco las Flags Necesarias
	 if( sigemptyset(&accionSenial_SIGCHLD.sa_mask)){
		 loguear(LOG_LEVEL_ERROR, __func__ , "(Linea %d) No se pudieron limpiar las señales por defecto. Error detectado: %s", __LINE__ , strerror(errno));
		 return EXIT_ERROR;
	 }
	 accionSenial_SIGCHLD.sa_flags = SA_RESTART | SA_NOCLDSTOP;

	//Ahora si establesco la Accion para la Señal, a partir de ahora es automatico, no hace falta tocar nada mas
	if (sigaction( SIGCHLD, &accionSenial_SIGCHLD, NULL) ){
		loguear(LOG_LEVEL_ERROR, __func__ , "(Linea %d) No se pudo redefinir la señal SIGCHLD. Error detectado: %s", __LINE__ , strerror(errno));
		return EXIT_ERROR;
	}

	if(sigaction(SIGPIPE, &accionSenial_SIGPIPE, NULL)) {
		loguear(LOG_LEVEL_ERROR, __func__ , "(Linea %d) No se pudo redefinir la señal SIGPIPE. Error detectado: %s", __LINE__ , strerror(errno));
		return EXIT_ERROR;
	}
	//FIN del tema de redefinición de handlers

	//verifico si se trata de mapping o reducing
	if(MAP == scriptType) {

		loguear(LOG_LEVEL_DEBUG, __func__, "Inicio rutina de mapping. Script: %s", scriptFile);
	}else {

		loguear(LOG_LEVEL_DEBUG, __func__, "Inicio rutina de reducing. Script: %s", scriptFile);
	}

	//intento abrir pipes
	Macro_Check_And_Handle_Error(-1 == pipe(scriptPipe), "Error al abrir pipe para ejecutar la rutina");

	loguear(LOG_LEVEL_DEBUG, __func__, "Hago fork.");

	//hago el fork
	pid_t pid = fork();

	if(pid < 0) {
		//error en fork -- vuelvo a intentar más tarde si existe la config
		if(GLOBAL_Nodo_Config.tiempo_reintento_fork > 0) {

			Macro_ImprimirParaDebug("Error en fork. Se reintentará en %d segundos", GLOBAL_Nodo_Config.tiempo_reintento_fork);

			sleep(GLOBAL_Nodo_Config.tiempo_reintento_fork);

			pid = fork();
		}

		//verifico que no haya fallado por segunda vez
		Macro_Check_And_Handle_Error(pid < 0, "Error en fork. Se cancela.");
	}

	if(pid == 0) {
		//en el hijo
		//cierro extremos de pipes que no corresponden

		tempFile = NULL;

		//abro archivo de salida - temporal si es map, final si no lo es
		if(MAP == scriptType) {
			//armo path para archivo intermedio
			tempFile = string_new();
			string_append(&tempFile, outputFile);
			string_append(&tempFile, ".tmp");
			outputFileFD = open(tempFile, O_CREAT | O_TRUNC | O_RDWR, 0777);
		}else {
			outputFileFD = open(outputFile, O_CREAT | O_TRUNC | O_RDWR, 0777);
		}

		Macro_Check_And_Handle_Error(-1 == outputFileFD, "Error al abrir archivo de salida.");

		close(scriptPipe[WRITE_FD]);

		loguear(LOG_LEVEL_DEBUG, __func__, "Hijo: intercambio stdin y stdout.");

		//intercambio stdin y stdout
		Macro_Check_And_Handle_Error(-1 == dup2(scriptPipe[READ_FD], STDIN_FILENO), "Hijo: Error al intercambiar stdin");

		Macro_Check_And_Handle_Error(-1 == dup2(outputFileFD, STDOUT_FILENO), "Hijo: Error al intercambiar stdout");

		//como ya intercambié, no necesito más el fd
		close(scriptPipe[READ_FD]);

		loguear(LOG_LEVEL_DEBUG, __func__, "Hijo: hago exec.");

		//defino argv a pasarle al exec
		char *argv[] = {scriptFile, NULL};

		//hago exec para intercambiar la imagen del hijo con la del script
		execv(argv[0], argv);

		loguear(LOG_LEVEL_DEBUG, __func__, "Hijo: error en exec.");

		//si llego acá es porque hubo algún error en el exec
		fprintf(stderr, "Hijo: error en exec de la rutina. script: %s, salida: %s, tipo de script: %d", scriptFile, outputFile, scriptType);
		//uso _exit para que no flushee stdout
		_exit(EXIT_ERROR);
	}

	if(pid > 0) {
		//en el padre
		//cierro extremos del pipe que no corresponden
		close(scriptPipe[READ_FD]);

		loguear(LOG_LEVEL_DEBUG, __func__, "Padre: envío datos a la rutina.");

		if(MAP == scriptType) {
			//es un mapping por lo que debo enviar datos del bloque
			Macro_Check_And_Handle_Error(-1 == enviarBloqueDeDatos(contentBlock, scriptPipe[WRITE_FD]), "Padre: Error al enviar bloque de datos");
		}else {
			//es un reducing por lo que debo aparear una lista de archivos
			Macro_Check_And_Handle_Error(-1 == enviarArchivosApareados(filesList, scriptPipe[WRITE_FD]), "Padre: Error al enviar archivos apareados");
		}

		//cierro extremo de escritura porque ya terminé de escribir en el pipe. (Esto se interpreta como un EOF del extremo de lectura, lo que hace que finalice la rutina)
		close(scriptPipe[WRITE_FD]);

		loguear(LOG_LEVEL_DEBUG, __func__, "Padre: espero finalización del hijo.");

		//espero a la finalización del hijo y obtengo su resultado
		childResult = -5;

		int result = waitpid(pid, &childResult, WUNTRACED);

		Macro_Check_And_Handle_Error(-1 == result, "Padre: error en waitpid");

		//verifico resultado del hijo
		Macro_Check_And_Handle_Error(childResult < 0, "Padre: el script volvió con error.");
	}

	//finalizado el exec del script.. ahora veo si debo hacer sort
	if(MAP == scriptType) {

		loguear(LOG_LEVEL_DEBUG, __func__, "Es un mapping. Voy a ejecutar sort.");

		loguear(LOG_LEVEL_DEBUG, __func__, "Hago fork.");

		//hago el fork
		pid = fork();

		if(pid < 0) {
			//error en fork -- vuelvo a intentar más tarde si existe la config
			if(GLOBAL_Nodo_Config.tiempo_reintento_fork > 0) {

				Macro_ImprimirParaDebug("Error en fork para sort. Se reintentará en %d segundos", GLOBAL_Nodo_Config.tiempo_reintento_fork);

				sleep(GLOBAL_Nodo_Config.tiempo_reintento_fork);

				pid = fork();
			}

			//verifico que no haya fallado por segunda vez
			Macro_Check_And_Handle_Error(pid < 0, "Error en fork para sort. Se cancela.");
		}

		if(pid == 0) {
			//en el hijo
			loguear(LOG_LEVEL_DEBUG, __func__, "Hijo: intercambio stdin");

			//armo path de temporal
			tempFile = string_new();
			string_append(&tempFile, outputFile);
			string_append(&tempFile, ".tmp");

			//abro archivo intermedio
			tempFileFD = open(tempFile, O_RDONLY);

			//intercambio stdin con el temporal
			Macro_Check_And_Handle_Error(NULL == freopen(tempFile, "r", stdin), "Hijo: error al intercambiar stdin del sort");

			//intercambio stdout con archivo final
			Macro_Check_And_Handle_Error(NULL == freopen(outputFile, "w+", stdout), "Hijo: error al intercambiar stdout del sort");

			loguear(LOG_LEVEL_DEBUG, __func__, "Hijo: hago exec");

			//defino argv a pasarle al exec
			char *argv[] = {"/usr/bin/sort", NULL};

			//hago exec para intercambiar la imagen del hijo con la del proceso sort
			execv(argv[0], argv);

			loguear(LOG_LEVEL_DEBUG, __func__, "Hijo: error en exec.");

			//si llego acá es porque hubo algún error en el exec
			fprintf(stderr, "Error en exec del SORT. script: %s, salida: %s, tipo de script: %d", scriptFile, outputFile, scriptType);
			//uso _exit para que no flushee stdout
			_exit(EXIT_ERROR);
		}

		if(pid > 0) {
			//en el padre
			loguear(LOG_LEVEL_DEBUG, __func__, "Padre: envío datos al sort.");

			//espero a la finalización del hijo y obtengo su resultado
			childResult = -5;

			int result = waitpid(pid, &childResult, WUNTRACED);

			Macro_Check_And_Handle_Error(-1 == result, "Padre: error en waitpid del sort");

			//verifico resultado del hijo
			Macro_Check_And_Handle_Error(childResult < 0, "Padre: el sort volvió con error.");

			loguear(LOG_LEVEL_DEBUG, __func__, "Sort finalizado correctamente");
		}
	}

	loguear(LOG_LEVEL_DEBUG, __func__, "Rutina ejecutada correctamente.");

	return EXIT_SUCCESS;

Error_Handler:

	loguear(LOG_LEVEL_ERROR, __func__, "Falló la función executeScript (%s)", Macro_Obtener_Errno());

	//cierro fds del pipe
	close(scriptPipe[READ_FD]);
	close(scriptPipe[WRITE_FD]);

	if(outputFileFD != -5) {

		close(outputFileFD);
	}

	if(tempFileFD != -5) {

		close(tempFileFD);
	}

	return EXIT_ERROR;
}

int enviarBloqueDeDatos(char* contentBlock, int pipeInput) {

	char* auxPointer = NULL;
	char* currentPointer = contentBlock;
	int salir = 0;
	int resultado = 0;

	while(salir == 0) {
		//reinicio puntero
		auxPointer = NULL;
		//busco el próximo salto de línea '\n'
		auxPointer = strchr(currentPointer, '\n');

		if( (NULL == auxPointer) || ('\0' == currentPointer[0]) ) {
			//no encontré más - salgo
			salir = 1;
		}else {
			//escribo la línea al pipe
			resultado = write(pipeInput, currentPointer, auxPointer - currentPointer + 1); //+1 por el \n

			if(-1 == resultado) {

				Macro_Imprimir_Error("Error al enviar línea a pipe.");
				return -1;
			}

			//avanzo el puntero para iterar
			currentPointer = auxPointer + 1;
		}
	}

	return EXIT_SUCCESS;
}

int enviarArchivosApareados(t_list* filesList, int pipeInput) {

	int cantidadArchivos = filesList->elements_count;
	Macro_ImprimirParaDebug("Cantidad Archivos:'%d'", cantidadArchivos);
	loguear(LOG_LEVEL_TRACE, __func__, "Cantidad Archivos:'%d'", cantidadArchivos);

	FILE* files[cantidadArchivos];
	int i;
	t_list* listaProcesamiento = list_create();
	char* nombreArchivo;
	char* pathCompleto;

	//abro los archivos
	for(i = 0; i < cantidadArchivos; i++) {

		pathCompleto = string_new();
		nombreArchivo = list_get(filesList, i);

		string_append(&pathCompleto, GLOBAL_Nodo_Config.dir_temp);
		string_append(&pathCompleto, nombreArchivo);

		Macro_ImprimirParaDebugConDatos("Abro archivo %s", pathCompleto);
		loguear(LOG_LEVEL_TRACE, __func__,"Abro archivo %s", pathCompleto);

		//abro archivo
		files[i] = fopen(pathCompleto, "r");

		if(files[i] == NULL) {
			Macro_Imprimir_Error("No se pudo abrir archivo:'%s'. Estaba en la Posicion '%d' de La Lista", pathCompleto, i);
			loguear(LOG_LEVEL_ERROR, __func__,"No se pudo abrir archivo:'%s'. Estaba en la Posicion '%d' de La Lista", pathCompleto, i);
			return EXIT_ERROR;
		}
	}

	t_elemento_a_procesar* elemento = NULL;

	//cargo la lista a procesar...
	for(i = 0; i < cantidadArchivos; i++) {

		elemento = malloc( sizeof(t_elemento_a_procesar) );

		elemento->archivo = files[i];
		elemento->posicionSiguiente = 0;
		memset(&elemento->linea[0], 0, CHAR_BUFFER_SIZE);

		agregarSiguienteLinea(elemento);

		list_add(listaProcesamiento, elemento);

		elemento = NULL;
	}

	//hago sort de la lista

	bool (*comparator)(void*, void*);

	comparator = &compararElementosAProcesar;

	list_sort(listaProcesamiento, comparator);

	//envio el menor por el pipe e itero
	//agrego a la lista la línea siguiente a la que saqué - si es eof cierro el archivo

	t_elemento_a_procesar* punteroAElemento = NULL;

	while(listaProcesamiento->elements_count > 0) {
		//saco el primero (ya debe estar ordenada la lista)
		punteroAElemento = (t_elemento_a_procesar*) list_get(listaProcesamiento, 0);

		//si es una línea vacía no lo mando
		if( (0 == strlen(punteroAElemento->linea)) || (0 == strcmp("\n", punteroAElemento->linea)) || (0 == strcmp("\n\0", punteroAElemento->linea)) ) {
			//no hago nada
			//Macro_ImprimirParaDebugConDatos("Encontré un salto de línea, no lo mando.");
		}else {
			//loguear2(__func__, "Envío por pipe: [Inicio]%s[Final]\nStrlen: %d", punteroAElemento->linea, strlen(punteroAElemento->linea));
			//escribo la línea en el pipe, para que se la envie al Proceso Hijo que tiene cambiado el STDIN y STDOUT por la rutina
			write(pipeInput, punteroAElemento->linea, strlen(punteroAElemento->linea));
		}

		//agrego la siguiente línea a la lista y me fijo el retorno
		if(ARCHIVO_CERRADO == agregarSiguienteLinea(punteroAElemento)) {
			//el archivo estaba cerrado - sacarlo de la lista
			list_remove_and_destroy_element(listaProcesamiento, 0, (void*)&destructor_elementoListaProcesamiento);
		}else {//si no dio archivo cerrado entonces ordeno
			//ordeno la lista nuevamente
			list_sort(listaProcesamiento, comparator);
		}

	}

	//cierro los archivos restantes y libero recursos
	for(i = 0; i < cantidadArchivos; i++) {

		fclose(files[i]);
	}

	//destruyo lista y sus elementos
	list_destroy_and_destroy_elements(listaProcesamiento, (void*)&destructor_elementoListaProcesamiento);

	return EXIT_SUCCESS;
}


int agregarSiguienteLinea(t_elemento_a_procesar* elemento) {

	//loguear(LOG_LEVEL_TRACE, __func__, "Empezo Funcion");

	if(elemento->archivo == NULL) {
		//si está en null es porque ya lo cerré - lo saco de la lista

		return ARCHIVO_CERRADO;
	}

	int j = 0;
	int salir = 0;
	long unsigned int bufferSize = CHAR_BUFFER_SIZE;
	char* bufferAux = NULL;
	int iError = 0;

	//pido memoria para el buffer
	char* buffer = malloc(CHAR_BUFFER_SIZE);

	if(NULL == buffer) {
		//error en malloc
		Macro_Imprimir_Error("%s - error en malloc", __func__);
		return EXIT_ERROR;
	}

	//loguear(LOG_LEVEL_TRACE, __func__, "Me voy a Posicionar en Archivo. FD:'%d', Posicion:'%d'", fileno(elemento->archivo), elemento->posicionSiguiente);

	//apunto a la pos siguiente
	iError = fseek(elemento->archivo, elemento->posicionSiguiente, SEEK_SET);
	if( iError != 0 ){
		Macro_Imprimir_Error("No me pude Posicionar en el Archivo");
		return EXIT_ERROR;
	}


	//loguear(LOG_LEVEL_TRACE, __func__, "Entro al While");

	//leo de a un char y voy llenando el buffer
	while(salir == 0) {

	//loguear(LOG_LEVEL_TRACE, __func__, "Ciclamos en el While");

		//si es fin de archivo corot el ciclo
		if(feof(elemento->archivo)) {
			salir = 1;
		}

		//verifico tener espacio en el buffer
		if(j >= bufferSize) {
			//si no hay realoco
			bufferAux = realloc(buffer, bufferSize + CHAR_BUFFER_SIZE);

			//verifico que haya tenido éxito la realocación
			if(NULL == bufferAux) {

				Macro_Imprimir_Error("%s - error en realloc, hago free", __func__);
				//libero memoria anterior
				free(buffer);

				return EXIT_ERROR;
			}else {
				//como funcionó el realloc, reasigno nuevo puntero a buffer
				buffer = bufferAux;
			}
		}

		//leo siguiente caracter del archivo
		buffer[j] = fgetc(elemento->archivo);

		//verifico si es fin de línea o archivo y salgo
		if(buffer[j] == '\n' || buffer[j] == '\r' || buffer[j] == '\0') {

			buffer[j+1] = '\0';

			//verifico si ya termino
			if('\0' == fgetc(elemento->archivo)) {
				//fin de archivo
				elemento->archivo = NULL;
			}
			//indico que ya terminé
			salir = 1;
		}else {
			//aumento contador
			j++;
		}
	}

	//loguear(LOG_LEVEL_TRACE, __func__, "Fin While");


	//si quedé en eof, desasigno el archivo
	if(NULL != elemento->archivo && feof(elemento->archivo)) {
		elemento->archivo = NULL;
	}

	//buffer[j+1] = '\0'; //por las dudas

	memset(&elemento->linea[0], 0, bufferSize); //limpio la linea del elemento

	strcpy(elemento->linea, buffer); //copio el buffer a la linea

	elemento->posicionSiguiente = j + elemento->posicionSiguiente + 1; //seteo posición siguiente

	//loguear(LOG_LEVEL_TRACE, __func__, "Hago Free");


	//desasigno memoria del buffer
	free(buffer);

	//loguear(LOG_LEVEL_TRACE, __func__, "Fin Funcion");

	return EXIT_SUCCESS;
}

bool compararElementosAProcesar(void* elem1, void* elem2) {

	int resultado = strcmp( ((t_elemento_a_procesar*)elem1)->linea, ((t_elemento_a_procesar*)elem2)->linea );

	if(resultado <=0) { //si resultado es menor a 0 entonces elem1 es menor a elem2 (si es igual devuelvo que elem1 es menor)
		return true;
	}else{ //sino es mayor
		return false;
	}
}

void destructor_elementoListaProcesamiento(t_elemento_a_procesar* elemento){

	if (elemento != NULL) {

		free(elemento);
	}
}

char* readFile(const char* filePath) {

	//obtengo stat del archivo para ver su size
	struct stat fileStat;

	if (stat(filePath, &fileStat) == -1) {
		Macro_Imprimir_Error("Error al realizar stat al archivo: %s", filePath);
		return NULL;
	}

	//abro el archivo
	FILE* fileStream = fopen(filePath, "r");

	if(fileStream == NULL) {
		Macro_Imprimir_Error("Error al intentar abrir archivo: %s", filePath);
		return NULL;
	}

	//aloco memoria para guardar el contenido del archivo
	char* fileContent = malloc((size_t) fileStat.st_size);

	if((int) fileContent == -1) {
		Macro_Imprimir_Error("Error al realizar malloc.");
		return NULL;
	}

	//leo el contenido del archivo
	size_t result = fread(fileContent, (size_t) fileStat.st_size, 1, fileStream);

	if(result == 0 || ferror(fileStream) != 0 || strcmp(fileContent,"") == 0) {
		Macro_Imprimir_Error("Error al intentar leer el archivo: %s", filePath);
		return NULL;
	}

	//cierro el archivo
	if(fclose(fileStream) != 0) {
		Macro_Imprimir_Error("Error al intentar cerrar archivo: %s", filePath);
		return NULL;
	}

	return fileContent;
}

uint32_t calcularCantidadBloques(char* archivo_bin) {

	struct stat sb;
	uint32_t numeroError;

	//veo si existe el archivo bin
	if(Comun_existeArchivo(archivo_bin) < 0) {
		//no existe
		fprintf(stderr, "calcularCantidadBloques: no existe el archivo bin: %s\n", archivo_bin);
		return EXIT_ERROR;
	}

	//hago stat para obtener el size del archivo
	numeroError = stat(archivo_bin, &sb);
	if(numeroError < 0) {
		fprintf(stderr, "calcularCantidadBloques: error en stat (%s)\n", strerror(errno));
		return EXIT_ERROR;
	}

	//devuelvo la cantidad de bloques dividiendo el size por el tamaño de los bloques
	return sb.st_size / TAMANIO_BLOQUE;
}

int guardarScriptEnDisco(char* nombreArchivo, char* contenidoDelScript, char* pathDestino, uint32_t scriptSize) {

	loguear(LOG_LEVEL_TRACE, __func__, "Se va a generar la rutina:'%s' en '%s'", nombreArchivo, pathDestino);

	char* pathCompleto = string_new();
	string_append(&pathCompleto, pathDestino);
	string_append(&pathCompleto, nombreArchivo);

	loguear(LOG_LEVEL_TRACE, __func__, "creo el archivo:'%s'",pathCompleto );

	//creo el archivo,  si existe lo piso

	//creo el archivo con open
	int fd = open(pathCompleto, O_CREAT | O_TRUNC, 0777);

	if(-1 == fd) {
		Macro_Imprimir_Error("Error al intentar crear el archivo %s con open", pathCompleto);
		return EXIT_ERROR;
	}

	if(-1 == close(fd)) {
		Macro_Imprimir_Error("Error al intentar cerrar el archivo %s creado con open. Continuando ejecución...", pathCompleto);
	}

	//lo abro con fopen
	FILE* script = fopen(pathCompleto, "w");

	if(NULL == script) {
		Macro_Imprimir_Error("Error al intentar abrir el archivo %s con fopen\n", pathCompleto);
		return EXIT_ERROR;
	}

	//escribo el contenido del script en el nuevo archivo
	if( 0 >= fwrite(contenidoDelScript, scriptSize, 1, script)) {

		Macro_Imprimir_Error( "Error al intentar escribir en el archivo %s", pathCompleto);
		fclose(script);
		return EXIT_ERROR;
	}

	//cierro el archivo
	if(0 != fclose(script)) {
		Macro_Imprimir_Error( "Error al intentar cerrar el archivo %s ", pathCompleto);
		return EXIT_ERROR;
	}

	return EXIT_SUCCESS;
}

int leerArchivoConfig(t_nodo_config* nodo_config) {

	//verifico si existe el archivo de config
	int resultado = Comun_existeArchivo(RUTA_ARCHIVO_CONFIG);

	if(-1 == resultado) {
		fprintf(stdout, "leerArchivoConfig: Error. No existe el archivo de config (%s)\n", RUTA_ARCHIVO_CONFIG);
		return -1;
	}

	//genero el config
	t_config *config = config_create(RUTA_ARCHIVO_CONFIG);

	//valido que estén los campos y los voy asignando al nodo_config
	if(config_has_property(config, "PUERTO_FS") == true) {
		Macro_ImprimirParaDebug("leerArchivoConfig - leo config puerto fs\n");
		nodo_config->puerto_fs = config_get_int_value(config, "PUERTO_FS");
	}
	else {
		Macro_Imprimir_Error("El archivo de configuracion del Nodo no tiene el campo PUERTO_FC\n");
		return -1;
	}

	if(config_has_property(config, "PUERTO_NODO") == true) {
		Macro_ImprimirParaDebug("leerArchivoConfig - leo config puerto nodo\n");
		nodo_config->puerto_nodo = config_get_int_value(config, "PUERTO_NODO");
	}
	else {
		Macro_Imprimir_Error("El archivo de configuracion del Nodo no tiene el campo PUERTO_NODO\n");
		return -1;
	}

	if(config_has_property(config, "IP_FS") == true) {
		Macro_ImprimirParaDebug("leerArchivoConfig - leo config ip fs\n");
		strcpy(nodo_config->ip_fs, config_get_string_value(config, "IP_FS"));
	}
	else {
		Macro_Imprimir_Error("El archivo de configuracion del Nodo no tiene el campo IP_FS\n");
		return -1;
	}

	if(config_has_property(config, "IP_NODO") == true) {
		Macro_ImprimirParaDebug("leerArchivoConfig - leo config ip nodo\n");
		strcpy(nodo_config->ip_nodo, config_get_string_value(config, "IP_NODO"));
	}
	else {
		Macro_Imprimir_Error("El archivo de configuracion del Nodo no tiene el campo IP_NODO\n");
		return -1;
	}

	if(config_has_property(config, "ARCHIVO_BIN") == true) {
		Macro_ImprimirParaDebug("leerArchivoConfig - leo config archivo bin\n");
		//strcpy(nodo_config->nombre_bin, "datos.bin");
		strcpy(nodo_config->nombre_bin, config_get_string_value(config, "ARCHIVO_BIN"));
	}
	else {
		Macro_Imprimir_Error("El archivo de configuracion del Nodo no tiene el campo ARCHIVO_BIN\n");
		return -1;
	}

	if(config_has_property(config, "DIR_TEMP") == true) {
		Macro_ImprimirParaDebug("leerArchivoConfig - leo config dir temp\n");
		strcpy(nodo_config->dir_temp, config_get_string_value(config, "DIR_TEMP"));
		nodo_config->dir_temp[strlen(nodo_config->dir_temp) + 1] = '\0';
	}
	else {
		Macro_Imprimir_Error("El archivo de configuracion del Nodo no tiene el campo DIR_TEMP\n");
		return -1;
	}

	if(config_has_property(config, "NOMBRE_NODO") == true) {
		Macro_ImprimirParaDebug("leerArchivoConfig - leo config nombre nodo\n");

		//strcpy(nodo_config->nombre_nodo, "Nodo1");
		strcpy(nodo_config->nombre_nodo, config_get_string_value(config, "NOMBRE_NODO"));
	}
	else {
		Macro_Imprimir_Error("El archivo de configuracion del Nodo no tiene el campo DIR_TEMP\n");
		return -1;
	}

	if(config_has_property(config, "NODO_NUEVO") == true) {
		Macro_ImprimirParaDebug("leerArchivoConfig - leo config nodo nuevo\n");
		if(strcmp(config_get_string_value(config, "NODO_NUEVO"), "SI") == 0) {
			nodo_config->is_nuevo = true;
		}
		else {
			nodo_config->is_nuevo = false;
		}
	}
	else {
		Macro_Imprimir_Error("El archivo de configuracion del Nodo no tiene el campo NODO_NUEVO\n");
		return -1;
	}

	if(config_has_property(config, "TIEMPO_REINTENTO_FORK") == true) {
		Macro_ImprimirParaDebug("leerArchivoConfig - leo config tiempo reintento fork\n");
		nodo_config->tiempo_reintento_fork = config_get_int_value(config, "TIEMPO_REINTENTO_FORK");
	}
	else {
		Macro_Imprimir_Error("El archivo de configuracion del Nodo no tiene el campo PUERTO_NODO\n");
		//no cancelo.. seteo default
		nodo_config->tiempo_reintento_fork = 0;
	}

	//informo los valores del config
	Macro_ImprimirParaDebugConDatos("El puerto del FileSystem:  	 %d", nodo_config->puerto_fs);
	Macro_ImprimirParaDebugConDatos("El puerto del Nodo:        	 %d", nodo_config->puerto_nodo);
	Macro_ImprimirParaDebugConDatos("La IP del FileSystem:      	 %s", nodo_config->ip_fs);
	Macro_ImprimirParaDebugConDatos("La IP del Nodo:           		 %s", nodo_config->ip_nodo);
	Macro_ImprimirParaDebugConDatos("El nombre del archivo bin: 	 %s", nodo_config->nombre_bin);
	Macro_ImprimirParaDebugConDatos("El nombre del nodo:       	 	 %s", nodo_config->nombre_nodo);
	Macro_ImprimirParaDebugConDatos("El directorio temporal:    	 %s", nodo_config->dir_temp);
	Macro_ImprimirParaDebugConDatos("Es un nodo nuevo:               %d", nodo_config->is_nuevo);
	Macro_ImprimirParaDebugConDatos("El tiempo de reintento de fork: %d", nodo_config->tiempo_reintento_fork);

	//borro config
	config_destroy(config);

	return EXIT_SUCCESS;
}

void loguear(t_log_level nivelDeLogueo, const char* nombreTareaFuncion, const char* textoPorLoguear, ... ){
	//Necesito juntar en un Unico String todos los char que se le pasen, sino no puedo loguear
	char* textoJuntadoParaLoguear;

	//Utilizo una Lista de Argumentos Variables y Una funcion de las Commons para juntar los Strings
	va_list listaArgumentosVariables;
	va_start(listaArgumentosVariables, textoPorLoguear);

	textoJuntadoParaLoguear = string_from_vformat(textoPorLoguear, listaArgumentosVariables);


	//Inicio Mutex
	pthread_mutex_lock(loggingMutex);

	//Creo mi Log, defino que NO se muestre por pantalla y defino el Nivel de Log.
		//NOTA: Si no existe el Log, lo Crea. Si ya existia agrega cosas al final
	t_log *miLog = log_create( ARCHIVO_DE_LOG, (char*) nombreTareaFuncion, false, LOG_LEVEL_TRACE);


	//Veo con que nivel debo Imprimir
	switch (nivelDeLogueo) {
		case LOG_LEVEL_TRACE:
			log_trace(miLog, textoJuntadoParaLoguear);
			break;
		case LOG_LEVEL_DEBUG:
			log_debug(miLog, textoJuntadoParaLoguear);
			break;
		case LOG_LEVEL_INFO:
			log_info(miLog, textoJuntadoParaLoguear);
			break;
		case LOG_LEVEL_WARNING:
			log_warning(miLog, textoJuntadoParaLoguear);
			break;
		case LOG_LEVEL_ERROR:
			log_error(miLog, textoJuntadoParaLoguear);
			break;
		default:
			printf("Me mandaste a imprimir un nivel de Logueo que no existe, revisa los Argumentos");
			break;
	}

	//Libero la Memoria del va_list
	va_end(listaArgumentosVariables);

	//Libero la Memoria del Char* Dinamico
	Comun_LiberarMemoria((void**)&textoJuntadoParaLoguear);

	//Libero la Memoria y dejo de escribir en el Log
	log_destroy(miLog);

	//Fin Mutex
	pthread_mutex_unlock(loggingMutex);
}

void loguear2(const char* nombreTareaFuncion, const char* textoPorLoguear, ... ){

	t_log_level nivelDeLogueo = LOG_LEVEL_TRACE;

	//Necesito juntar en un Unico String todos los char que se le pasen, sino no puedo loguear
	char* textoJuntadoParaLoguear;

	//Utilizo una Lista de Argumentos Variables y Una funcion de las Commons para juntar los Strings
	va_list listaArgumentosVariables;
	va_start(listaArgumentosVariables, textoPorLoguear);

	textoJuntadoParaLoguear = string_from_vformat(textoPorLoguear, listaArgumentosVariables);


	//Inicio Mutex
	pthread_mutex_lock(loggingMutex);

	//Creo mi Log, defino que NO se muestre por pantalla y defino el Nivel de Log.
		//NOTA: Si no existe el Log, lo Crea. Si ya existia agrega cosas al final
	t_log *miLog = log_create( "LogSecundario.txt", (char*) nombreTareaFuncion, false, LOG_LEVEL_TRACE);


	//Veo con que nivel debo Imprimir
	switch (nivelDeLogueo) {
		case LOG_LEVEL_TRACE:
			log_trace(miLog, textoJuntadoParaLoguear);
			break;
		case LOG_LEVEL_DEBUG:
			log_debug(miLog, textoJuntadoParaLoguear);
			break;
		case LOG_LEVEL_INFO:
			log_info(miLog, textoJuntadoParaLoguear);
			break;
		case LOG_LEVEL_WARNING:
			log_warning(miLog, textoJuntadoParaLoguear);
			break;
		case LOG_LEVEL_ERROR:
			log_error(miLog, textoJuntadoParaLoguear);
			break;
		default:
			printf("Me mandaste a imprimir un nivel de Logueo que no existe, revisa los Argumentos");
			break;
	}

	//Libero la Memoria del va_list
	va_end(listaArgumentosVariables);

	//Libero la Memoria del Char* Dinamico
	Comun_LiberarMemoria((void**)&textoJuntadoParaLoguear);

	//Libero la Memoria y dejo de escribir en el Log
	log_destroy(miLog);

	//Fin Mutex
	pthread_mutex_unlock(loggingMutex);
}


void loguearPantalla(t_log_level nivelDeLogueo, const char* nombreTareaFuncion, const char* textoPorLoguear, ... ){
	//Necesito juntar en un Unico String todos los char que se le pasen, sino no puedo loguear
	char* textoJuntadoParaLoguear;

	//Utilizo una Lista de Argumentos Variables y Una funcion de las Commons para juntar los Strings
	va_list listaArgumentosVariables;
	va_start(listaArgumentosVariables, textoPorLoguear);

	textoJuntadoParaLoguear = string_from_vformat(textoPorLoguear, listaArgumentosVariables);

	//Inicio Mutex
	pthread_mutex_lock(loggingMutex);

	//Aca Imprimo a Mano por Pantalla, porque el de LOG se ve Feo ya que da mucha info innecesaria
	Macro_ImprimirParaDebug("[%s]", nombreTareaFuncion);
	printf("%s \n", textoJuntadoParaLoguear);

	//Creo mi Log, defino que NO se muestre por pantalla y defino el Nivel de Log.
		//NOTA: Si no existe el Log, lo Crea. Si ya existia agrega cosas al final
	t_log *miLog = log_create( ARCHIVO_DE_LOG, (char*) nombreTareaFuncion, false, LOG_LEVEL_TRACE);


	//Veo con que nivel debo Imprimir
	switch (nivelDeLogueo) {
		case LOG_LEVEL_TRACE:
			log_trace(miLog, textoJuntadoParaLoguear);
			break;
		case LOG_LEVEL_DEBUG:
			log_debug(miLog, textoJuntadoParaLoguear);
			break;
		case LOG_LEVEL_INFO:
			log_info(miLog, textoJuntadoParaLoguear);
			break;
		case LOG_LEVEL_WARNING:
			log_warning(miLog, textoJuntadoParaLoguear);
			break;
		case LOG_LEVEL_ERROR:
			log_error(miLog, textoJuntadoParaLoguear);
			break;
		default:
			printf("Me mandaste a imprimir un nivel de Logueo que no existe, revisa los Argumentos");
			break;
	}

	//Libero la Memoria del va_list
	va_end(listaArgumentosVariables);

	//Libero la Memoria del Char* Dinamico
	Comun_LiberarMemoria((void**)&textoJuntadoParaLoguear);

	//Libero la Memoria y dejo de escribir en el Log
	log_destroy(miLog);

	//Fin Mutex
	pthread_mutex_unlock(loggingMutex);
}

int countStrLines(char* str) {

	char* pointer = str;
	int counter = 0;

	while (pointer != NULL) {

		pointer = strchr(pointer, '\n');

		if(pointer != NULL) {
			pointer = pointer + 1;
			counter++;
		}
	}

	return counter;
}

//Rutina para Eliminar Procesos Zombies (se encarga automaticamente)
void Handler_Eliminar_Zombies(int valorQueNoUso) {
	//NOTA: Los Printf Comentados son Importantes, no los Borren. Les serviran para Debug si hay algun problema con el Handler

	//Les Recomiendo que no Quiten este Mensaje, porque sino no saben cuando se llamo al Handler. Tambien les Permite saber cuando se Termino Cualquier Proceso (Normal o Zombie)
	loguear(LOG_LEVEL_TRACE, "Handler_Eliminar_Zombies" , "Se termino un proceso (Sea Normal o Zombie) \n" );

	//printf("Voy a Eliminar Proceso Zombie.");

	//Primero Veo si es un Proceso Zombie, en caso que no lo Sea Nunca entra al While
	pid_t idProceso = waitpid((pid_t) -1 , 0 , WNOHANG);
	//printf("ID: %d\n" , (int) idProceso);

	//Elimino todos los Procesos Hijos (creados con fork) (que se Acaban de Terminar)  que encuentra
	while (idProceso > 0) {
		//printf("·");

		//Para Debug,Hago un Logueo cada ves que se termino de ejecutar.
		loguear(LOG_LEVEL_TRACE, "Handler_Eliminar_Zombies" , "Se detecto que un proceso de ID %d se termino, para evitar que se convierta en Zombie, se liberaron sus recursos \n" , idProceso );

		//Ya que estamos (para Aprobechar Recursos y Rendimiento), veo si hay otros Procesos Terminados para Liberar sus Recursos
		idProceso = waitpid((pid_t) -1 , 0 , WNOHANG);
	}
	return;
}

void Handler_Sigpipe(int arg) {
	//Ignoro sigpipe
	loguear(LOG_LEVEL_ERROR, __func__ , "(Linea %d) Se recibió la señal SIGPIPE. Verificar por qué se rompió el pipe!. Error detectado: %s", __LINE__ , strerror(errno));
}

//inicia el mutex para logging
int initLogMutex() {
	int numeroError = 0;

	// place our shared data in shared memory
	int prot = PROT_READ | PROT_WRITE;
	int flags = MAP_SHARED | MAP_ANONYMOUS;
	loggingMutex = mmap(NULL, sizeof(pthread_mutex_t), prot, flags, -1, 0);

	if(-1 == (int) loggingMutex) {
		Macro_Imprimir_Error("Error al hacer mmap");
		return EXIT_ERROR;
	}

	assert(loggingMutex);

	// initialise mutex so it works properly in shared memory
	pthread_mutexattr_t attr;
	numeroError = pthread_mutexattr_init(&attr);
	if (numeroError != 0) {
		Macro_Imprimir_Error("No se Inicializo Bien los Atributos del Mutex de logging");
		return EXIT_ERROR;
	}

	numeroError = pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
	if (numeroError != 0) {
		Macro_Imprimir_Error("No se Establecieron Bien los Atributos del Mutex de logging");
		return EXIT_ERROR;
	}

	numeroError = pthread_mutex_init(loggingMutex, &attr);
	if (numeroError != 0) {
		Macro_Imprimir_Error("El Mutex de logging no se Inicializo Bien");
		return EXIT_ERROR;
	}

	return EXIT_SUCCESS;
}
