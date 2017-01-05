#include <stdio.h>
#include <stdlib.h>
//Librerias para el Mutex y los Threads
#include <pthread.h>
#include <time.h>
#include <unistd.h>		//Para Sleep


#include "../Biblioteca_Comun/Biblioteca_Comun.h"
#include "../Sockets/Biblioteca_Sockets.h"
#include "../FileSystem/headers/Biblioteca_Bloques.h"		//TODO Cambiarlo de Lugar a Serializadores
#include "../Serializador/Protocolo_Marta_JOB_Nodo.h"

#include <commons/string.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/collections/list.h>



#define INFINITO 							1
#define SEGUNDOS_ESPERA_JOB 				"5"						//Segundos de Espera entre Reintento de conectar al Marta
#define ARCHIVO_DE_LOG 						"JOBLog.txt"			//Nombre del Archivo de Logs del JOB
#define RUTA_ARCHIVO_CONFIG_JOB 			"JOBConfig.cfg"
#define UNICO_BLOQUE_RUTINA 				0						//Considero que las Rutinas deben tener un tamaño menor a 20mb, entonces le digo que lea desde el Inicio del Archivo (Byte 0)
#define TIEMPO_ESPERA_EJECUCION_RUTINAS		(100*60)					//100 Minutos



typedef struct {
	tipo_socket* 	socketConectadoAlCliente;
	t_header 		headerRecibido;
	char 			rutina[40];
	char			tipoOrdenParaLoguear[40];		//Los Puse de tamaño 40 para que alcance seguro
	char			ordenEnvio[40];
	tipo_enum_orden	tipoOrden;
} tipo_empaquetado;






/*	Funcion para Loguear(escribir) en el ARCHIVO_DE_LOG del JOB
			Nivles de Logueo Disponibles:
										LOG_LEVEL_TRACE
										LOG_LEVEL_DEBUG
										LOG_LEVEL_INFO
										LOG_LEVEL_WARNING
										LOG_LEVEL_ERROR
NOTA: Permite pasar argumentos %s al estilo Printf  */
void Servidor_loguear(t_log_level nivelDeLogueo, const char* nombreTareaFuncion, const char* textoPorLoguear, ... );


//Esta Funcion ademas de realizar el Logueo Imprime por Pantalla lo que Loguea
void Servidor_loguearPantalla(t_log_level nivelDeLogueo, const char* nombreTareaFuncion, const char* textoPorLoguear, ... );


bool esValidoArchivoConfig(t_config* archivoConfig);

//Dado los datos los empaqueta a un Tipo Unico para poder crear el thread orden y pasarselo como parametro
//Aca se genera el empaquetado para "threadOrden" y se cargan los Strings que necesita, segun cual sea la Orden pedida.
void Servidor_empaquetarYcrearThread( tipo_socket* socketConectadoAlCliente ,t_header headerRecibido ,char* rutina, tipo_enum_orden tipoDeOrden );


//Funcion para Crear threads para atender las Ordenes del Marta
//Ante Cualquier Problema le Envia al Marta que Fallo la Orden. Si todo sale bien le avisa que hubo terminacion correcta.
//Basicamente se encarga de Recibir las Ordenes/Peticiones del Marta y Reenviarlas al Nodo. Luego espera al nodo que le envie una respuesta de si se pudo ejecutar correctamente o no.
//NOTA: Como las 3 ordenes (Mapping y Reduce Con/Sin Combiner) son parcticamente iguales no valia la pena crear 3 funciones iguales solo porque cambian un par de strings, asi que decidi que esos strings se los pase el Empaquetado y ya.
void threadOrden(void* datosEmpaquetados);

//Para Debug
static void imprimir_Char(const char* string, const uint32_t tamanioChar);




//Inicializo un Mutex Global para los Logs, asi se puede loguear desde donde sea sin problemas.
pthread_mutex_t GLOBAL_mutex_interno_paraLogs = PTHREAD_MUTEX_INITIALIZER;









//Voy a Utilizar el Main como Servidor Principal del JOB
int main() {
	uint32_t iNumeroError = 0;		//Variable para Manejo de Errores. Por defecto es 0 que significa sin Errores
	t_config* archivoConfig = NULL;
	tipo_socket* socketConectadoAlMarta = NULL;
	t_list* listaArchivos = NULL;
	tipo_socket* socketEnEscucha = NULL;
	tipo_socket* socketConectadoAlCliente = NULL;
	t_header headerRecibido;

	Servidor_loguear(LOG_LEVEL_INFO, __func__, "----------------------------------------------------------");
	Servidor_loguear(LOG_LEVEL_INFO, __func__, "----------------------------------------------------------");
	Servidor_loguear(LOG_LEVEL_INFO , "main" , ">>Iniciando JOB...");
	Comun_Pantalla_Separador_Destacar("Iniciando JOB");



	Macro_ImprimirEstadoInicio("Viendo Si existe El archivo de Configuracion..");
	if(Comun_existeArchivo(RUTA_ARCHIVO_CONFIG_JOB) < 0 ){
		Macro_ImprimirEstadoFinal(EJECUCION_FALLIDA);
		printf(ANSI_COLOR_RED "No existe el Archivo de Configuracion" ANSI_COLOR_RESET "\n");
		return -1;
	}
	Macro_ImprimirEstadoFinal(EJECUCION_CORRECTA);



	//Abro Archivo de Configuracion
	archivoConfig = config_create(RUTA_ARCHIVO_CONFIG_JOB);



	//Verifico que tenga los Campos/Claves que necesitamos
	Macro_ImprimirEstadoInicio("Verificando Archivo de Configuracion..");
	if( esValidoArchivoConfig(archivoConfig) == false ){
		//No hace falta imprimir el "Macro_ImprimirEstadoFinal" ni Ningun mensaje, se encarga sola la funcion
		return -1;
	}
	Macro_ImprimirEstadoFinal(EJECUCION_CORRECTA);



	//Cargo en las Variables los Datos del Archivo de Configuracion
	char* 	job_PuertoListen 	= config_get_string_value(archivoConfig , "PUERTO_LISTEN");
	char* 	job_MartaIP 		= config_get_string_value(archivoConfig , "IP_MARTA");
	char* 	job_MartaPuerto 	= config_get_string_value(archivoConfig , "PUERTO_MARTA");
	char*	job_RutinaMapper 	= config_get_string_value(archivoConfig , "MAPPER");
	char*	job_RutinaReduce 	= config_get_string_value(archivoConfig , "REDUCE");
	char** 	job_Archivos 		= config_get_array_value(archivoConfig , "ARCHIVOS");
	char* 	job_Resultado 		= config_get_string_value(archivoConfig , "RESULTADO");
	char* combiner_string = config_get_string_value(archivoConfig , "COMBINER");
	bool 	job_UsaCombiner 	= string_equals_ignore_case("true", combiner_string);

	if(job_UsaCombiner==true){
		printf("- Soy JOB "  ANSI_COLOR_GREEN "CON" ANSI_COLOR_RESET " Combiner \n");
	}else{
		printf("- Soy JOB "  ANSI_COLOR_YELLOW "SIN" ANSI_COLOR_RESET " Combiner \n");
	}

	free(combiner_string);

	//Chequeo que halla almenos 1 archivo para usar y este bien el archivo final
	Macro_ImprimirEstadoInicio("Verificando Archivos sobre los que trabajar..");
	if(job_Archivos[0]==NULL ){
		Macro_ImprimirEstadoFinal(EJECUCION_FALLIDA);
		printf(ANSI_COLOR_RED "No hay Definidos Archivos sobre los Cuales trabajar" ANSI_COLOR_RESET "\n");
		return -1;
	}
	Macro_ImprimirEstadoFinal(EJECUCION_CORRECTA);


	Macro_ImprimirEstadoInicio("Verificando Archivo Final..");
	if (job_Resultado == NULL) {
		Macro_ImprimirEstadoFinal(EJECUCION_FALLIDA);
		printf(ANSI_COLOR_RED "No esta definido el Archivo Final" ANSI_COLOR_RESET "\n");
		return -1;
	}
	Macro_ImprimirEstadoFinal(EJECUCION_CORRECTA);

	//Para DEBUG Array de Chars. Con 2 archivos: en la posicion 2 hay NULL y en 3 es fuera de memoria
	//job_Archivos[0];
	//job_Archivos[1];
	//job_Archivos[2];
	//job_Archivos[3];



	//Valido que existan las Rutinas Mapper y Reduce
	Macro_ImprimirEstadoInicio("Viendo Si existen las Rutinas Mapper y Reduce..");

	if (Comun_existeArchivo(job_RutinaMapper) < 0) {
		Macro_ImprimirEstadoFinal(EJECUCION_FALLIDA);
		printf(ANSI_COLOR_RED "No existe la Rutina Mapper:'%s'" ANSI_COLOR_RESET "\n" , job_RutinaMapper);
		return -1;
	}

	if (Comun_existeArchivo(job_RutinaReduce) < 0) {
		Macro_ImprimirEstadoFinal(EJECUCION_FALLIDA);
		printf(ANSI_COLOR_RED "No existe la Rutina Reduce:'%s'" ANSI_COLOR_RESET "\n" , job_RutinaReduce);
		return -1;
	}

	Macro_ImprimirEstadoFinal(EJECUCION_CORRECTA);



	Comun_Pantalla_Separador_Destacar("Conectando a Marta");


	//Voy a ver si puedo conectarme al marta, en caso que No este Disponible: lo Logueo y espero unos segundos
	while (!Sockets_estaDisponibleServidor(job_MartaIP , job_MartaPuerto)) {
		Servidor_loguearPantalla(LOG_LEVEL_INFO , "main" , "- Aun no esta Disponible el Marta, espero %s segundos y reintento..." , SEGUNDOS_ESPERA_JOB);
		sleep( atoi(SEGUNDOS_ESPERA_JOB) );
	}
	Servidor_loguear(LOG_LEVEL_INFO , "main" , "El Marta esta Disponible!! Le mando la Peticion de Conexion al Marta");
	Comun_Pantalla_Separador_Destacar("Conectado al Marta");



	//Me conecto nuevamente al Marta
	socketConectadoAlMarta = Sockets_conectar_servidor(job_MartaIP , job_MartaPuerto);
	if(socketConectadoAlMarta == NULL ){
		printf(ANSI_COLOR_RED "Esto es Raro, Recien verifique que el Marta estaba disponible pero al conectarse acaba de caerse" ANSI_COLOR_RESET "\n");
		return -1;
	}


	//Armo la Peticion
	tipo_Datos_Conexion_Inicial peticion;
	strcpy( peticion.jobPuerto , job_PuertoListen);
	peticion.soportaCombiner = job_UsaCombiner;
	strcpy( peticion.rutaYnombreArchivoFinal , job_Resultado);

	//Serializo la Peticion.
	uint32_t tamanioSerializacion;
	char* peticionSerializada = Serializar_DatosConexionInicial(peticion, &tamanioSerializacion);


	//Serializo la Orden
	char* ordenSerializada = package_create(peticionSerializada, tamanioSerializacion, "JobNuevo", JOB);

	Comun_LiberarMemoria((void**)&peticionSerializada);

	//Envio la Orden al Marta
	iNumeroError = Sockets_enviar_datos(socketConectadoAlMarta,ordenSerializada);
	if(iNumeroError <= 0 ){
		printf(ANSI_COLOR_RED "No se pudo enviar la Orden Inicial al Marta" ANSI_COLOR_RESET "\n");
		return -1;
	}

	package_destroy(ordenSerializada);

	//Transformo de char** a Lista de las Commons
	listaArchivos = list_create();
	int archivoActual;
	for (archivoActual = 0; job_Archivos[archivoActual] != NULL; archivoActual++) {
		list_add(listaArchivos , job_Archivos[archivoActual]);
	}


	//Serializo la lista.
	char* listaSerializada = Serializar_listaArchivos(listaArchivos, &tamanioSerializacion);

	//Serializo la Orden
	ordenSerializada = package_create(listaSerializada, tamanioSerializacion, "archivosDeTrabajo", JOB);

	//Envio la Orden
	iNumeroError = Sockets_enviar_datos(socketConectadoAlMarta,ordenSerializada);
	if(iNumeroError <= 0 ){
		printf(ANSI_COLOR_RED "No se pudo enviar la Lista de Archivos al Marta" ANSI_COLOR_RESET "\n");
		return -1;
	}

	printf("- Me Acepto el Marta el JOB. Es cuestion de Tiempo que se Complete o De Error"  ANSI_COLOR_RESET "\n");		//Uso Reset por las dudas

	Comun_Pantalla_Separador_Destacar("Iniciando Servidor de Ordenes");

	//Inicio Servidor
	socketEnEscucha = Sockets_ponerme_escuchar(job_PuertoListen);
	if(socketEnEscucha==NULL){
		printf(ANSI_COLOR_RED "No puedo ponerme a Escuchar Peticiones de Ordenes" ANSI_COLOR_RESET "\n");
		return -1;
	}

	Servidor_loguear(LOG_LEVEL_INFO, __func__, "Servidor Iniciado \n");


	//Lo pongo debajo porque si el JOB se conecta a un Marta sin FS Conectado, me envia mas rapido el Marta la Orden EndJOb de lo que yo abro el Servidor
	package_destroy(ordenSerializada);

	Comun_LiberarMemoria((void**)&listaSerializada);

	//Libero la Memoria de la lista ya la destruyo
	list_destroy_and_destroy_elements(listaArchivos, (void*)&destructor_elementoListaArchivo);

	//Cierro Socket del Marta
	Sockets_cerrar_desconectar(socketConectadoAlMarta);


	//Acepto nuevos clientes infinitamente, bloqueandose hasta que se conecte un nuevo cliente
	//Solo para en Cuanto llegue una Orden EndJob
	bool llegoOrdenEndJob = false;
	while ( llegoOrdenEndJob == false) {
		Macro_ImprimirParaDebug("Esperando Nueva Orden del Marta...\n");

		//Acepto un nuevo cliente
		socketConectadoAlCliente = Sockets_aceptar_cliente(socketEnEscucha);
		//Chequeo Errores sin parar la ejecucion
		if (socketConectadoAlCliente == NULL) {
			Servidor_loguear(LOG_LEVEL_ERROR, "Servidor de Escucha", "Hubo un Problema al Aceptar un Cliente" );
			continue;
		}

		Servidor_loguear(LOG_LEVEL_TRACE, "Servidor de Escucha", "Se conecto alguien al JOB, su IP es: %s y su Puerto es: %d", Sockets_obtener_ip_cliente(socketConectadoAlCliente), Sockets_obtener_puerto_cliente(socketConectadoAlCliente) );


		//Aca vamos a esperar a que me llegue una peticion
		//Recibimos el header
		iNumeroError = Sockets_recibir_Header(socketConectadoAlCliente , &headerRecibido);
		//Chequeo Errores
		if (iNumeroError == 0) {
			Servidor_loguear(LOG_LEVEL_WARNING , "Servidor de Escucha" , "Se corto la conexion apenas recibimos al cliente, No tenemos manera de saber quien era asi que no podemos hacer nada");
			continue;

		} else if (iNumeroError < 0) {
			Servidor_loguear(LOG_LEVEL_ERROR , "Servidor de Escucha" , "(Linea %d) Hubo un problema con la conexion apenas recibimos al cliente, se imprimio por pantalla que paso (la funcion de Socket lo hizo)" , __LINE__);
			continue;
		}


		//Veo quien se me conecto y en base a eso voy a ver que orden/peticion me mandaron para atenderla
		switch (headerRecibido.sender_id) {
			//Veo si se me conecto El Marta
			case MARTA:
				Servidor_loguear(LOG_LEVEL_INFO, "Servidor de Escucha", "Llego una Orden '%s' del Marta", headerRecibido.order);

				if (header_esOrden(headerRecibido , "ejecutarMapping")) {
					printf("- Llego Orden Mapping de Marta \n");
					Servidor_empaquetarYcrearThread(socketConectadoAlCliente , headerRecibido, job_RutinaMapper, MAPPING);

				} else if (header_esOrden(headerRecibido , "ejecutarReduceConCombine")) {
					printf("- Llego Orden Reduce CON Combine de Marta \n");
					Servidor_empaquetarYcrearThread(socketConectadoAlCliente , headerRecibido, job_RutinaReduce, REDUCEconCOMBINER);

				} else if (header_esOrden(headerRecibido , "ejecutarReduceSinCombine")) {
					printf("- Llego Orden Reduce SIN Combine de Marta \n");
					Servidor_empaquetarYcrearThread(socketConectadoAlCliente , headerRecibido, job_RutinaReduce, REDUCEsinCOMBINER);

				} else if (header_esOrden(headerRecibido , "EndJob")) {
					printf("- Llego Orden EndJob de Marta \n");
					llegoOrdenEndJob = true; 			//Hago que salga del While Infinito para atender esta Orden y que pare todo

				} else {
					Servidor_loguear(LOG_LEVEL_WARNING , "Servidor de Escucha" , "Llego una orden del Marta que no se la reconoce, la orden decia: %s" , headerRecibido.order);
				}
				break;
			default:
				Servidor_loguear(LOG_LEVEL_WARNING , "Servidor de Escucha" , "Llego una Orden de alguien que no reconosco, quien mando: %d" , headerRecibido.sender_id);
				break;
		}//Fin Switch de Seleccion

	}//While Infinito

	//Solo se ejecutara aca abajo en caso de que llegue la Orden "EndJob", sino nunca llega aca debajo.
	Servidor_loguear(LOG_LEVEL_INFO, "EndJOB", "Iniciando Orden EndJob" );

	Comun_Pantalla_Separador_Destacar("JOB Terminado");

	//Tengo que recibir el Payload que es la Confirmacion de Terminacion Correcta o NO.
	char* confirmarcion;
	iNumeroError = Sockets_recibir_Datos(socketConectadoAlCliente, &confirmarcion,  headerRecibido);
	if(iNumeroError <= 0){
		printf(ANSI_COLOR_RED "No se pudo recibir la Razon/Confirmacion del EndJob" ANSI_COLOR_RESET "\n");
		return -1;
	}

	Servidor_loguear(LOG_LEVEL_TRACE, "EndJOB", "La razon del EndJob que llego es:'%s'", confirmarcion );

	if( string_equals_ignore_case(confirmarcion, "Terminacion Correcta") ){
		printf(ANSI_COLOR_GREEN "El JOB se completo Exitosamente\n"  ANSI_COLOR_RESET);
		printf("El archivo resultado YA ha sido subido al File System en '%s'\n", job_Resultado);

	}else{
		printf(ANSI_COLOR_YELLOW "El JOB se Termino con Errores"  ANSI_COLOR_RESET "\n");
		printf("Razon: %s\n", confirmarcion);
	}


	Servidor_loguearPantalla(LOG_LEVEL_INFO, "EndJOB", "Fin Orden EndJob. Se apago el JOB" );
	//Se termina el JOB, se termina el proceso Main y con el todos sus Hilos y se libera la memoria utilizada.
	return 0;



/*
Error_Handler:
	//Aca no hay que hacer gran cosa porque debido a que es el Hilo/Proceso Main, ante cualquier problema se cierra y se liberan todos los recursos.
	printf(ANSI_COLOR_RED "Debido a un Fallo Critico el JOB debio Cerrarse.. \n"  ANSI_COLOR_RESET);
	return -1;
*/
}





void Servidor_loguear(t_log_level nivelDeLogueo, const char* nombreTareaFuncion, const char* textoPorLoguear, ... ){
	//Necesito juntar en un Unico String todo los char que se le pasen, sino no puedo loguear
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
	free(textoJuntadoParaLoguear);

	//Libero la Memoria y dejo de escribir en el Log
	log_destroy(miLog);

	//Fin Mutex
	pthread_mutex_unlock(&GLOBAL_mutex_interno_paraLogs);
}



void Servidor_loguearPantalla(t_log_level nivelDeLogueo, const char* nombreTareaFuncion, const char* textoPorLoguear, ... ){
	//Necesito juntar en un Unico String todo los char que se le pasen, sino no puedo loguear
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
	free(textoJuntadoParaLoguear);

	//Libero la Memoria y dejo de escribir en el Log
	log_destroy(miLog);

	//Fin Mutex
	pthread_mutex_unlock(&GLOBAL_mutex_interno_paraLogs);
}



bool esValidoArchivoConfig(t_config* archivoConfig){

	if (config_has_property(archivoConfig , "PUERTO_LISTEN") == false) {
		Macro_ImprimirEstadoFinal(EJECUCION_FALLIDA);
		printf(ANSI_COLOR_RED "El archivo Config No tiene el Campo PUERTO_LISTEN" ANSI_COLOR_RESET "\n");
		return false;
	}

	if (config_has_property(archivoConfig , "IP_MARTA") == false) {
		Macro_ImprimirEstadoFinal(EJECUCION_FALLIDA);
		printf(ANSI_COLOR_RED "El archivo Config No tiene el Campo IP_MARTA" ANSI_COLOR_RESET "\n");
		return false;
	}

	if (config_has_property(archivoConfig , "PUERTO_MARTA") == false) {
		Macro_ImprimirEstadoFinal(EJECUCION_FALLIDA);
		printf(ANSI_COLOR_RED "El archivo Config No tiene el Campo PUERTO_MARTA" ANSI_COLOR_RESET "\n");
		return false;
	}

	if (config_has_property(archivoConfig , "MAPPER") == false) {
		Macro_ImprimirEstadoFinal(EJECUCION_FALLIDA);
		printf(ANSI_COLOR_RED "El archivo Config No tiene el Campo MAPPER" ANSI_COLOR_RESET "\n");
		return false;
	}

	if (config_has_property(archivoConfig , "REDUCE") == false) {
		Macro_ImprimirEstadoFinal(EJECUCION_FALLIDA);
		printf(ANSI_COLOR_RED "El archivo Config No tiene el Campo REDUCE" ANSI_COLOR_RESET "\n");
		return false;
	}

	if (config_has_property(archivoConfig , "COMBINER") == false) {
		Macro_ImprimirEstadoFinal(EJECUCION_FALLIDA);
		printf(ANSI_COLOR_RED "El archivo Config No tiene el Campo COMBINER" ANSI_COLOR_RESET "\n");
		return false;
	}

	if (config_has_property(archivoConfig , "ARCHIVOS") == false) {
		Macro_ImprimirEstadoFinal(EJECUCION_FALLIDA);
		printf(ANSI_COLOR_RED "El archivo Config No tiene el Campo ARCHIVOS" ANSI_COLOR_RESET "\n");
		return false;
	}

	if (config_has_property(archivoConfig , "RESULTADO") == false) {
		Macro_ImprimirEstadoFinal(EJECUCION_FALLIDA);
		printf(ANSI_COLOR_RED "El archivo Config No tiene el Campo RESULTADO" ANSI_COLOR_RESET "\n");
		return false;
	}


	//Llegados hasta Aca es valido
	return true;
}




void Servidor_empaquetarYcrearThread( tipo_socket* socketConectadoAlCliente ,t_header headerRecibido ,char* rutina, tipo_enum_orden tipoDeOrden ){
	int iNumeroError = 0;		//Variable para Manejo de Errores. Por defecto es 0 que significa sin Errores

	//Primero Empaquetamos
	tipo_empaquetado* datosEmpaquetados = malloc( sizeof(tipo_empaquetado) );
	datosEmpaquetados->socketConectadoAlCliente = socketConectadoAlCliente;
	datosEmpaquetados->headerRecibido = headerRecibido;
	strcpy(datosEmpaquetados->rutina , rutina);
	datosEmpaquetados->tipoOrden = tipoDeOrden;

	//Como practicamente las 3 ordenes son iguales a excepcion por lo que deben Loguear y por la orden de envio.
	//Uso el empaquetado para pasarlas a una unica funcion "threadOrden" y no tener 3 funciones iguales que solo cambien un par de Strings
	switch(tipoDeOrden){
		case MAPPING:
			strcpy(datosEmpaquetados->tipoOrdenParaLoguear , "threadOrdenMapping");
			strcpy(datosEmpaquetados->ordenEnvio , "ejecutarMapping");
			break;

		case REDUCEconCOMBINER:
			strcpy(datosEmpaquetados->tipoOrdenParaLoguear , "threadOrdenReduceConCombiner");
			strcpy(datosEmpaquetados->ordenEnvio , "ejecutarReduceConCombine");
			break;

		case REDUCEsinCOMBINER:
			strcpy(datosEmpaquetados->tipoOrdenParaLoguear , "threadOrdenReduceSinCombiner");
			strcpy(datosEmpaquetados->ordenEnvio , "ejecutarReduceSinCombine");
			break;

		default:
			Macro_Check_And_Handle_Error( true, "Llego Cualquier Cosa al Switch del Tipo de Orden");
	}


	//Creo una variable para el ID del Thread, no nos importa realmente que se pierda el ID de los Threads Hijos
	pthread_t idThread;

	//Ahora Creamos el Thread que manejara la Orden
	iNumeroError = pthread_create(&idThread , NULL , (void*) &threadOrden , (void*) datosEmpaquetados);
	Macro_Check_And_Handle_Error(iNumeroError != 0, "Ocurrio un Error al Crear el Thread para la Orden. Ver ERRNO");

	//Le digo al Thread que cuando se termine libere los recursos que uso, lo hace el solo y nos olvidamos nosotros
	iNumeroError = pthread_detach(idThread);
	Macro_Check_And_Handle_Error(iNumeroError != 0, "Ocurrio un Error al hacer el Detach del Thread para la Orden. Ver ERRNO");

	return;



Error_Handler:
	Servidor_loguear(LOG_LEVEL_ERROR, "Servidor_empaquetarYcrearThread", "No se pudo crear el Thread correctamente o no se pudo hacer el Detach y esta generando Memory Leaks");

	//En caso de Error hay que cerrar el Socket, porque ya nadie lo usara mas
	Sockets_cerrar_desconectar(socketConectadoAlCliente);
	return;
}



void threadOrden(void* datosEmpaquetados){
	uint32_t 		iNumeroError = 0;				//Variable para Manejo de Errores. Por defecto es 0 que significa sin Errores
	char* 			datosSerializados = NULL;
	tipo_socket* 	socketConectadoAlNodo = NULL;
	char* 			datosSerializadosConHeader = NULL;
	tipo_bloque* 	bloqueLeidoRutina = NULL;
	char*			bloqueRutinaSerializada = NULL;
	char*			bloqueRutinaSerializadaConHeader = NULL;
	char* 			confirmacion = NULL;
	char*			respuestaAlMarta = NULL;
	char*			respuestaAlMartaConHeader = NULL;
	tipo_Datos_RespuestaMarta respuestaMarta;
	uint32_t 		tamanioSerializacion = -1;
	char* 			nodoIP = NULL;
	char* 			nodoPuerto = NULL;
	bool			estabaConNodo = false;
	bool			seDesconectoNodo = false;

	//Des-Empaqueto los Datos y los Copio a Variables Locales, asi evitamos problemas de Sincronizacion
	tipo_socket* 	socketConectadoAlMarta 	= ( (tipo_empaquetado*)datosEmpaquetados )->socketConectadoAlCliente ;
	t_header 		headerRecibido 			= ( (tipo_empaquetado*)datosEmpaquetados )->headerRecibido;
	char* 			rutina 					= string_duplicate( ( (tipo_empaquetado*)datosEmpaquetados )->rutina );
	char*			tipoOrdenParaLoguear	= string_duplicate( ( (tipo_empaquetado*)datosEmpaquetados )->tipoOrdenParaLoguear );
	char*			ordenEnvio				= string_duplicate( ( (tipo_empaquetado*)datosEmpaquetados )->ordenEnvio );
	tipo_enum_orden	tipoDeOrden				= ( (tipo_empaquetado*)datosEmpaquetados )->tipoOrden;

	//free( (tipo_empaquetado*)datosEmpaquetados );


	Servidor_loguear(LOG_LEVEL_INFO, tipoOrdenParaLoguear, "Iniciando Orden");


	//Recibo la Ip y Puerto del Nodo con quien debo reenviar la Orden
	Servidor_loguear(LOG_LEVEL_TRACE, tipoOrdenParaLoguear, "Recibiendo la Ip y Puerto del Nodo con quien debo reenviar la Orden");
	iNumeroError = Sockets_recibir_Datos(socketConectadoAlMarta, &datosSerializados, headerRecibido);
	Macro_Check_And_Handle_Error(iNumeroError <= 0, "Problema al Recibir IP y Puerto del Marta");


	//Des-Serializo los Datos de Nodo
	DesSerializar_DatosNodo(datosSerializados, &nodoIP, &nodoPuerto);

	Comun_LiberarMemoria((void**) &datosSerializados);

	Servidor_loguear(LOG_LEVEL_INFO, tipoOrdenParaLoguear, "Pidieron Trabajar Sobre el Nodo de IP: %s Puerto: %s", nodoIP, nodoPuerto);

	//Recibo el header de los Datos de la Orden
	t_header headerDatosOrden;
	iNumeroError = Sockets_recibir_Header(socketConectadoAlMarta, &headerDatosOrden);
	Macro_Check_And_Handle_Error(iNumeroError <= 0, "Problema al Recibir Header de los Datos de la Orden")


	//Recibo los Datos de la Orden
	Servidor_loguear(LOG_LEVEL_TRACE, tipoOrdenParaLoguear, "Recibiendo los Datos de la Orden");
	Macro_ImprimirParaDebug("\nRecibiendo los Datos de la Orden \n");
	iNumeroError = Sockets_recibir_Datos(socketConectadoAlMarta, &datosSerializados, headerDatosOrden);
	Macro_Check_And_Handle_Error(iNumeroError <= 0, "Problema al Recibir los Datos de la Orden");


	//Para Debug
	Macro_ImprimirParaDebug("Header Size:'%ld' \n", (long int)headerDatosOrden.payload_size);
	imprimir_Char(datosSerializados, headerDatosOrden.payload_size);

	//Me Conecto al Nodo
	estabaConNodo = true;
	Servidor_loguear(LOG_LEVEL_TRACE, tipoOrdenParaLoguear, "Conectando al Nodo");
	socketConectadoAlNodo = Sockets_conectar_servidor(nodoIP , nodoPuerto);
	if (socketConectadoAlNodo == NULL) {
		seDesconectoNodo = true;
		Macro_Check_And_Handle_Error(true, "Problema al Conectar al Nodo");
		//No hace falta return, va seguro al Error_Handler
	}



	//Reempaqueto los Datos de la Orden (payload) con un Nuevo Header
	datosSerializadosConHeader = package_create(datosSerializados, headerDatosOrden.payload_size, ordenEnvio, JOB);



	//Reenvio los Datos de la Orden al Nodo
	Servidor_loguear(LOG_LEVEL_TRACE, tipoOrdenParaLoguear, "Reenviando Datos de la Orden al Nodo");
	iNumeroError = Sockets_enviar_datos(socketConectadoAlNodo, datosSerializadosConHeader);
	if(iNumeroError == 0){
		seDesconectoNodo = true;
	}
	Macro_Check_And_Handle_Error(iNumeroError <= 0, "Problema al Reenviar los Datos de la Orden al Nodo");

	Comun_LiberarMemoria((void**) &datosSerializados);
	Comun_LiberarMemoria((void**) &datosSerializadosConHeader);



	//Leo la Rutina y la Mapeo a Memoria
	uint32_t PosicionLecturaArchivo = UNICO_BLOQUE_RUTINA;
	bloqueLeidoRutina = Bloques_obtener_desde_archivo_texto(rutina, &PosicionLecturaArchivo);
	Macro_Check_And_Handle_Error(bloqueLeidoRutina == NULL, "Problema al leer la Rutina")


	bloqueRutinaSerializada = Bloques_serializar(bloqueLeidoRutina, &tamanioSerializacion);
	Comun_LiberarMemoria((void**) &bloqueLeidoRutina);



	//Armo Paquete  a enviar
	bloqueRutinaSerializadaConHeader = package_create(bloqueRutinaSerializada, tamanioSerializacion, "envioRutina", JOB);

	//Envio mi Rutina
	Servidor_loguear(LOG_LEVEL_TRACE, tipoOrdenParaLoguear, "Enviando Rutina al Nodo");
	iNumeroError = Sockets_enviar_datos(socketConectadoAlNodo, bloqueRutinaSerializadaConHeader);
	if(iNumeroError == 0){
		seDesconectoNodo = true;
	}
	Macro_Check_And_Handle_Error(iNumeroError <= 0, "Problema al Enviar la Rutina");
	Comun_LiberarMemoria((void**) &bloqueRutinaSerializada);
	Comun_LiberarMemoria((void**) &bloqueRutinaSerializadaConHeader);



	//Espero Confirmacion del Nodo
	Servidor_loguear(LOG_LEVEL_TRACE, tipoOrdenParaLoguear, "Esperando Confirmacion del Nodo");
	t_header headerConfirmacion;

	//Voy a usar un Select para que Halla un Time Out del tiempo de espera de ejecucion de la Rutina
	tipo_select select;
	Sockets_Select_preparar(&select);
	Sockets_Select_agregar( socketConectadoAlNodo, &select);

	iNumeroError = Sockets_Select_esperarEnvios(  socketConectadoAlNodo, &select, TIEMPO_ESPERA_EJECUCION_RUTINAS, 0 );
	Macro_Check_And_Handle_Error(iNumeroError == -1, "Problema con el Select de esperar Respuesta de Ejecucion Rutina");

	Servidor_loguear(LOG_LEVEL_TRACE, __func__, "Terminado Tiempo Espera del Select(%d Segundos) o Simplemente Recibi algo antes del tiempo de espera", TIEMPO_ESPERA_EJECUCION_RUTINAS);

	if( Sockets_Select_enviaronAlgo(socketConectadoAlNodo, &select)==0 ){
		Servidor_loguear(LOG_LEVEL_ERROR, __func__, "No enviaron Nada y se paso el Time Out de %d Segundos", TIEMPO_ESPERA_EJECUCION_RUTINAS);
		seDesconectoNodo = true;

		Macro_Check_And_Handle_Error( true, "No enviaron Nada y se paso el Time Out de %d Segundos", TIEMPO_ESPERA_EJECUCION_RUTINAS);
		//No hace falta Return porque va siempre al Error_Handler
	}

	//Solo llego aca cuando Llego una Confirmacion
	iNumeroError = Sockets_recibir_Header(socketConectadoAlNodo, &headerConfirmacion);
	if(iNumeroError == 0){
		seDesconectoNodo = true;	//Esto es Porque Puede pasar que el CheckSum de TCPIP Descarte el Paquete ya que llego Corrompido y el Nodo esta enviando las cosas Mal. En ese caso lo doy como caido porque se jodio la conexion de Red de ese nodo
	}
	Macro_Check_And_Handle_Error(iNumeroError <= 0, "Problema al Recibir Header de la Confimacion");

	iNumeroError = Sockets_recibir_Datos(socketConectadoAlNodo, &confirmacion, headerConfirmacion);
	if(iNumeroError == 0){
		seDesconectoNodo = true;
	}
	Macro_Check_And_Handle_Error(iNumeroError <= 0, "Problema al Recibir Header de la Confimacion");
	Sockets_cerrar_desconectar(socketConectadoAlNodo);		//Cierro Ahora porque no se usa mas
	estabaConNodo = false;

	Servidor_loguear(LOG_LEVEL_TRACE, tipoOrdenParaLoguear, "Confirmacion Recibida: %s. Analizo si La rutina termino correctamente", confirmacion);
	Comun_LiberarMemoria((void**) &confirmacion);		//No la Uso para nada


	//Preparo la Respuesta al Marta
	strcpy(respuestaMarta.nodoIP , nodoIP);
	strcpy(respuestaMarta.nodoPuerto , nodoPuerto);
	respuestaMarta.tipoDeOrden = tipoDeOrden;


	//Ahora veo si se ejecuto Correctamente o No la Rutina, para preparar la Respuesta al Marta
	if(header_esOrden(headerConfirmacion, "rutinaExitosa")){
		Servidor_loguear(LOG_LEVEL_TRACE, tipoOrdenParaLoguear, "Se termino correctamente la Rutina. Aviso al Marta");
		respuestaMarta.terminacionCorrecta = TERMINACION_CORRECTA;

	}else{
		Servidor_loguear(LOG_LEVEL_TRACE, tipoOrdenParaLoguear, "NO se termino correctamente la Rutina. Aviso al Marta que Fallo");
		respuestaMarta.terminacionCorrecta = TERMINACION_FALLIDA;
	}

	strcpy(respuestaMarta.nodoIP , nodoIP);
	strcpy(respuestaMarta.nodoPuerto , nodoPuerto);
	respuestaMarta.tipoDeOrden = tipoDeOrden;
	Servidor_loguear(LOG_LEVEL_TRACE, tipoOrdenParaLoguear, "Se termino la Rutina (correctamente o no). Aviso al Marta");


	respuestaAlMarta = Serializar_RespuestaMarta(respuestaMarta, &tamanioSerializacion);

	respuestaAlMartaConHeader = package_create(respuestaAlMarta, tamanioSerializacion, "TerminacionRutina", JOB);


	//Le aviso al Marta  el resultado de la Rutina
	iNumeroError = Sockets_enviar_datos(socketConectadoAlMarta, respuestaAlMartaConHeader);
	Macro_Check_And_Handle_Error(iNumeroError <= 0, "Problema al Enviar al Marta el Resultado de la Rutina");
	Comun_LiberarMemoria((void**) &respuestaAlMarta);
	Comun_LiberarMemoria((void**) &respuestaAlMartaConHeader);



	Servidor_loguear(LOG_LEVEL_INFO, tipoOrdenParaLoguear, "Orden Finalizada Correctamente \n");

	//Cierro el Socket y Libero la Memoria que faltaba liberar
	Sockets_cerrar_desconectar(socketConectadoAlMarta);
	Comun_LiberarMemoria((void**) &rutina);
	Comun_LiberarMemoria((void**) &tipoOrdenParaLoguear);
	Comun_LiberarMemoria((void**) &ordenEnvio);
	Comun_LiberarMemoria((void**) &nodoIP);
	Comun_LiberarMemoria((void**) &nodoPuerto);

	pthread_exit(EXIT_SUCCESS);




Error_Handler:
	Servidor_loguear(LOG_LEVEL_ERROR, tipoOrdenParaLoguear, "Hubo un problema en la Rutina. Procedo a Avisarle al Marta que Fallo");

	//Ante Cualquier Problema, lo Primero que hago es avisarle al Marta que fallo la Orden
	//Primero Preparo la Orden
	strcpy(respuestaMarta.nodoIP , nodoIP);
	strcpy(respuestaMarta.nodoPuerto , nodoPuerto);
	respuestaMarta.tipoDeOrden = tipoDeOrden;

	if(seDesconectoNodo == true){
		respuestaMarta.terminacionCorrecta = NODO_DESCONECTADO;
	}else{
		respuestaMarta.terminacionCorrecta = TERMINACION_FALLIDA;
	}

	Comun_LiberarMemoria((void**) &respuestaAlMarta);	//Libero la Memoria por si venia con algo asociado previamiente
	respuestaAlMarta = Serializar_RespuestaMarta(respuestaMarta, &tamanioSerializacion);

	char* ordenFallida = package_create(respuestaAlMarta, tamanioSerializacion, "TerminacionRutina", JOB);

	//Solo envio por aca al Marta si es que estaba con el Nodo, sino significa que hubo problema con el Marta
	if( estabaConNodo == true ){
		iNumeroError = Sockets_enviar_datos(socketConectadoAlMarta, ordenFallida);
		if (iNumeroError <= 0) {
			Servidor_loguear(LOG_LEVEL_ERROR, tipoOrdenParaLoguear, "No pude avisarle al Marta que fallo la Rutina \n");
		}else{
			Servidor_loguear(LOG_LEVEL_INFO, tipoOrdenParaLoguear, "Se le Aviso al Marta que Fallo la Rutina \n");
			Sockets_cerrar_desconectar(socketConectadoAlMarta);
		}
	}else{
		Servidor_loguear(LOG_LEVEL_ERROR, tipoOrdenParaLoguear, "No pude avisarle al Marta Porque el Marta Mismo Fallo \n");
	}

	package_destroy(ordenFallida);


	// Libero la Memoria Utilizada
	Comun_LiberarMemoria((void**) &rutina);
	Comun_LiberarMemoria((void**) &tipoOrdenParaLoguear);
	Comun_LiberarMemoria((void**) &ordenEnvio);
	Comun_LiberarMemoria((void**) &nodoIP);
	Comun_LiberarMemoria((void**) &nodoPuerto);
	Comun_LiberarMemoria((void**) &datosSerializados);
	Comun_LiberarMemoria((void**) &datosSerializadosConHeader);
	Comun_LiberarMemoria((void**) &bloqueLeidoRutina);
	Comun_LiberarMemoria((void**) &bloqueRutinaSerializada);
	Comun_LiberarMemoria((void**) &bloqueRutinaSerializadaConHeader);
	Comun_LiberarMemoria((void**) &confirmacion);
	Comun_LiberarMemoria((void**) &respuestaAlMarta);
	Comun_LiberarMemoria((void**) &respuestaAlMartaConHeader);

	pthread_exit( (void*)EXIT_FAILURE);
}

static void imprimir_Char(const char* string, const uint32_t tamanioChar){
	uint32_t cantidadCaracteresPorImprimir = 0;
	Macro_ImprimirParaDebug("El String es: ");
	fflush(stdout);


	cantidadCaracteresPorImprimir = tamanioChar;

	//Voy a imprimir caracter por caracter el payload del string que me pasaron
	uint32_t iContadorCaracter;
	for (iContadorCaracter = 0; iContadorCaracter < cantidadCaracteresPorImprimir; iContadorCaracter++) {
		//Probar con <= a ver que pasa

		//Como no puedo imprimir \0 porque Buguea el STDIN, imprimo el \0 como literal
		if (string[iContadorCaracter] == '\0') {
			Macro_ImprimirParaDebug("\\0");
			fflush(stdout);
		} else if (string[iContadorCaracter] == '\n'){
			Macro_ImprimirParaDebug("\\n");
			fflush(stdout);
		}else {
			Macro_ImprimirParaDebug("%c" , (char) string[iContadorCaracter]);
			fflush(stdout);
		}
	}

	//Para evitar Problemas con Futuros Printf
	Macro_ImprimirParaDebug("\n");
	return;
}
