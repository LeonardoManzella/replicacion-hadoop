#include "../headers/marta_configuration.h"

#include <commons/config.h>
#include <commons/string.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

//Inicializo un Mutex Global para los Logs, asi se puede loguear desde donde sea sin problemas.
pthread_mutex_t GLOBAL_mutex_interno_paraLogs = PTHREAD_MUTEX_INITIALIZER;

void normalizeString(char* stringToNormalize);

bool isConfigValid() {
	if (access(MARTA_CONFIG_PATH, F_OK) == -1) {
		return false;
	}

	t_config* archivoConfig = config_create(MARTA_CONFIG_PATH);
	bool returnBool = true;

	if (!config_has_property(archivoConfig, "FS_IP")) {
		returnBool = false;
	}

	if (!config_has_property(archivoConfig, "FS_PUERTO")) {
		returnBool = false;
	}

	if (!config_has_property(archivoConfig, "MARTA_PUERTO_ESCUCHA")) {
		returnBool = false;
	}

	config_destroy(archivoConfig);
	return returnBool;
}

void getFsIpAndPort(char ip[], char port[]) {
	//Estaba Mal porque Hacias Free de Variable Local con el Config Destroy
	t_config* archivoConfig = config_create(MARTA_CONFIG_PATH);

	char *ipString;
	if (config_has_property(archivoConfig, "FS_IP")) {
		ipString = config_get_string_value(archivoConfig, "FS_IP");
	} else {
		//Control de Errores para que Explote al Usarlo con Funciones Sockets
		ipString = string_duplicate("-1");
	}
	char *portString;
	if (config_has_property(archivoConfig, "FS_PUERTO")) {
		portString = config_get_string_value(archivoConfig, "FS_PUERTO");
	} else {
		//Control de Errores para que Explote al Usarlo con Funciones Sockets
		portString = string_duplicate("-1");
	}

	Macro_ImprimirParaDebug("Antes1\n");
	Macro_ImprimirParaDebug("IP:%s   Puerto:%s\n", ipString, portString);
	Macro_ImprimirParaDebug("Despues1\n");

	//Para Pruebas TODO Sacarlo
	//strcpy(ip, "127.0.0.1");
	//strcpy(port, "6000");

	strcpy(ip, ipString);
	strcpy(port, portString);

	config_destroy(archivoConfig);
}

int getMartaListeningPort() {
	//Estaba Mal porque Hacias Free de Variable Local con el Config Destroy
	//t_config config = *config_create(MARTA_CONFIG_PATH);
	t_config* archivoConfig = config_create(MARTA_CONFIG_PATH);

	int returnInt = -1;

	if (config_has_property(archivoConfig, "MARTA_PUERTO_ESCUCHA")) {
		returnInt = config_get_int_value(archivoConfig, "MARTA_PUERTO_ESCUCHA");
	}

	config_destroy(archivoConfig);

	return returnInt;
}

void loguear(t_log_level nivelDeLogueo, const char* nombreTareaFuncion, const char* textoPorLoguear, ...) {
	//Necesito juntar en un Unico String todos los char que se le pasen, sino no puedo loguear
	char* textoJuntadoParaLoguear;

	//Utilizo una Lista de Argumentos Variables y Una funcion de las Commons para juntar los Strings
	va_list listaArgumentosVariables;
	va_start(listaArgumentosVariables, textoPorLoguear);

	textoJuntadoParaLoguear = string_from_vformat(textoPorLoguear, listaArgumentosVariables);

	//Inicio Mutex
	pthread_mutex_lock(&GLOBAL_mutex_interno_paraLogs);

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
	Comun_LiberarMemoria((void**) &textoJuntadoParaLoguear);

	//Libero la Memoria y dejo de escribir en el Log
	log_destroy(miLog);

	//Fin Mutex
	pthread_mutex_unlock(&GLOBAL_mutex_interno_paraLogs);
}

void loguearPantalla(t_log_level nivelDeLogueo, const char* nombreTareaFuncion, const char* textoPorLoguear, ...) {
	//Necesito juntar en un Unico String todos los char que se le pasen, sino no puedo loguear
	char* textoJuntadoParaLoguear;

	//Utilizo una Lista de Argumentos Variables y Una funcion de las Commons para juntar los Strings
	va_list listaArgumentosVariables;
	va_start(listaArgumentosVariables, textoPorLoguear);

	textoJuntadoParaLoguear = string_from_vformat(textoPorLoguear, listaArgumentosVariables);

	//Inicio Mutex
	pthread_mutex_lock(&GLOBAL_mutex_interno_paraLogs);

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
	Comun_LiberarMemoria((void**) &textoJuntadoParaLoguear);

	//Libero la Memoria y dejo de escribir en el Log
	log_destroy(miLog);

	//Fin Mutex
	pthread_mutex_unlock(&GLOBAL_mutex_interno_paraLogs);
}

void normalizeString(char* stringToNormalize) {
//Funcion creada porque en las commons agrega un \n al final que rompe las keys
	int index = 0;
	int size = strlen(stringToNormalize);

	while (index <= size) {
		if (stringToNormalize[index] == '\n') {
			stringToNormalize[index] = '\0';
		}

		index++;
	}
}
