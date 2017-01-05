#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <libgen.h> //Para el "basename" y "dirname"
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h> // Para open y Close
#include <sys/mman.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <commons/log.h>
#include <assert.h>

#include "../../Serializador/Protocolo_Marta_JOB_Nodo.h"
#include "../../Biblioteca_Comun/Biblioteca_Comun.h"
#include "../../FileSystem/headers/Biblioteca_Bloques.h"
#include "../../Sockets/Biblioteca_Sockets.h"
#include "../../Serializador/serialized_package.h"
#include "../../Serializador/Protocolo_Nodos_FS.h"

#include "bibliotecaNodo.h"

extern t_nodo_config GLOBAL_Nodo_Config;


//Método que realiza la conexión con el fs
int conectarFileSystem(int numero_bloques);

//Método principal del servidor, espera conexiones y lanza threads de threadCreadoPorCliente
void threadBaseServidor(char* sPuertoEscucha);

//Método que maneja el flujo a realizar al recibir una nueva conexión en threadBaseServidor
void threadCreadoPorCliente(void* socket);


//Rutina para Eliminar Procesos Zombies (se encarga automaticamente)
void Handler_Eliminar_Zombies(int valorQueNoUso);


//Ordenes Externas que debe Atender el Nodo
void ordenPedirArchivo(tipo_socket* socket, t_header header);
void ordenCopiarBloqueANodo(tipo_socket* socketConectadoFS, t_header header);
void ordenPedirBloque(tipo_socket* socket, t_header header);
void ordenEnvioUbicacionBloqueParaGuardar(tipo_socket* socket, t_header header);
void ordenEjecutarReduceSinCombine(tipo_socket* socket, t_header header);
void ordenEjecutarReduceConCombine(tipo_socket* socket, t_header header);
void ordenEjecutarMapping(tipo_socket* socketConectadoAlJob, t_header header);
void ordenPedirBloqueMD5(tipo_socket* socketConectadoFS, t_header header);

//solicita un archivo a otro nodo en base a los datos ingresados por parámetro
int solicitarArchivoANodo(tipo_nodoExterno* nodo_externo);

//Dado un Bloque lo Escribe en el Archivo Indicado (sea o no el archivo BIN, por eso el Parametro, para Reutilizarlo), Si no existe lo Crea. Devuelve 0 en ejecucion Correcta y -1 en Ejecucion Fallida
int Archivo_EscribirBloque(const char* rutaArchivo, uint32_t numeroBloqueDondeEscribir , tipo_bloque* bloquePorEscribir);

//Dado un Bloque lo escribe en el Archivo Bin.
//Internamente Usa Mutex para Evitar Problemas de Sincronizacion
//Devuelve 0 en ejecucion Correcta y -1 en Ejecucion Fallida
int ArchivoBIN_EscribirBloque(uint32_t numeroBloqueDondeEscribir , tipo_bloque* bloquePorEscribir);

//Dado un Bloque lo Lee del Archivo Bin y lo guarda en "bloqueLeido" (deben pasarlo por Referencia y estar Inicializado a NULL)
//Internamente Usa Mutex para Evitar Problemas de Sincronizacion
//Devuelve 0 en ejecucion Correcta y -1 en Ejecucion Fallida
int ArchivoBIN_LeerBloque(uint32_t numeroBloqueALeer , tipo_bloque** bloqueLeido, size_t tamanioBloque);

//Imprime elementos char de una lista
void imprimirListaDeArchivos(t_list* lista, char* nombreFuncion, char* nombreLista);

//Inicializa el mutex para el archivo bin
int initBinMutex();

//La configuracion del nodo para que la puedan utilizar todos los threads creados
//t_nodo_config GLOBAL_Nodo_Config;

//Read-Write Lock para Archivo BIN
pthread_mutex_t *GLOBAL_LockArchivoBIN = NULL;

int main(int argc, char **argv) {

	uint32_t resultado = 0;

	Comun_Pantalla_Separador_Destacar("Iniciando Nodo");

	resultado = initLogMutex();

	if(EXIT_ERROR == resultado) {
		Macro_ImprimirEstadoFinal(EJECUCION_FALLIDA);
		printf( ANSI_COLOR_RED "Nodo - Error al inicializar mutex para logs" ANSI_COLOR_RESET "\n");
		return EXIT_ERROR;
	}

	//levanto mutex para logs
	Macro_ImprimirEstadoInicio("Inicializando Mutex para logs");

	loguear(LOG_LEVEL_INFO, __func__, "----------------------------------------------------------");
	loguear(LOG_LEVEL_INFO, __func__, "----------------------------------------------------------");
	loguear(LOG_LEVEL_INFO, __func__, ">>Iniciando Nodo");

	loguearPantalla(LOG_LEVEL_TRACE , __func__ , "Nodo - Process ID: %d", getpid());

	//levanto mutex para bin
	Macro_ImprimirEstadoInicio("Inicializando Mutex para logs");

	resultado = initBinMutex();

	if(EXIT_ERROR == resultado) {
		Macro_ImprimirEstadoFinal(EJECUCION_FALLIDA);
		printf( ANSI_COLOR_RED "Nodo - Error al inicializar mutex para bin" ANSI_COLOR_RESET "\n");
		return EXIT_ERROR;
	}

	//leo archivo de config
	Macro_ImprimirEstadoInicio("Verificando Archivo Config");

	resultado = leerArchivoConfig(&GLOBAL_Nodo_Config);

	if(EXIT_ERROR == resultado) {
		Macro_ImprimirEstadoFinal(EJECUCION_FALLIDA);
		printf( ANSI_COLOR_RED "Nodo - Error al leer archivo de config (%s)" ANSI_COLOR_RESET "\n", RUTA_ARCHIVO_CONFIG);
		return EXIT_ERROR;
	}

	Macro_ImprimirEstadoFinal(EJECUCION_CORRECTA);

	//veo si existe el bin
	Macro_ImprimirEstadoInicio("Verificando si existe archivo BIN");
	resultado = Comun_existeArchivo(GLOBAL_Nodo_Config.nombre_bin);
	if(EXIT_ERROR == resultado) {
		Macro_ImprimirEstadoFinal(EJECUCION_FALLIDA);
		printf( ANSI_COLOR_RED "Nodo - no existe el archivo bin (%s)"ANSI_COLOR_RESET "\n", GLOBAL_Nodo_Config.nombre_bin);
		return EXIT_ERROR;
	}
	Macro_ImprimirEstadoFinal(EJECUCION_CORRECTA);

	//calculo la cantidad de bloques del bin
	Macro_ImprimirEstadoInicio("Calculando cantidad bloques archivo BIN");
	uint32_t numero_bloques = calcularCantidadBloques(GLOBAL_Nodo_Config.nombre_bin);
	if(EXIT_ERROR == numero_bloques) {
		Macro_ImprimirEstadoFinal(EJECUCION_FALLIDA);
		printf( ANSI_COLOR_RED "Nodo - error al calcular cantidad de bloques (%s)" ANSI_COLOR_RESET "\n", GLOBAL_Nodo_Config.nombre_bin);
		return EXIT_ERROR;
	}
	Macro_ImprimirEstadoFinal(EJECUCION_CORRECTA);

	loguearPantalla(LOG_LEVEL_TRACE , __func__ , "Nodo - cantidad de bloques en el archivo bin: %d", numero_bloques);

	//conecto con filesystem
	printf("- Conectando al File System\n");
	if(EXIT_ERROR == conectarFileSystem(numero_bloques)) {
		//En realidad como la funcion de Arriba Tiene muchos Whiles nunca llega aca, osea nunca da error
		printf( ANSI_COLOR_RED "Nodo - Error al establecer conexión con el File System en IP:'%s' Puerto:'%d'" ANSI_COLOR_RESET "\n", GLOBAL_Nodo_Config.ip_fs, GLOBAL_Nodo_Config.puerto_fs);
		return EXIT_ERROR;
	}

	Comun_Pantalla_Separador_Destacar("Conexión establecida con FileSystem");

	// Inicializo la variable del mutex del archivo bin
	Macro_ImprimirEstadoInicio("Inicializando mutex archivo BIN");
	resultado = pthread_rwlock_init(GLOBAL_LockArchivoBIN, NULL);
	if(EXIT_ERROR == resultado) {
		Macro_ImprimirEstadoFinal(EJECUCION_FALLIDA);
		printf( ANSI_COLOR_RED "Nodo - error al inicializar lock" ANSI_COLOR_RESET "\n");
		return EXIT_ERROR;
	}
	Macro_ImprimirEstadoFinal(EJECUCION_CORRECTA);

	//quedo a la escucha de conexiones indefinidamente...
	threadBaseServidor(string_itoa(GLOBAL_Nodo_Config.puerto_nodo));

	//Nunca llega aca abajo y eso esta bien. Tampoco es Necesario

	return 0;
}

int conectarFileSystem(int numero_bloques) {

	int iNumeroError = 0;
	tipo_socket* socketFileSystem = NULL;

	bool conexionConfirmada = false;
	t_header headerRecibido;
	char* confirmacion;

	//me pude conectar, envío datos del nodo
	t_datos_nodo datosEsteNodo;
	strcpy(datosEsteNodo.nombre_nodo, GLOBAL_Nodo_Config.nombre_nodo);
	strcpy(datosEsteNodo.ip, GLOBAL_Nodo_Config.ip_nodo);
	datosEsteNodo.puerto = GLOBAL_Nodo_Config.puerto_nodo;
	datosEsteNodo.numero_bloques = numero_bloques;

	//loguear(LOG_LEVEL_DEBUG, "threadNodo", "Datos nodo a enviar: %s, %s, %s, %s", datosEsteNodo.nombre_nodo, datosEsteNodo.numero_bloques, datosEsteNodo.ip, datosEsteNodo.puerto);

	//Serializo el Payload de la orden
	char* payloadAvisarNodoConectado = serializar_datos_nodo(datosEsteNodo);

	//Armo la Orden
	char* ordenAvisarNodoConectado = package_create(payloadAvisarNodoConectado, sizeof(t_datos_nodo), "avisarNodoConectado" , NODO);

	//envío datos de nodo para confirmar la conexión hasta ser aceptada.
	while(conexionConfirmada == false) {

		socketFileSystem = NULL;

		//intento conectar al fs indefinidamente
		while(socketFileSystem == NULL) {

			loguear(LOG_LEVEL_DEBUG, "threadNodo", ">>Intento conectarme al fs" );

			socketFileSystem = Sockets_conectar_servidor(GLOBAL_Nodo_Config.ip_fs, string_itoa(GLOBAL_Nodo_Config.puerto_fs));

			if (socketFileSystem == NULL) {
				printf( "- Error al conectarse al FileSystem. \nSe intentará nuevamente en 10 Segundos...\n");
				sleep(10);
			}
		}

		loguear(LOG_LEVEL_DEBUG, "threadNodo", ">>Conexión con FS establecida. Se enviarán datos del nodo." );

		//Se la envio al FS
		iNumeroError = Sockets_enviar_datos(socketFileSystem, ordenAvisarNodoConectado);
		if (iNumeroError <= 0) {
			loguear(LOG_LEVEL_ERROR, "threadNodo", ">>NO pude enviar orden 'avisarNodoConectado' al FS" );
			continue;
		}

		loguear(LOG_LEVEL_DEBUG, "threadNodo", ">>Orden 'avisarNodoConectado' enviada." );

		//Espero contestacion
		iNumeroError = Sockets_recibir_Header(socketFileSystem, &headerRecibido);
		if (iNumeroError <= 0) {
			loguear(LOG_LEVEL_ERROR, "threadNodo", ">>NO pude recibir header del FS");
			continue;
		}

		loguear(LOG_LEVEL_DEBUG, "threadNodo", ">>Header de FS recibido.");

		iNumeroError = Sockets_recibir_Datos(socketFileSystem, &confirmacion, headerRecibido);
		if (iNumeroError <= 0) {
			loguear(LOG_LEVEL_ERROR, "threadNodo", ">>NO pude recibir payloadAvisarNodoConectado del FS");
			continue;
		}

		loguear(LOG_LEVEL_DEBUG, "threadNodo", ">>Payload de FS recibido.");

		//Verifico la Contestacion Recibida o sino imprimo que me llego
		if (!string_equals_ignore_case(confirmacion, "Ok, ya estas conectado")) {
			loguear(LOG_LEVEL_ERROR, "threadNodo", ">>No llegó una confirmación correcta del FS: %s", confirmacion);
			continue;
		}

		loguear(LOG_LEVEL_DEBUG, "threadNodo", ">>Conexión a FS validada.");

		conexionConfirmada = true;
	}

	printf(ANSI_COLOR_RESET "\n");		//Por Estabilidad
	loguearPantalla(LOG_LEVEL_INFO, __func__, ">>Nodo validado por FS");

	//libero recursos del paquete enviado
	Comun_LiberarMemoria((void**)&ordenAvisarNodoConectado);

	Sockets_cerrar_desconectar(socketFileSystem);

	return EXIT_SUCCESS;
}

void threadBaseServidor(char* sPuertoEscucha) {

	//Indico que Cada vez que me llegue la señal SIGCHLD (se termino un proceso hijo) revise si esta en estado Zombie y lo mata correctamente
	//Voy a definir la accion para la señal SIGCHLD
	/*	Creo un struct necesario para la funcion "sigaction"
	 NOTA: Es necesario poner "struct" porque hay una funcion que se llama igual al tipo de dato (sino, entiende cualquier cosa el compilador)
	 */
	struct sigaction accionSenial_SIGCHLD;

	//Le especifico que la accion sera llamar a la funcion "Handler_Eliminar_Zombies"
	accionSenial_SIGCHLD.sa_handler = Handler_Eliminar_Zombies;

	//Limpio la lista de Señales que atrapa por Defecto y Establesco las Flags Necesarias
	 if( sigemptyset(&accionSenial_SIGCHLD.sa_mask)){
		 loguear(LOG_LEVEL_ERROR, "threadBaseServidor" , "(Linea %d) No se pudieron limpiar las señales por defecto. Error detectado: %s", __LINE__ , strerror(errno));
		 return;
	 }
	 accionSenial_SIGCHLD.sa_flags = SA_RESTART | SA_NOCLDSTOP;


	//Ahora si establesco la Accion para la Señal, a partir de ahora es automatico, no hace falta tocar nada mas
	if (sigaction( SIGCHLD, &accionSenial_SIGCHLD, NULL) ){
		loguear(LOG_LEVEL_ERROR, "threadBaseServidor" , "(Linea %d) No se pudo redefinir la señal SIGCHLD. Error detectado: %s", __LINE__ , strerror(errno));
		return;
	}

	//Creo el Socket de Escucha del Servidor
	tipo_socket* socketEnEscucha = Sockets_ponerme_escuchar(sPuertoEscucha);
	//Chequeo errores
	if (socketEnEscucha==NULL) {
		loguear(LOG_LEVEL_ERROR, "threadBaseServidor", "(Linea %d) Hubo un problema con la conexion apenas nos pusimos en escucha...", __LINE__);
		return;
	}

	Comun_Pantalla_Separador_Destacar("Servidor Iniciado");

	//Acepto nuevos clientes infinitamente, bloqueandose hasta que se conecte un nuevo cliente
	while ( INFINITO ) {
		//Variable para Manejo de Errores
		int iNumeroError = 0;

		Macro_ImprimirParaDebug("Esperando nuevas ordenes...\n");

		//Acepto un nuevo cliente
		tipo_socket* socketConectadoAlCliente = Sockets_aceptar_cliente(socketEnEscucha);
		//Chequeo errores
		if (socketEnEscucha == NULL) {
			loguear(LOG_LEVEL_ERROR, "threadBaseServidor", "(Linea %d) Hubo un problema con la conexion cuando intentamos aceptar un cliente...", __LINE__);
			return;
		}

		//Armo el string para loguear que se conecto un cliente
		loguear(LOG_LEVEL_TRACE, "Servidor de Escucha", "Se conecto alguien al Nodo, su IP es: %s y su Puerto es: %d", Sockets_obtener_ip_cliente(socketConectadoAlCliente), Sockets_obtener_puerto_cliente(socketConectadoAlCliente) );


		//Creo una variable para el ID del Thread, no nos importa realmente que se pierda el ID de los Threads Hijos
		pthread_t idThread;

		//Creo un nuevo Thread para atender al cliente y le paso el socketConectadoAlCliente
		iNumeroError = pthread_create(&idThread, NULL, (void*) &threadCreadoPorCliente,(void*) socketConectadoAlCliente);

		//Chequeo Errores al crear el thread
		if (iNumeroError != 0) {
			Macro_Imprimir_Error("Error al crear el thread");
			return;
		}

		//Le digo al Thread que cuando se termine libere los recursos que uso, lo hace el solo y nos olvidamos nosotros
		iNumeroError = pthread_detach(idThread);

		//Chequeo Errores en el detach
		if (iNumeroError != 0) {
			Macro_Imprimir_Error("Error al hacer detach");
			return;
		}

	}

	//En teoria nunca ejecutaria aca abajo
	return;
}

void threadCreadoPorCliente(void* socket) {
	/*	Basicamente lo que hace es leer el header y en base a el lee cual es la orden que nos envian.
	 *	Luego en base a la orden llama a una funcion que realiza lo que pide la orden
	 *	Entonces hay tantas funciones como Orden
	 */

	//Para evitar hacer muchos casteos cada vez que llamamos a "socket"
	tipo_socket* socketConectadoAlCliente = (tipo_socket*) socket;

	//Variable para controlar errores
	int iNumeroError = 0;
	//Variable para el header
	t_header headerRecibido;

	//Recibimos el header
	iNumeroError=Sockets_recibir_Header(socketConectadoAlCliente, &headerRecibido);

	//Chequeo Errores
	if(iNumeroError <= 0){

		loguear(LOG_LEVEL_WARNING, "threadCreadoPorCliente", "Se corto la conexión apenas recibimos al cliente. No tenemos manera de saber el emisor.\n");
		return;

	}else if(iNumeroError < 0){
		loguear(LOG_LEVEL_ERROR, "threadCreadoPorCliente", "(Linea %d) Hubo un problema con la conexion apenas recibimos al cliente.", __LINE__);
		return;
	}

	//Veo quien se me conecto y en base a eso voy a ver que orden/peticion me mandaron para tenderla
	switch (headerRecibido.sender_id) {

		//Veo si se me conectó el fs
		case FILESYSTEM:
			loguearPantalla(LOG_LEVEL_TRACE, __func__, "Llego una orden:'%s' desde el File System", headerRecibido.order);

			if(header_esOrden(headerRecibido, "pedirSubirArchivoFinal")){

				Comun_Pantalla_Separador_Destacar("Orden: pedirSubirArchivoFinal");
				ordenPedirArchivo(socket, headerRecibido);

			}else if(header_esOrden(headerRecibido, "envioUbicacionBloqueParaGuardar")){

				Comun_Pantalla_Separador_Destacar("Orden: envioUbicacionBloqueParaGuardar");
				ordenEnvioUbicacionBloqueParaGuardar(socket, headerRecibido);

			}else if(header_esOrden(headerRecibido, "pedirBloque")){

				Comun_Pantalla_Separador_Destacar("Orden: pedirBloque");
				ordenPedirBloque(socket, headerRecibido);

			}else if(header_esOrden(headerRecibido, "copiarBloqueANodo")){

				Comun_Pantalla_Separador_Destacar("Orden: copiarBloqueANodo");
				ordenCopiarBloqueANodo(socket, headerRecibido);
				
			}else if(header_esOrden(headerRecibido, "pedirBloqueMD5")){

				Comun_Pantalla_Separador_Destacar("Orden: pedirBloqueMD5");
				ordenPedirBloqueMD5(socket, headerRecibido);	

			}else {
				loguear(LOG_LEVEL_WARNING, "threadCreadoPorCliente", " Llego una orden del FileSystem que no se reconoce, la orden era: %s", headerRecibido.order);
			}
			break;

		//Veo si se me conectó algún nodo
		case NODO:
			loguearPantalla(LOG_LEVEL_TRACE, __func__, "Llego una orden:'%s' desde un Nodo", headerRecibido.order);

			if(header_esOrden(headerRecibido, "pedirArchivo")) {

				Comun_Pantalla_Separador_Destacar("Orden: pedirArchivo");
				ordenPedirArchivo(socket, headerRecibido);

			}else if (header_esOrden(headerRecibido, "envioUbicacionBloqueParaGuardar")) {

				Comun_Pantalla_Separador_Destacar("Orden: envioUbicacionBloqueParaGuardar");
				ordenEnvioUbicacionBloqueParaGuardar(socket, headerRecibido);

			}else if(header_esOrden(headerRecibido, "pedirBloque")){

				Comun_Pantalla_Separador_Destacar("Orden: pedirBloque");
				ordenPedirBloque(socket, headerRecibido);

			}else {
				loguear(LOG_LEVEL_WARNING, "threadCreadoPorCliente", "Llego una orden de un Nodo que no se reconoce, la orden era: %s", headerRecibido.order);
			}
			break;

		case JOB:
			loguearPantalla(LOG_LEVEL_TRACE, __func__, "Llego una Orden:'%s' desde un Job", headerRecibido.order);

			if (header_esOrden(headerRecibido, "ejecutarMapping")) {

				Comun_Pantalla_Separador_Destacar("Orden: ejecutarMapping");
				ordenEjecutarMapping(socket, headerRecibido);

			}else if(header_esOrden(headerRecibido, "ejecutarReduceSinCombine")){

				Comun_Pantalla_Separador_Destacar("Orden: ejecutarReduceSinCombine");
				ordenEjecutarReduceSinCombine(socket, headerRecibido);

			}else if(header_esOrden(headerRecibido, "ejecutarReduceConCombine")){

				Comun_Pantalla_Separador_Destacar("Orden: ejecutarReduceConCombine");
				ordenEjecutarReduceConCombine(socket, headerRecibido);

			}else {
				loguear(LOG_LEVEL_WARNING, "threadCreadoPorCliente", "Llego una orden de un Job que no se reconoce, la orden era: %s", headerRecibido.order );
			}
			break;

		default:

			loguear(LOG_LEVEL_WARNING, "threadCreadoPorCliente", "Llego una orden de alguien que no reconozco. Sender_ID: %d. Orden: %s", headerRecibido.sender_id, headerRecibido.order);
			break;
	}

	//Salgo de la Funcion
	pthread_exit( EXIT_SUCCESS);
}

//Defino funciones para realizar las órdenes

void ordenPedirArchivo(tipo_socket* socket, t_header header) {

	char* nombreArchivo;
	char* pathCompleto = string_new();
	char* payload = NULL;
	uint32_t punteroPosicion = 0;
	uint32_t contadorNumeroBloque = 0;
	t_datos_archivo_final datosArchivoFinal;
	int iError = 0;
	uint32_t tamanioArchivo = 0;
	char* packageToSend = NULL;
	char* ordenEnviarDatos = NULL;
	char* ordenEnviarBloques = NULL;
	uint32_t tamanioSerializacion = -1;
	bool estabaConSocket = true;

	loguear(LOG_LEVEL_INFO, __func__, "Iniciando orden");

	// Se recibe el payload con el nombre de archivo final a enviar
	iError = Sockets_recibir_Datos(socket, &nombreArchivo, header);
	Macro_Check_And_Handle_Error( iError <= 0, "Hubo un error en la recepcion de datos");
	estabaConSocket = false;

	// Se arma el path completo al archivo final
	string_append(&pathCompleto, GLOBAL_Nodo_Config.dir_temp);
	string_append(&pathCompleto, nombreArchivo);
	string_append(&pathCompleto, "\0");

	loguear(LOG_LEVEL_TRACE, __func__, "Viendo si existe el archivo: '%s'", pathCompleto);

	// Se verifica si existe el archivo
	// En el caso negativo se desconecta el socket al FS
	iError = Comun_existeArchivo(pathCompleto);
	Macro_Check_And_Handle_Error(iError == -1, "Hubo un error en la recepcion de datos");

	loguear(LOG_LEVEL_TRACE, __func__, "Si existe el archivo. Obteniendo tamanio archivo");

	// Se determina el tamano del archivo final a enviar
	tamanioArchivo = Bloques_obtener_tamanio_archivo(pathCompleto);
	Macro_Check_And_Handle_Error(tamanioArchivo == -1, "No se pudo obtener el tamaño del archivo");

	loguear(LOG_LEVEL_TRACE, __func__, "Obtenido tamaño archivo:'%ld'. Preparando datos archivo", tamanioArchivo);

	// Se preparan para el envio los datos del archivo final
	datosArchivoFinal.tamanio = tamanioArchivo;
	datosArchivoFinal.cantidad_bloques = Bloques_obtener_cantidad_bloques_archivo(pathCompleto);
	Macro_Check_And_Handle_Error(datosArchivoFinal.cantidad_bloques == -1, "No se pudo obtener la cantidad de bloques del archivo");

	loguear(LOG_LEVEL_TRACE, __func__, "Cantidad de bloques:'%ld'. Obteniendo MD5", datosArchivoFinal.cantidad_bloques);

	char* md5_obtenido = Comun_obtener_MD5(pathCompleto,true);
	Macro_Check_And_Handle_Error(md5_obtenido == NULL, "El archivo tiene tamaño 0, revisar que no este vacio");

	strcpy( datosArchivoFinal.md5 , md5_obtenido );

	loguear(LOG_LEVEL_TRACE, __func__, "Serializando datos archivo");

	payload = serializar_datos_archivo_final(datosArchivoFinal, &tamanioSerializacion);

	// Dependiendo de si la orden vino de FileSystem o Nodo respondo con la ordenes diferentes
	if(header.sender_id == FILESYSTEM) {
		ordenEnviarDatos = "enviarDatosArchivoFinal";
		ordenEnviarBloques = "enviarBloqueArchivoFinal";
	}
	else {
		ordenEnviarDatos = "enviarDatosArchivo";
		ordenEnviarBloques = "enviarBloqueArchivo";
	}

	// Preparamos el package a enviar
	packageToSend = package_create(payload, tamanioSerializacion, ordenEnviarDatos, NODO);

	loguear(LOG_LEVEL_TRACE, __func__, "Enviando los datos del archivo");

	// Se envian los datos del archivo final al que lo pidio
	estabaConSocket = true;
	iError = Sockets_enviar_datos(socket, packageToSend);
	Macro_Check_And_Handle_Error( iError <= 0, "No se enviaron los datos del archivo");
	estabaConSocket = false;

	// Liberamos la memoria
	Comun_LiberarMemoria((void**)&packageToSend);
	Comun_LiberarMemoria((void**)&payload);

	loguear(LOG_LEVEL_TRACE, __func__, "Empiezo a enviar bloques");

	// Mientras hay mas bloques seguimos enviandolos a FS
	while(punteroPosicion < tamanioArchivo) {

		//Aumento el contador para el siguiente bloque
		contadorNumeroBloque++;

		loguear(LOG_LEVEL_TRACE, __func__, "Leyendo bloque: %d", contadorNumeroBloque);

		//Puntero para el bloque a leer
		tipo_bloque* punteroBloque = NULL;

		//Leo el bloque del archivo y lo cargo en memoria
		punteroBloque = Bloques_obtener_desde_archivo_texto(pathCompleto, &punteroPosicion);
		Macro_Check_And_Handle_Error(punteroBloque == NULL, "Problema al leer el bloque: %d", contadorNumeroBloque);

		loguear(LOG_LEVEL_TRACE, __func__, "Bloque leido, serializandolo");

		// Serializo y preparo el package que contiene el bloque para enviarselo a FS
		tamanioSerializacion = -1;
		char* payloadBloque = Bloques_serializar(punteroBloque, &tamanioSerializacion);
		packageToSend = package_create(payloadBloque, tamanioSerializacion, ordenEnviarBloques, NODO);
		Comun_LiberarMemoria((void**)&payloadBloque);

		loguear(LOG_LEVEL_TRACE, __func__, "Enviando bloque");

		// Se realiza el envio del bloque en cuestion
		estabaConSocket = true;
		iError = Sockets_enviar_datos(socket, packageToSend);
		Macro_Check_And_Handle_Error(iError <= 0, "No se envio el bloque del archivo");
		estabaConSocket = false;

		loguear(LOG_LEVEL_TRACE, __func__, "Se envio el bloque del archivo");

		// Se libera la memoria del package
		Comun_LiberarMemoria((void**)&packageToSend);
		Comun_LiberarMemoria((void**)&punteroBloque);

		loguear(LOG_LEVEL_TRACE, __func__, "Bloque %d enviado", contadorNumeroBloque);
	}

	loguear(LOG_LEVEL_INFO, __func__, "Fin de orden \n");

	// Se libera la memoria de los strings generados
	Comun_LiberarMemoria((void**)&nombreArchivo);
	Comun_LiberarMemoria((void**)&pathCompleto);

	Sockets_cerrar_desconectar(socket);

	return;

Error_Handler:

	loguear(LOG_LEVEL_ERROR, __func__, "Hubo un error en la ejecucion de la orden ordenPedirArchivo");

	Comun_LiberarMemoria((void**)&packageToSend);
	Comun_LiberarMemoria((void**)&payload);

	if (estabaConSocket == false){
		Sockets_cerrar_desconectar(socket);
	}

	return;
}


void ordenEnvioUbicacionBloqueParaGuardar(tipo_socket* socket, t_header header) {

	int iError = 0;
	char* numeroBloque = NULL;
	uint32_t numeroBloqueFinal = 0;
	char* payloadBloque = NULL;
	tipo_bloque* datosBloque = NULL;
	t_header headerDatosBloque;
	char* respuestaOkSerializada = NULL;
	char* respuestaError = NULL;
	bool estabaEnviandoORecibiendoFS = true;
	char* pathCarpetaTemporal = NULL;

	loguear(LOG_LEVEL_INFO, __func__, "Inicio Orden");

	// Se recibe el payload con el numero del bloque en el archivo .bin
	iError = Sockets_recibir_Datos(socket, &numeroBloque, header);
	Macro_Check_And_Handle_Error(iError <= 0, "Hubo un error en la recepcion del numero de bloque a guardar de FS");
	estabaEnviandoORecibiendoFS = false;

	numeroBloqueFinal = atol(numeroBloque);

	fprintf(stdout, "Bloque a recibir: %s\n", numeroBloque);

	loguear(LOG_LEVEL_TRACE, __func__, "Bloque numero '%d'. Recibiendo header bloque", numeroBloqueFinal);

	//headerDatosBloque = malloc(sizeof(t_header));
	estabaEnviandoORecibiendoFS = true;
	iError = Sockets_recibir_Header(socket, &headerDatosBloque);
	Macro_Check_And_Handle_Error(iError <= 0, "Hubo un error en la recepcion del header de los datos del bloque de FS");
	estabaEnviandoORecibiendoFS = false;
	
	loguear(LOG_LEVEL_TRACE, __func__, "Recibiendo bloque");

	estabaEnviandoORecibiendoFS = true;
	iError = Sockets_recibir_Datos(socket, &payloadBloque, headerDatosBloque);
	Macro_Check_And_Handle_Error(iError <= 0, "Hubo un error en la recepcion de datos del bloque de FS");
	estabaEnviandoORecibiendoFS = false;
	
	loguear(LOG_LEVEL_TRACE, __func__, "Deserializando Bloque");

	datosBloque = Bloques_des_serializar(payloadBloque);

	//Comun_LiberarMemoria((void**)&headerDatosBloque);
	Comun_LiberarMemoria((void**)&payloadBloque);

	loguear(LOG_LEVEL_TRACE, __func__, "Escribiendo bloque en archivo bin");

	iError = ArchivoBIN_EscribirBloque(numeroBloqueFinal, datosBloque);
	Macro_Check_And_Handle_Error(iError == -1, "Hubo un error al escribir en el archivo bin");

	pathCarpetaTemporal = string_new();
	string_append(&pathCarpetaTemporal, GLOBAL_Nodo_Config.dir_temp);
	string_append(&pathCarpetaTemporal, string_itoa(numeroBloqueFinal));
	string_append(&pathCarpetaTemporal, ".txt");

	iError = Archivo_EscribirBloque(pathCarpetaTemporal, 0, datosBloque);
	Macro_Check_And_Handle_Error(iError == -1, "Hubo un error al escribir el bloque en la carpeta temporal");

	Comun_LiberarMemoria((void**) &datosBloque);
	Comun_LiberarMemoria((void**) &pathCarpetaTemporal);

	loguear(LOG_LEVEL_TRACE, __func__, "Bloque guardado, avisando a %s : %d", Sockets_obtener_ip_cliente(socket), Sockets_obtener_puerto_cliente(socket));

	char* respuestaOkFS = "Bloque Guardado Correctamente";
	respuestaOkSerializada = package_create(respuestaOkFS, strlen(respuestaOkFS) + 1, "respuestaBloqueGuardado", NODO);
	
	estabaEnviandoORecibiendoFS = true;
	iError = Sockets_enviar_datos(socket, respuestaOkSerializada);
	Macro_Check_And_Handle_Error(iError <= 0, "Hubo un error en el envio de la respuesta exitosa al FS");
	estabaEnviandoORecibiendoFS = false;
	
	Comun_LiberarMemoria((void**)&respuestaOkSerializada);

	loguear(LOG_LEVEL_INFO, __func__, "Fin Orden \n");

	fprintf(stdout, "Bloque guardado correctamente. Fin de orden.\n");

	return;

Error_Handler:

	loguear(LOG_LEVEL_ERROR, __func__, "Ocurrio un error en la recepcion del bloque a guardar. Aviso al FileSystem");

	respuestaError = "No se pudo Guardar el Archivo";
	char* respuestaErrorSerializado = package_create(respuestaError, strlen(respuestaError) + 1, "respuestaBloqueGuardado", NODO);

	if( estabaEnviandoORecibiendoFS == false ){
		loguear(LOG_LEVEL_ERROR, __func__, "Aviso al FS que hubo Error al guardar el bloque");
		
		iError = Sockets_enviar_datos(socket, respuestaErrorSerializado);
		if(iError == 0 || iError == -1) {
			loguear(LOG_LEVEL_ERROR, __func__, "No se pudo enviar la ejecucion erronea de la orden envioUbicacionBloqueParaGuardar");
		}
		Comun_LiberarMemoria((void**) &respuestaOkSerializada);
	}else{
		loguear(LOG_LEVEL_ERROR, __func__, "Se corto la conexion al FS y no podemos avisar del error");
	}
	Comun_LiberarMemoria((void**)&respuestaErrorSerializado);

	Comun_LiberarMemoria((void**)&pathCarpetaTemporal);
	Comun_LiberarMemoria((void**)&datosBloque);
	Comun_LiberarMemoria((void**)&payloadBloque);
	Comun_LiberarMemoria((void**)&respuestaOkSerializada);

	return;
}

void ordenPedirBloque(tipo_socket* socket, t_header header) {

	int iError = 0;
	char* datosDelBloqueSerializados = NULL;
	tipo_bloque* bloqueEnviar = NULL;
	uint32_t tamanioSerializado = -1;
	char* payloadBloque = NULL;
	char* packageToSend = NULL;

	loguear(LOG_LEVEL_INFO, __func__, "Iniciando Orden");

	iError = Sockets_recibir_Datos(socket, &datosDelBloqueSerializados, header);
	Macro_Check_And_Handle_Error(iError <= 0, "Hubo un error en la recepcion del numero de bloque a enviar de FS");

	t_datos_orden_leer_bloque datosDelBloqueALeer = deserializar_orden_leer_bloque(datosDelBloqueSerializados);

	fprintf(stdout, "Bloque a enviar: %d\n", datosDelBloqueALeer.nodoDestinoNumeroBloque);

	loguear(LOG_LEVEL_TRACE, __func__, "Leyendo bloque del archivo Bin");

	bloqueEnviar = Bloques_crear_nuevo();
	iError = ArchivoBIN_LeerBloque(datosDelBloqueALeer.nodoDestinoNumeroBloque, &bloqueEnviar, datosDelBloqueALeer.tamanioBloque);
	Comun_LiberarMemoria((void**)&datosDelBloqueSerializados);

	if(iError == -1) {
		loguear(LOG_LEVEL_ERROR, __func__, "Error al leer el archivo bin");
	}

	Macro_Check_And_Handle_Error(iError == -1, "Hubo un error al Leer del archivo bin");

	loguear(LOG_LEVEL_TRACE, __func__, "Serializando bloque. Tamanio:'%ld'", bloqueEnviar->tamanioBloque);

	// Serializo y preparo el package que contiene el bloque para enviarselo a FS
	payloadBloque = Bloques_serializar(bloqueEnviar, &tamanioSerializado);
	Bloques_destruir(bloqueEnviar);
	//Comun_LiberarMemoria((void**)&bloqueEnviar);

	packageToSend = package_create(payloadBloque, tamanioSerializado, "enviarBloquePedido", NODO);
	Comun_LiberarMemoria((void**)&payloadBloque);

	loguear(LOG_LEVEL_TRACE, __func__, "Enviando bloque");

	// Se realiza el envio del bloque en cuestion
	iError = Sockets_enviar_datos(socket, packageToSend);
	Macro_Check_And_Handle_Error(iError <= 0, "No se pudo enviar bloque");


	Comun_LiberarMemoria((void**)&packageToSend);

	loguear(LOG_LEVEL_INFO, __func__, "Fin orden \n");

	fprintf(stdout, "Bloque enviado correctamente. Fin de orden.\n");

	return;

Error_Handler:

	loguear(LOG_LEVEL_ERROR, "ordenPedirBloque", "Ocurrió un error en la realizacion del pedido del bloque.");

	Comun_LiberarMemoria((void**)&packageToSend);
	Comun_LiberarMemoria((void**)&bloqueEnviar);
	Comun_LiberarMemoria((void**)&payloadBloque);
	Comun_LiberarMemoria((void**)&datosDelBloqueSerializados);
	return;

}

void ordenCopiarBloqueANodo(tipo_socket* socketConectadoFS, t_header header) {

	uint32_t iError = 0;
	tipo_bloque* bloqueEnviar = NULL;
	t_datos_orden_copiar_bloque datosBloque;
	uint32_t numeroBloqueReal = 0;
	char* datosSerializados = NULL;
	tipo_socket* socketNodoDestino = NULL;
	char* datosSerializadosEnviar = NULL;
	char* numeroBloqueSerializado = NULL;
	uint32_t tamanioSerializacion = -1;
	char* packageEnviar = NULL;
	char* respuestaOk = NULL;
	char* respuestaOkSerializado = NULL;
	bool socketNodo = false;
	char* respuestaError = NULL;
	bool estabaEnviandoORecibiendoFS = true;

	loguear(LOG_LEVEL_INFO, __func__, "Inicio Orden");

	loguear(LOG_LEVEL_TRACE, __func__, "Recibiendo Datos de FS");

	iError = Sockets_recibir_Datos(socketConectadoFS, &datosSerializados, header);
	Macro_Check_And_Handle_Error(iError <= 0, "Hubo un error en la recepcion del numero de bloque a enviar de FS");
	estabaEnviandoORecibiendoFS = false;

	loguear(LOG_LEVEL_TRACE, __func__, "Deserializando bloque");

	datosBloque = deserializar_orden_copiar(datosSerializados);
	Comun_LiberarMemoria((void**)&datosSerializados);

	loguear(LOG_LEVEL_TRACE, __func__, "Leyendo bloque de archivo bin");
	bloqueEnviar = Bloques_crear_nuevo();
	//numeroBloqueReal = datosBloque.nodoOrigenNumeroBloque;
	iError = ArchivoBIN_LeerBloque(datosBloque.nodoOrigenNumeroBloque, &bloqueEnviar, datosBloque.tamanioBloque);

	if(iError == -1) {
		loguear(LOG_LEVEL_ERROR, __func__, "Error al leer el archivo bin");
	}

	Macro_Check_And_Handle_Error(iError == -1, "Hubo un error al leer del archivo bin");


	fprintf(stdout, "Bloque original: %d\n", numeroBloqueReal);

	loguear(LOG_LEVEL_TRACE, __func__, "Conectando nodo a enviar bloque");

	socketNodoDestino = Sockets_conectar_servidor(datosBloque.nodoDestinoIP, string_itoa((int) datosBloque.nodoDestinoPuerto));
	Macro_Check_And_Handle_Error(socketNodoDestino == NULL, "No se pudo establecer conexion con el nodo destino");
	socketNodo = true;

	char* nodoDestinoNroBloque = string_itoa((int) datosBloque.nodoDestinoNumeroBloque);

	numeroBloqueSerializado = package_create(nodoDestinoNroBloque, strlen(nodoDestinoNroBloque) + 1, "envioUbicacionBloqueParaGuardar", NODO);
	
	loguear(LOG_LEVEL_TRACE, __func__, "Enviando orden de guardar bloque a nodo destino");

	iError = Sockets_enviar_datos(socketNodoDestino, numeroBloqueSerializado);
	Macro_Check_And_Handle_Error(iError == 0 || iError == -1, "No se pudo enviar el numero del bloque a guardar en el nodo destino");	
	Comun_LiberarMemoria((void**)&numeroBloqueSerializado);

	loguear(LOG_LEVEL_TRACE, __func__, "Serializando bloque");

	datosSerializadosEnviar = Bloques_serializar(bloqueEnviar, &tamanioSerializacion);
	Bloques_destruir(bloqueEnviar);

	packageEnviar = package_create(datosSerializadosEnviar, tamanioSerializacion, "envioBloqueSerializadoParaGuardar", NODO);
	Comun_LiberarMemoria((void**)&datosSerializadosEnviar);

	loguear(LOG_LEVEL_TRACE, __func__, "Enviando bloque a nodo destino");

	iError = Sockets_enviar_datos(socketNodoDestino, packageEnviar);
	Macro_Check_And_Handle_Error(iError == 0 || iError == -1, "No se pudo enviar el bloque al nodo destino");	
	Comun_LiberarMemoria((void**)&packageEnviar);

	Sockets_cerrar_desconectar(socketNodoDestino);
	socketNodo = false;

	loguear(LOG_LEVEL_TRACE, __func__, "Se envio bloque. Aviso a FS");

	respuestaOk = "Confirmacion";
	respuestaOkSerializado = package_create(respuestaOk, strlen(respuestaOk) + 1, "respuestaCopiarBloqueANodo", NODO);

	estabaEnviandoORecibiendoFS = true;
	iError = Sockets_enviar_datos(socketConectadoFS, respuestaOkSerializado);
	Macro_Check_And_Handle_Error(iError == 0 || iError == -1, "No se pudo enviar la confirmacion");
	estabaEnviandoORecibiendoFS = false;

	Comun_LiberarMemoria((void**)&respuestaOkSerializado);

	Sockets_cerrar_desconectar(socketConectadoFS);

	loguear(LOG_LEVEL_INFO, __func__, "Fin Orden \n");

	fprintf(stdout, "Bloque enviado correctamente. Fin de orden\n");

	return;

Error_Handler:

	loguear(LOG_LEVEL_ERROR, __func__, "Problema en algun lado");

	respuestaError = "No se realizo la orden ordenCopiarBloqueANodo\0";
	char* respuestaErrorSerializado = package_create(respuestaError, strlen(respuestaError) + 1, "cancelacionCopiarBloqueANodo", NODO);

	if( estabaEnviandoORecibiendoFS == false ){
		loguear(LOG_LEVEL_ERROR, __func__, "Aviso al FS que hubo Error al copiar el Bloque");
		
		iError = Sockets_enviar_datos(socketConectadoFS, respuestaErrorSerializado);
		if(iError == 0 || iError == -1) {
			loguear(LOG_LEVEL_ERROR, "ordenCopiarBloqueANodo", "No se pudo enviar la ejecucion erronea de la orden ordenCopiarBloqueANodo");
		}
		Comun_LiberarMemoria((void**)&respuestaOkSerializado);
	}else{
		loguear(LOG_LEVEL_ERROR, __func__, "Se corto la conexion al FS y no podemos avisar del error");
	}
	Comun_LiberarMemoria((void**)&respuestaErrorSerializado);

	if(socketNodo) {
		Sockets_cerrar_desconectar(socketNodoDestino);
	}

	// Libero todos los recursos
	Bloques_destruir(bloqueEnviar);
	Comun_LiberarMemoria((void**)&datosSerializados);
	Comun_LiberarMemoria((void**)&socketNodoDestino);
	Comun_LiberarMemoria((void**)&datosSerializadosEnviar);
	Comun_LiberarMemoria((void**)&numeroBloqueSerializado);
	Comun_LiberarMemoria((void**)&packageEnviar);
	return;
}

void ordenPedirBloqueMD5(tipo_socket* socketConectadoFS, t_header header) {

	int iError = 0;
	bool estabaEnviandoORecibiendoFS = true;
	char* datosSerializados = NULL;
	uint32_t numeroBloqueReal = 0;
	char* bloqueMD5 = NULL;
	tipo_bloque* punteroBloque = NULL;
	char* packageEnviar = NULL;
	char* bloqueMD5Serializado = NULL;
	char* respuestaError = NULL;
	uint32_t tamanioSerializacion = 0;
	char* respuestaOkSerializado = NULL;
	
	loguear(LOG_LEVEL_INFO, __func__, "Inicio Orden");

	loguear(LOG_LEVEL_TRACE, __func__, "Recibiendo Datos de FS");

	iError = Sockets_recibir_Datos(socketConectadoFS, &datosSerializados, header);
	Macro_Check_And_Handle_Error(iError <= 0, "Hubo un error en la recepcion del numero de bloque de FS");
	estabaEnviandoORecibiendoFS = false;

	loguear(LOG_LEVEL_TRACE, __func__, "Deserializando los datos de la orden");
	
	t_datos_orden_pedir_md5_bloque datosOrdenMD5 = deserializarNodoPedirBloqueMD5(datosSerializados);
	Comun_LiberarMemoria((void**) &datosSerializados);
	
	loguear(LOG_LEVEL_TRACE, __func__, "Leyendo bloque de archivo bin");
	punteroBloque = Bloques_crear_nuevo();

	iError = ArchivoBIN_LeerBloque(datosOrdenMD5.nodoDestinoNumeroBloque, &punteroBloque, datosOrdenMD5.tamanioBloque);
	if(iError == -1) {
		loguear(LOG_LEVEL_ERROR, __func__, "Error al leer el archivo bin");
	}
	
	Macro_Check_And_Handle_Error(iError == -1, "Hubo un error al leer del archivo bin");
	
	bloqueMD5 = Comun_obtener_MD5_Bloque(punteroBloque, true);
	Macro_Check_And_Handle_Error(bloqueMD5 == NULL, "Hubo un error al intentar de calcular MD5 del bloque");
	Bloques_destruir(punteroBloque);
	
	loguear(LOG_LEVEL_TRACE, __func__, "Serializando el MD5 del bloque");

	bloqueMD5Serializado = serializarNodoPedirBloqueMD5(bloqueMD5, &tamanioSerializacion);
	Comun_LiberarMemoria((void**) &bloqueMD5);
	
	packageEnviar = package_create(bloqueMD5Serializado, tamanioSerializacion, "pedirBloqueMD5", NODO);
	Comun_LiberarMemoria((void**) &bloqueMD5Serializado);


	loguear(LOG_LEVEL_TRACE, __func__, "Enviando el MD5 del bloque a nodo destino");
	estabaEnviandoORecibiendoFS = true;
	iError = Sockets_enviar_datos(socketConectadoFS, packageEnviar);
	Macro_Check_And_Handle_Error(iError <= 0, "No se pudo enviar el MD5 del bloque");
	estabaEnviandoORecibiendoFS = false;
	loguear(LOG_LEVEL_TRACE, __func__, "Se envio exitosamente el MD5 del bloque");
	
	Comun_LiberarMemoria((void**) &packageEnviar);

	Sockets_cerrar_desconectar(socketConectadoFS);
	
	return;

  Error_Handler:

    loguear(LOG_LEVEL_ERROR, __func__, "Hubo error en la ejecucion de la orden pedirBloqueMD5");

  	respuestaError = "No se realizo la orden pedirBloqueMD5\0";
  	char* respuestaErrorSerializado = package_create(respuestaError, strlen(respuestaError) + 1, "cancelacionPedirBloqueMD5", NODO);

  	if(estabaEnviandoORecibiendoFS == false) {
  		loguear(LOG_LEVEL_ERROR, __func__, "Aviso al FS que hubo un error al calcular el MD5 del bloque");

  		iError = Sockets_enviar_datos(socketConectadoFS, respuestaErrorSerializado);
  		if(iError == 0 || iError == -1) {
  			loguear(LOG_LEVEL_ERROR, __func__, "No se pudo enviar la ejecucion erronea de la orden pedirBloqueMD5");
  		}
  		Comun_LiberarMemoria((void**) &respuestaOkSerializado);
  	}
  	else {
  		loguear(LOG_LEVEL_ERROR, __func__, "Se corto la conexion al FS y no podemos avisar del error");
  	}

	Bloques_destruir(punteroBloque);
  	Comun_LiberarMemoria((void**) &respuestaErrorSerializado);
	Comun_LiberarMemoria((void**) &datosSerializados);
	Comun_LiberarMemoria((void**) &bloqueMD5);
	Comun_LiberarMemoria((void**) &bloqueMD5Serializado);
	Comun_LiberarMemoria((void**) &packageEnviar);

	return;

}

void ordenEjecutarMapping(tipo_socket* socketConectadoAlJob, t_header header) {

	char* nombreScript = NULL;
	int resultado = 0;
	char* buffer = NULL;
	uint32_t numeroBloque = 0;
	char* nombreArchivoTemporal = NULL;
	tipo_bloque* bloque = NULL;
	char* pathArchivoFinal = NULL;
	char* pathScript = NULL;
	char* respuestaAJob = NULL;
	char* paqueteSerializado = NULL;
	bool estabaRecibiendoOEnviandoJob = true;
	uint32_t tamanioBloque = 0;

	loguear(LOG_LEVEL_INFO, __func__, "Iniciando orden");

	loguear(LOG_LEVEL_TRACE, __func__, "Recibiendo datos orden mapping");

	//recibo el payload que tiene el nro de bloque local a mapear y el nombre del archivo temporal resultante
	resultado = Sockets_recibir_Datos(socketConectadoAlJob , &buffer , header);
	Macro_Check_And_Handle_Error(resultado <= 0, "ordenEjecutarMapping: Error al recibir datos del payload.");
	estabaRecibiendoOEnviandoJob = false;

	loguear(LOG_LEVEL_TRACE, __func__, "Deserializando datos orden mapping");

	//deserealizo los datos recibidos
	DesSerializar_DatosMapping(buffer, &numeroBloque, &tamanioBloque ,&nombreArchivoTemporal);

	fprintf(stdout, "(tid:%d) Datos para mapping recibidos:\nnumeroBloque: %d\nnombreArchivoTemporal: %s\n", process_get_thread_id(), numeroBloque, nombreArchivoTemporal);

	loguear(LOG_LEVEL_TRACE, __func__, "Recibido que el archivo final se debe llamar:'%s'", nombreArchivoTemporal);

	Comun_LiberarMemoria((void**)&buffer);

	loguear(LOG_LEVEL_TRACE, __func__, "Recibiendo header bloque rutina");

	//quedo a la espera del próximo paquete
	estabaRecibiendoOEnviandoJob = true;
	resultado = Sockets_recibir_Header(socketConectadoAlJob, &header);
	Macro_Check_And_Handle_Error(resultado <= 0, "ordenEjecutarMapping: Error al recibir datos del header.");
	estabaRecibiendoOEnviandoJob = false;

	loguear(LOG_LEVEL_TRACE, __func__, "Esperando datos bloque rutina");


	//verifico que sea el mensaje "envioRutina" que trae en el payload un bloque con la rutina de mapping
	if(header_esOrden(header, "envioRutina")){
		//obtengo el payload

		estabaRecibiendoOEnviandoJob = true;
		resultado = Sockets_recibir_Datos(socketConectadoAlJob , &buffer , header);
		Macro_Check_And_Handle_Error(resultado <= 0, "ordenEjecutarMapping: Error al recibir datos del payload esperando orden envioRutina.");
		estabaRecibiendoOEnviandoJob = false;

		loguear(LOG_LEVEL_TRACE, __func__, "Deserializando bloque rutina");

		//bloque = Bloques_crear_nuevo();

		bloque = Bloques_des_serializar(buffer);

		nombreScript = string_new();
		string_append(&nombreScript, "scriptMapping-");
		string_append(&nombreScript, __TIME__);
		string_append(&nombreScript, "-");
		string_append(&nombreScript, nombreArchivoTemporal);

		loguear(LOG_LEVEL_TRACE, __func__, "Guardando rutina:'%s' de mapping en disco", nombreScript);

		//guardo el script en un archivo en el disco
		resultado = guardarScriptEnDisco(nombreScript, bloque->contenidoBloque, GLOBAL_Nodo_Config.dir_temp, bloque->tamanioBloque);
		Macro_Check_And_Handle_Error(resultado < 0 , "ordenEjecutarMapping: Error al guardar script en disco.");

		Comun_LiberarMemoria((void**)&buffer);
		Comun_LiberarMemoria((void**)&bloque);
		//	Comun_LiberarMemoria((void**)&bloque);
	}else{
		Macro_Check_And_Handle_Error(true, "ordenEjecutarMapping: No llego header 'envioRutina'");
	}

	bloque = Bloques_crear_nuevo();

	loguear(LOG_LEVEL_TRACE, __func__, "Voy a Leer bloque archivo bin para pasarselo a la rutina");

	resultado = ArchivoBIN_LeerBloque(numeroBloque, &bloque, tamanioBloque);

	if(resultado == -1) {
		loguear(LOG_LEVEL_ERROR, __func__, "Error al leer el archivo bin");
	}

	Macro_Check_And_Handle_Error(resultado == -1, "Hubo un error al leer del archivo bin");

	loguear(LOG_LEVEL_TRACE, __func__, "Voy a ejecutar rutina sobre el bloque");

	//armo los paths y ejecuto la rutina llamando al método correspondiente
	pathScript = string_new();
	string_append(&pathScript, GLOBAL_Nodo_Config.dir_temp);
	string_append(&pathScript, nombreScript);

	pathArchivoFinal = string_new();
	string_append(&pathArchivoFinal, GLOBAL_Nodo_Config.dir_temp);
	string_append(&pathArchivoFinal, nombreArchivoTemporal);

	//Macro_ImprimirParaDebugConDatos("Nodo: tamaño del bloque a enviar: %ld, contenido del bloque a enviar: [INICIO BLOQUE]%s[FIN BLOQUE]", bloque->tamanioBloque, bloque->contenidoBloque);

	loguear(LOG_LEVEL_TRACE, __func__, "Path archivo final:'%s'", pathArchivoFinal);

	Macro_ImprimirParaDebug("(tid:%d) Mando a ejecutar el mapping...\n", process_get_thread_id());
	resultado = executeScript( bloque->contenidoBloque, pathArchivoFinal, pathScript, NULL, MAP);

	Macro_ImprimirParaDebugConDatos("Fin de la rutina de mapping. Resultado: %d", resultado);
	loguear(LOG_LEVEL_INFO, "ordenEjecutarMapping", "Fin rutina mapping. Resultado: %d", resultado);

	fprintf(stdout, "(tid:%d) Orden de mapping ejecutada. Resultado:%d\n", process_get_thread_id(), resultado);

	//valido resultado y que el archivo existe. Respondo con "rutinaExitosa" o "rutinaFallida" dependiendo del resultado...
	Macro_Check_And_Handle_Error(resultado < 0, "ordenEjecutarMapping: Rutina de mapping fallida.");

	loguear(LOG_LEVEL_TRACE, __func__, "Verificando que exista el archivo temporal:'%s'", pathArchivoFinal);

	resultado = Comun_existeArchivo(pathArchivoFinal);
	Macro_Check_And_Handle_Error(resultado < 0, "ordenEjecutarMapping: El archivo temporal mappeado no existe :(.");

	respuestaAJob = "Se ejecuto correctamente la rutina";

	paqueteSerializado = package_create(respuestaAJob, strlen(respuestaAJob) + 1, "rutinaExitosa", NODO);

	Macro_Check_And_Handle_Error(paqueteSerializado == NULL, "ordenEjecutarMapping: No se pudo generar el paquete serializado para responder a la rutina.");

	resultado = Sockets_enviar_datos(socketConectadoAlJob , paqueteSerializado);
	Macro_Check_And_Handle_Error(resultado <= 0, "ordenEjecutarMapping: No se pudo Avisar al Job que se completo correctamente la rutina");

	//libero recursos
	Comun_LiberarMemoria((void**)&paqueteSerializado);
	Comun_LiberarMemoria((void**)&nombreArchivoTemporal);
	Comun_LiberarMemoria((void**) &bloque);

	loguear(LOG_LEVEL_INFO, __func__, "Fin orden \n");

	//termino OK
	return;

Error_Handler:

	loguear(LOG_LEVEL_ERROR, "ordenEjecutarMapping", "Ocurrio un error en algun lado de la ejecucion del proceso de mapping. Aviso al Job");

	//Aviso a job del error

	respuestaAJob = "No se ejecuto correctamente la rutina";

	paqueteSerializado = package_create(respuestaAJob, strlen(respuestaAJob) + 1, "rutinaFallida", NODO);

	if(paqueteSerializado == NULL) {
		loguear(LOG_LEVEL_ERROR, "ordenEjecutarMapping", "Ocurrio un error intentar generar el paquete para avisar al Job de la ejecución erronea.");
	}

	if( estabaRecibiendoOEnviandoJob == false ){
		loguear(LOG_LEVEL_ERROR , __func__ , "Enviando al Job que fallo la rutina");
		if ((Sockets_enviar_datos(socketConectadoAlJob , paqueteSerializado)) <= 0) {
			loguear(LOG_LEVEL_ERROR , __func__ , "No pude enviarle al Job que hubo problemas");
		}
	}else{
		loguear(LOG_LEVEL_ERROR , __func__ , "No pude enviar al Job que fallo la rutina porque se cayo la conexion con el Job");
	}

	//libero recursos
	Comun_LiberarMemoria((void**) &bloque);
	Comun_LiberarMemoria((void**) &paqueteSerializado);
	Comun_LiberarMemoria((void**) &nombreArchivoTemporal);
	Comun_LiberarMemoria((void**) &pathArchivoFinal);
	Comun_LiberarMemoria((void**) &pathScript);
	Comun_LiberarMemoria((void**) &buffer);

	return;
}

//recibo el payload con el nombre del archivo temporal resultante y un listado de nombres de archivos locales a los que tengo que ejecutarles la rutina de reduce
//en caso de ser sin combiner, recibo además una lista de archivos en otros nodos. Realizo los pedidos de archivos a los distintos nodos en un ciclo...
//quedo a la espera del mensaje "envioRutina" que trae en el payload un bloque con la rutina de mapping.
//deserializo la rutina con biblioteca bloques y la guardo en un archivo temporal dándole un nombre único y asignando permisos correspondientes.
//apareo los archivos y les aplico la rutina de reduce llamando al método correspondiente
//valido resultado y que el archivo existe. Respondo con "rutinaExitosa" o "rutinaFallida" dependiendo del resultado...
int ejecutarReduce(tipo_socket* socketConectadoAlJob, t_header header, int modo) {

	int i = 0;
	char* buffer = NULL;
	char* nombreArchivoResultado = NULL;
	char* nombreScript = NULL;
	t_list* listaArchivosLocales = NULL;
	t_list* listaNodosExternos = NULL;
	tipo_bloque* bloque = NULL;
	char* archivoAReducir = NULL;
	char* paqueteSerializado = NULL;
	char* pathScript = NULL;
	char* pathArchivoFinal = NULL;
	bool 	estabaConJob = true;
	char* respuestaAJob = NULL;
	uint32_t resultado;

	loguear(LOG_LEVEL_INFO, __func__, "Iniciando");

	//recibo el payload con el nombre del archivo temporal resultante y un listado de nombres de archivos locales a los que tengo que ejecutarles la rutina de reduce
	resultado = Sockets_recibir_Datos(socketConectadoAlJob , &buffer , header);
	Macro_Check_And_Handle_Error(resultado <= 0, "ejecutarReduce: Error al recibir datos del payload.");
	estabaConJob = false;

	//deserealizo los datos recibidos
	if(REDUCE_CON_COMBINER == modo) {
		loguear(LOG_LEVEL_TRACE, __func__, "Deserializando datos reduce CON combine");
		DesSerializar_DatosReduceConCombiner(buffer, &listaArchivosLocales, &nombreArchivoResultado);

		fprintf(stdout, "Recibidos datos para reduce con combiner\n");
		listaNodosExternos = list_create();
	}else {
		loguear(LOG_LEVEL_TRACE, __func__, "Deserializando datos reduce SIN combine");
		DesSerializar_DatosReduceSinCombiner(buffer, &listaArchivosLocales, &listaNodosExternos, &nombreArchivoResultado);
		Macro_ImprimirParaDebug("Cantidad elementos lista nodos externos:'%d'", list_size(listaNodosExternos));
		fprintf(stdout, "Recibidos datos para reduce sin combiner\n");
		Macro_Check_And_Handle_Error(NULL == listaNodosExternos, "listaNodosExternos no puede ser nula.");
	}

	Macro_Check_And_Handle_Error(NULL == nombreArchivoResultado, "Nombre archivo resultado no puede ser nulo.");

	Macro_Check_And_Handle_Error(NULL == listaArchivosLocales, "listaArchivosLocales no puede ser nula.");

	Macro_ImprimirParaDebug("Cantidad elementos lista archivos locales:'%d'", list_size(listaArchivosLocales));

	Comun_LiberarMemoria((void**)&buffer);

	//muestro las listas recibidas
	imprimirListaDeArchivos(listaArchivosLocales, __func__, "Archivos locales");
	imprimirListaDeArchivos(listaNodosExternos, __func__, "Archivos de nodos externos");

	loguear(LOG_LEVEL_TRACE, __func__, "Espero header rutina");

	//quedo a la espera del mensaje "envioRutina" que trae en el payload un bloque con la rutina de reduce
	estabaConJob = true;
	resultado = Sockets_recibir_Header(socketConectadoAlJob, &header);
	Macro_Check_And_Handle_Error(resultado <= 0, "ejecutarReduce: Error al recibir datos del header.");
	estabaConJob = false;

	//verifico que sea el mensaje "envioRutina" que trae en el payload un bloque con la rutina de mapping
	if(header_esOrden(header, "envioRutina")){

		loguear(LOG_LEVEL_TRACE, __func__, "Recibiendo rutina");

		//obtengo el payload
		estabaConJob = true;
		resultado = Sockets_recibir_Datos(socketConectadoAlJob , &buffer , header);
		Macro_Check_And_Handle_Error(resultado <= 0, "ejecutarReduce: Error al recibir datos del payload esperando orden envioRutina.");
		estabaConJob = false;

		bloque = Bloques_des_serializar(buffer);

		nombreScript = string_new();
		string_append(&nombreScript, "scriptReduce-");
		string_append(&nombreScript, __TIME__);
		string_append(&nombreScript, "-");
		string_append(&nombreScript, nombreArchivoResultado);

		loguear(LOG_LEVEL_TRACE, __func__, "Guardando Rutina:'%s' de reducing en disco", nombreScript);

		//guardo el script en un archivo en el disco
		resultado = guardarScriptEnDisco(nombreScript, bloque->contenidoBloque, GLOBAL_Nodo_Config.dir_temp, bloque->tamanioBloque);
		Macro_Check_And_Handle_Error(resultado < 0, "ejecutarReduce: Error al guardar script en disco.");

		Comun_LiberarMemoria((void**)&buffer);
		Comun_LiberarMemoria((void**)&bloque);

		fprintf(stdout, "Rutina mapping guardada en disco (%s)\n", nombreScript);
	}else{
		//Deberiamos Parar Todo porque llega Basura
		loguear(LOG_LEVEL_ERROR, __func__, "No llego correctamente la Rutina de Reduce. Llego:'%s' y se Esperaba:'envioRutina'", header.order);
		Macro_Check_And_Handle_Error(true, "No llego correctamente la Rutina de Reduce. Llego:'%s' y se Esperaba:'envioRutina'", header.order);
		//No hace falta return porque va al Handler de Error
	}

	loguear(LOG_LEVEL_TRACE, __func__, "Voy a ejecutar el reduce");

	//veo si tengo que buscar archivos a otros nodos...
	if(REDUCE_SIN_COMBINER == modo) {//puedo tener que buscar archivos externos...

		//loopeo pidiendo archivos..

		tipo_nodoExterno* nodo_externo;

		loguear(LOG_LEVEL_TRACE, __func__, "Actualmente mi lista de Archivos Local tiene '%d' elementos", listaArchivosLocales->elements_count);
		loguear(LOG_LEVEL_TRACE, __func__, "Voy a Pedir Los Archivos Externos. Son:'%d'", listaNodosExternos->elements_count);

		for (i = 0; i < listaNodosExternos->elements_count; ++i) {

			nodo_externo = (tipo_nodoExterno*) list_get(listaNodosExternos, i);

			if(strcmp(nodo_externo->nodoIP, GLOBAL_Nodo_Config.ip_nodo) == 0 && strcmp(nodo_externo->nodoPuerto, string_itoa(GLOBAL_Nodo_Config.puerto_nodo)) == 0) {
				//es este nodo, no pido el archivo
				Macro_ImprimirParaDebugConDatos("No pido el archivo porque corresponde a este nodo: %s:%s", nodo_externo->nodoIP, nodo_externo->nodoPuerto);
				//FIXME: No deberia primero revisar si no esta en la lista de archivos Locales (por las dudas que no Este) y luego agregarlo a esa Lista??
				//FIXME: Lucas podes Confirmarme??

			}else {
				//es un nodo externo
				resultado = solicitarArchivoANodo(nodo_externo);
				Macro_Check_And_Handle_Error(resultado == -1, "Error al solicitar archivo %s al nodo %s en el puerto %s.", nodo_externo->nombreArchivo, nodo_externo->nodoIP, nodo_externo->nodoPuerto)

				Macro_ImprimirParaDebug("Nombre archivo a agregar lista archivos locales: '%s'\n", nodo_externo->nombreArchivo);

				//llegó ok - lo agrego a la lista de archivos locales
				list_add(listaArchivosLocales, string_duplicate(nodo_externo->nombreArchivo));

				Macro_ImprimirParaDebug("Cantidad elementos lista archivos locales:'%d'", list_size(listaArchivosLocales));
			}
		}

		loguear(LOG_LEVEL_TRACE, __func__,"Termine de pedir todos los archivos externos. Me quedo la lista archivos locales con'%d' elementos", list_size(listaArchivosLocales));

		//destruyo la lista
		list_destroy_and_destroy_elements(listaNodosExternos, (void*)&destructor_elementoListaNodo);
		listaNodosExternos = NULL;
	}


	loguear(LOG_LEVEL_TRACE, __func__, "Me preparo para ejecutar rutina");

	//armo los paths y ejecuto la rutina llamando al método correspondiente

	pathScript = string_new();
	string_append(&pathScript, GLOBAL_Nodo_Config.dir_temp);
	string_append(&pathScript, nombreScript);

	pathArchivoFinal = string_new();
	string_append(&pathArchivoFinal, GLOBAL_Nodo_Config.dir_temp);
	string_append(&pathArchivoFinal, nombreArchivoResultado);

	loguear(LOG_LEVEL_TRACE, __func__, "Ejecuto rutina:'%s'.   El archivo final a crear sera:'%s'", pathScript, pathArchivoFinal);

	fprintf(stdout, "Ejecuto rutina: %s. Archivo final: %s\n", nombreScript, pathArchivoFinal);

	resultado = executeScript(NULL, pathArchivoFinal, pathScript, listaArchivosLocales, REDUCE);
	loguear(LOG_LEVEL_INFO, "ejecutarReduce", "El resultado de la rutina de reduce ejecutada fue: %d", resultado);

	fprintf(stdout, "Resultado de la rutina %s: %d\n", pathScript, resultado);

	//valido resultado y que el archivo existe. Respondo con "rutinaExitosa" o "rutinaFallida" dependiendo del resultado...
	//FIXME No se que mierda le pasa que si resultado es -1 (-1 < 0) da False. Que CARAJO!
	Macro_Check_And_Handle_Error((resultado < 0) || (resultado == -1), "ejecutarReduce: Rutina de reduce fallida.");

	loguear(LOG_LEVEL_TRACE, __func__, "Viendo si existe el archivo resultado de la rutina reduce:'%s'", pathArchivoFinal);

	resultado = Comun_existeArchivo(pathArchivoFinal);

	Macro_Check_And_Handle_Error(resultado < 0, "ejecutarReduce: El archivo temporal reducido no existe :(.");

	loguear(LOG_LEVEL_TRACE, __func__, "Existe el archivo resultado del reduce, avisando al Job que se termino correctamente la rutina");

	respuestaAJob = "Se ejecuto correctamente la rutina\0";
	paqueteSerializado = package_create(respuestaAJob, strlen(respuestaAJob) + 1, "rutinaExitosa", NODO);
	Macro_Check_And_Handle_Error(paqueteSerializado == NULL, "ejecutarReduce: No se pudo generar el paquete serializado para responder a la rutina.");

	resultado = Sockets_enviar_datos(socketConectadoAlJob, paqueteSerializado);
	Macro_Check_And_Handle_Error(resultado <= 0, "ejecutarReduce: Error al intentar enviar confirmación de rutina exitosa al Job.");


	//elimino lista que ya no uso
	list_destroy_and_destroy_elements(listaArchivosLocales, (void*)&destructor_elementoListaArchivo);
	listaArchivosLocales = NULL;

	//libero recursos
	Comun_LiberarMemoria((void**)&paqueteSerializado);
	Comun_LiberarMemoria((void**)&nombreArchivoResultado);
	Comun_LiberarMemoria((void**)&bloque);

	loguear(LOG_LEVEL_INFO, __func__, "Fin correcto");

	fprintf(stdout, "Rutina ejecutada correctamente. Fin de orden\n");

	return EXIT_SUCCESS;

Error_Handler:

	loguear(LOG_LEVEL_ERROR, __func__, "Ocurrio un error en algun lado de la ejecucion del proceso de reduce. Aviso al Job");

	//aviso a job del error

	respuestaAJob = "No se ejecuto correctamente la rutina\0";

	paqueteSerializado = package_create(respuestaAJob, strlen(respuestaAJob) + 1, "rutinaFallida", NODO);

	if(paqueteSerializado == NULL) {
		loguear(LOG_LEVEL_ERROR, __func__, "Ocurrio un error intentar generar el paquete para avisar al Job de la ejecucion erronea.");
	}else {
		if(estabaConJob == false) {
			resultado = Sockets_enviar_datos(socketConectadoAlJob, paqueteSerializado);
			if(resultado <= 0) {
				loguear(LOG_LEVEL_ERROR, __func__, "ejecutarReduce: Error al intentar enviar confirmacion de rutina exitosa al Job.");
			}

		}else {
			loguear(LOG_LEVEL_ERROR, __func__, "No puedo enviarle fallo de la rutina al Job porque se desconecto");
		}
	}

	//libero recursos
	Comun_LiberarMemoria((void**)&paqueteSerializado);
	Comun_LiberarMemoria((void**)&bloque);
	Comun_LiberarMemoria((void**) &nombreArchivoResultado);
	Comun_LiberarMemoria((void**) &pathArchivoFinal);
	Comun_LiberarMemoria((void**) &pathScript);
	Comun_LiberarMemoria((void**) &archivoAReducir);
	Comun_LiberarMemoria((void**) &buffer);

	if(listaArchivosLocales != NULL) {
		list_destroy_and_destroy_elements(listaArchivosLocales, (void*)&destructor_elementoListaArchivo);
	}

	if(listaNodosExternos != NULL) {
		list_destroy_and_destroy_elements(listaNodosExternos, (void*)&destructor_elementoListaNodo);
	}

	return EXIT_ERROR;
}

void ordenEjecutarReduceConCombine(tipo_socket* socketConectadoAlJob, t_header header) {

	loguear(LOG_LEVEL_INFO, __func__, "Iniciando orden");

	int resultado = ejecutarReduce(socketConectadoAlJob, header, REDUCE_CON_COMBINER);

	loguear(LOG_LEVEL_INFO, __func__, "Fin orden (%d)", resultado);

	return;
}

void ordenEjecutarReduceSinCombine(tipo_socket* socketConectadoAlJob, t_header header) {

	loguear(LOG_LEVEL_INFO, __func__, "Iniciando orden");

	int resultado = ejecutarReduce(socketConectadoAlJob, header, REDUCE_SIN_COMBINER);

	loguear(LOG_LEVEL_INFO, __func__, "Fin orden (%d)", resultado);

	return;
}

int solicitarArchivoANodo(tipo_nodoExterno* nodo_externo) {

	char* paqueteSerializado = NULL;
	tipo_socket* socketNodoDestino = NULL;
	uint32_t posicionDentroArchivo = 0;
	tipo_bloque* punteroBloque = NULL;
	char* ordenSerializadaPayload = NULL;
	char* pathArchivoGuardar = NULL;
	int punteroArchivo = 0;
	bool estabaConSocket = false;

	loguear(LOG_LEVEL_INFO, __func__, "Iniciando. voy a pedir el archivo:'%s' al nodo de IP:'%s' y puerto:'%s'", nodo_externo->nombreArchivo, nodo_externo->nodoIP, nodo_externo->nodoPuerto);

	// Armo el paquete
	paqueteSerializado = package_create(nodo_externo->nombreArchivo, strlen(nodo_externo->nombreArchivo) +1, "pedirArchivo", NODO);
	Macro_Check_And_Handle_Error(NULL == paqueteSerializado, "No se pudo serializar el paquete a enviar.");

	// Me conecto al nodo destino
	socketNodoDestino = Sockets_conectar_servidor(nodo_externo->nodoIP, nodo_externo->nodoPuerto);
	Macro_Check_And_Handle_Error(socketNodoDestino == NULL, "No se pudo establecer conexion con el nodo destino");

	// Envío el paquete
	estabaConSocket = true;
	off_t resultado = Sockets_enviar_datos(socketNodoDestino, paqueteSerializado);
	Macro_Check_And_Handle_Error(resultado <= 0, "No se pudo enviar pedido de archivo");
	estabaConSocket = false;

	Comun_LiberarMemoria((void**) &paqueteSerializado);

	t_header headerNodo;

	estabaConSocket = true;
	resultado = Sockets_recibir_Header(socketNodoDestino, &headerNodo);
	Macro_Check_And_Handle_Error(resultado <= 0, "Hubo un error en la recepcion del header de los datos del bloque de nodo");
	estabaConSocket = false;

	loguear(LOG_LEVEL_TRACE, __func__, "Recibiendo datos archivo");

	// Recibo la cantidad de Bloques y el MD5 del archivo serializados
	estabaConSocket = true;
	resultado = Sockets_recibir_Datos(socketNodoDestino, &ordenSerializadaPayload, headerNodo);
	Macro_Check_And_Handle_Error(resultado <= 0, "Hubo un problema al intentar obtener el payload del Nodo");
	estabaConSocket = false;

	// Se reciben el tamaño del archivo, la  cantidad de bloques y el MD5 del archivo
	t_datos_archivo_final datosRecibidosArchivo = deserializar_datos_archivo_final(ordenSerializadaPayload);
	uint32_t tamanioArchivo = datosRecibidosArchivo.tamanio;
	uint32_t cantidadBloques = datosRecibidosArchivo.cantidad_bloques;
	char md5[33];
	strcpy(md5 , datosRecibidosArchivo.md5 );

	loguear(LOG_LEVEL_TRACE, __func__, "Tamanio del archivo: %d \n cantidad de bloques: %d \n MD5: %s", tamanioArchivo, cantidadBloques, md5 );

	Comun_LiberarMemoria((void**)&ordenSerializadaPayload);

	pathArchivoGuardar = string_new();
	string_append(&pathArchivoGuardar, GLOBAL_Nodo_Config.dir_temp);
	string_append(&pathArchivoGuardar, nodo_externo->nombreArchivo);

	punteroArchivo = open(pathArchivoGuardar , O_WRONLY | O_NONBLOCK | O_CREAT, S_IRWXU);
	Macro_Check_And_Handle_Error(punteroArchivo < 0 , "Hubo un error al abrir el archivo %s", nodo_externo->nombreArchivo);

	loguear(LOG_LEVEL_TRACE, __func__, "Voy a empezar a atajar bloques" );

	uint32_t bloqueActual;
	for (bloqueActual = 0; bloqueActual < cantidadBloques; bloqueActual++) { // Empiezo en 0 para facilitar el write

		Macro_ImprimirParaDebug("Reciebiendo bloque '%d'\n", bloqueActual);

		//Recibo el header
		estabaConSocket = true;
		resultado = Sockets_recibir_Header(socketNodoDestino, &headerNodo);
		Macro_Check_And_Handle_Error(resultado <= 0, "Hubo un problema al intentar obtener el header de la orden del nodo");

		//Recibo el bloque serializado
		resultado = Sockets_recibir_Datos(socketNodoDestino, &ordenSerializadaPayload, headerNodo);
		Macro_Check_And_Handle_Error(resultado <= 0, "Hubo un problema al intentar obtener el payload de la orden del nodo");
		estabaConSocket = false;

		punteroBloque = Bloques_des_serializar(ordenSerializadaPayload);
		Comun_LiberarMemoria((void**)&ordenSerializadaPayload);

	//	resultado = lseek(punteroArchivo, bloqueActual * TAMANIO_BLOQUE , SEEK_SET);
		resultado = lseek(punteroArchivo, posicionDentroArchivo , SEEK_SET);
		Macro_Check_And_Handle_Error(resultado < 0 , "Hubo un error en el posicionamiento en el archivo %s", nodo_externo->nombreArchivo);

		posicionDentroArchivo += punteroBloque->tamanioBloque;

		resultado = write(punteroArchivo , punteroBloque->contenidoBloque , punteroBloque->tamanioBloque);
		Macro_Check_And_Handle_Error(resultado < 0 , "Hubo un error en la escritura en el archivo %s", nodo_externo->nombreArchivo);

		loguear(LOG_LEVEL_TRACE, __func__, "Recibi el bloque '%d' con un tamaño de: '%d'", bloqueActual, punteroBloque->tamanioBloque );

		// Antes de atender al proximo bloque libero la memoria usada por el bloque (20mb)
		Comun_LiberarMemoria((void**) &punteroBloque);
	}

	loguear(LOG_LEVEL_TRACE, __func__, "Termine de recibir los bloques del archivo, cierro el archivo" );

	resultado = close(punteroArchivo);
	punteroArchivo = 0;
	Macro_Check_And_Handle_Error(resultado < 0 , "Hubo un error en el cierre del archivo %s", nodo_externo->nombreArchivo);

	loguear(LOG_LEVEL_INFO, __func__, "Recibi el archivo. FIN");

	Sockets_cerrar_desconectar(socketNodoDestino);

	return EXIT_SUCCESS;

Error_Handler:

	loguear(LOG_LEVEL_ERROR, __func__, "Hubo error");

	if(punteroArchivo != 0) {
		resultado = close(punteroArchivo);
		if(resultado < 0) {
			loguear(LOG_LEVEL_ERROR, __func__, "No se pudo cerrar el arhcivo");
		}
		resultado = remove(pathArchivoGuardar);
		if(resultado != 0) {
			loguear(LOG_LEVEL_ERROR, __func__, "No se pudo borrar el archivo");
		}
	}

	Comun_LiberarMemoria((void**) &ordenSerializadaPayload);
	Comun_LiberarMemoria((void**) &paqueteSerializado);
	Comun_LiberarMemoria((void**) &punteroBloque);
	Comun_LiberarMemoria((void**) &ordenSerializadaPayload);

	if (estabaConSocket == false){
		Sockets_cerrar_desconectar(socketNodoDestino);
	}

	loguear(LOG_LEVEL_ERROR, __func__, "Fin error");

	return EXIT_ERROR;
}

//fin funciones para órdenes

int Archivo_EscribirBloque(const char* rutaArchivo, uint32_t numeroBloqueDondeEscribir , tipo_bloque* bloquePorEscribir) {

	off_t iError = 0;
	int punteroArchivo = 0;

	punteroArchivo = open( rutaArchivo , O_WRONLY | O_NONBLOCK | O_CREAT, S_IRWXU);
	Macro_Check_And_Handle_Error(punteroArchivo < 0 , "Hubo un error al abrir el archivo .bin");

	iError = lseek(punteroArchivo , numeroBloqueDondeEscribir * TAMANIO_BLOQUE , SEEK_SET);
	Macro_Check_And_Handle_Error(iError < 0 , "Hubo un error en el posicionamiento en el archivo .bin");

	iError = write(punteroArchivo , bloquePorEscribir->contenidoBloque , TAMANIO_BLOQUE);
	Macro_Check_And_Handle_Error(iError < 0 , "Hubo un error en la escritura en el archivo .bin");

	iError = close(punteroArchivo);
	Macro_Check_And_Handle_Error(iError < 0 , "Hubo un error en el cierre del archivo .bin");

	return 0;

Error_Handler:
	loguear(LOG_LEVEL_ERROR, __func__, "Hubo un error al escribir en el archivo:'%s", rutaArchivo);

	if( punteroArchivo != 0 ){
		if( close(punteroArchivo) == -1 ){
			loguear(LOG_LEVEL_ERROR, __func__ , "No se pudo cerrar el archivo:'%s'", rutaArchivo);
		}
	}

	return -1;
}

int ArchivoBIN_EscribirBloque(uint32_t numeroBloqueDondeEscribir , tipo_bloque* bloquePorEscribir){

	int iError = 0;
	int iErrorEscritura = 0;

	loguear(LOG_LEVEL_TRACE, __func__, "Me llamaron para escribir el bloque:'%ld'", (long int)numeroBloqueDondeEscribir);

	//Primero Valido que haya algo que escribir
	Macro_Check_And_Handle_Error((bloquePorEscribir->tamanioBloque) <= 0 , "El bloque por escribir tiene un tamaño menor o igual a 0. Tamaño:%ld" , bloquePorEscribir->tamanioBloque);

	loguear(LOG_LEVEL_DEBUG, __func__, "Pido rwlock para escribir bloque del bin");
	iError = pthread_rwlock_wrlock(GLOBAL_LockArchivoBIN);
	if (iError != 0 ){
		loguear(LOG_LEVEL_WARNING, __func__, "No pude obtener rwlock para escribir bloque del bin. Numero error:%d", iError);
		Macro_Check_And_Handle_Error(true, "Problema con mutex. Numero error:%d", iError);
		//No hace falta Return porque va al Error_Handler
	}

	loguear(LOG_LEVEL_DEBUG, __func__, "Obtuve rwlock para escribir bloque del bin");
	iErrorEscritura = Archivo_EscribirBloque(GLOBAL_Nodo_Config.nombre_bin , numeroBloqueDondeEscribir , bloquePorEscribir);
	loguear(LOG_LEVEL_DEBUG, __func__, "Devuelvo rwlock de escribir bloque del bin");

	iError = pthread_rwlock_unlock(GLOBAL_LockArchivoBIN);

	if (iError != 0 ){
		loguear(LOG_LEVEL_WARNING, __func__, "No Pude Devolver rwlock de escribir bloque del bin. Numero error:%d", iError);
		//No paro porque pudo escribirse en el Archivo BIN

	}else{
		loguear(LOG_LEVEL_DEBUG, __func__, "Devuelto rwlock de escribir bloque del bin");
	}

	//Controlo Error de la Funcion de Escritura
	Macro_Check_And_Handle_Error(iErrorEscritura == -1, "No se pudo escribir el bloque");
	return 0;

Error_Handler:
	loguear(LOG_LEVEL_ERROR, __func__, "Hubo un problema al escribir en el archivo bin el bloque:'%ld'. OJO, puede haber problemas con los MUTEX", (long int)numeroBloqueDondeEscribir);

	return -1;
}

int ArchivoBIN_LeerBloque(uint32_t numeroBloqueALeer , tipo_bloque** bloqueLeido, size_t tamanioBloque){

	int iError = 0;
	bool useMutex = false;
	FILE* archivoBin = NULL;

	uint32_t direccionFinalDeBloque = numeroBloqueALeer * TAMANIO_BLOQUE;

	loguear(LOG_LEVEL_TRACE, __func__, "Me llamaron para leer el bloque:'%ld', de tamano: '%lu'", (long int)numeroBloqueALeer, tamanioBloque);

	loguear(LOG_LEVEL_DEBUG, __func__, "Pido rwlock para leer bloque del bin");
	iError = pthread_rwlock_rdlock(GLOBAL_LockArchivoBIN);

	if (iError != 0 ){
		loguear(LOG_LEVEL_WARNING, __func__, "No pude obtener rwlock para Leer bloque del bin. Numero error:%d", iError);
		Macro_Check_And_Handle_Error(true, "Problema con mutex. Numero error:%d", iError);
		//No hace falta Return porque va al Error_Handler
	}
	useMutex = true;

	loguear(LOG_LEVEL_DEBUG, __func__, "(tid:%d) Obtuve rwlock para leer bloque del bin", process_get_thread_id());
	loguear(LOG_LEVEL_DEBUG, __func__, "(tid:%d) Llamo a func de lectura de bloques con %lu", process_get_thread_id(), direccionFinalDeBloque);

	//	*bloqueLeido = Bloques_obtener_desde_archivo_texto_nodo(GLOBAL_Nodo_Config.nombre_bin, direccionFinalDeBloque);

	// Se abre el archivo .bin unicamente para la lectura
	archivoBin = fopen(GLOBAL_Nodo_Config.nombre_bin, "r");
	Macro_Check_And_Handle_Error(archivoBin == NULL, "Problema al abrir el archivo .bin");

	// Se obtiene el descriptor del archivo
	int fileDescriptor = fileno(archivoBin);
	loguear(LOG_LEVEL_DEBUG, __func__, "(tid:%d) Abri el archivo y voy a hacer el MMAP", process_get_thread_id());

	// Si tengo el tamanio del bloque leo directamente, sino utilizo la vieja funcion
	//if (tamanioBloque != 0) {

	Macro_Check_And_Handle_Error(tamanioBloque == 0, "Se recibio el tamano del bloque nulo");

	char* punteroBloqueMemoria = mmap(NULL, tamanioBloque, PROT_READ, MAP_PRIVATE, fileDescriptor, direccionFinalDeBloque);
	Macro_Check_And_Handle_Error(punteroBloqueMemoria == MAP_FAILED, "Fallo el mappeo del bloque del archivo .bin");

	(*bloqueLeido)->tamanioBloque = tamanioBloque;
	memcpy( (*bloqueLeido)->contenidoBloque , punteroBloqueMemoria, tamanioBloque);
	iError = munmap( punteroBloqueMemoria, tamanioBloque);
	Macro_Check_And_Handle_Error(iError == -1, "Fallo el Unmappeo del bloque del archivo .bin");
	//	} else {
	//		// Y que dios nos ayude aca!!
	//		char* punteroBloqueMemoria = mmap(NULL, TAMANIO_BLOQUE, PROT_READ, MAP_PRIVATE, fileDescriptor, direccionFinalDeBloque);
	//		//Macro_Check_And_Handle_Error(bloqueLeido==((void *) -1), "Problema al realizar el mappeo del bloque");
	//		size_t tamanioFinBloque = Bloques_obtener_tamanio_bloque_en_memoria( &punteroBloqueMemoria );
	//		(*bloqueLeido)->tamanioBloque = tamanioFinBloque;
	//		memcpy( (*bloqueLeido)->contenidoBloque , punteroBloqueMemoria, tamanioFinBloque);
	//		munmap( punteroBloqueMemoria, TAMANIO_BLOQUE);
	//	}

	iError = fclose(archivoBin);
	archivoBin = NULL;
	Macro_Check_And_Handle_Error(iError != 0, "Hubo un Error al cerrar el archivo .bin");

	loguear(LOG_LEVEL_DEBUG, __func__, "(tid:%d) Devuelvo rwlock de leer bloque del bin", process_get_thread_id());

	iError = pthread_rwlock_unlock(GLOBAL_LockArchivoBIN);
	if (iError != 0 ){
		loguear(LOG_LEVEL_WARNING, __func__, "(tid:%d) NO Pude Devolver rwlock de escribir bloque del bin. Numero error:%d", process_get_thread_id(), iError);
	}else{
		loguear(LOG_LEVEL_DEBUG, __func__, "(tid:%d) Devuelto rwlock de leer bloque del bin", process_get_thread_id());
	}

	//Controlo Error de la Funcion de Lectura
	Macro_Check_And_Handle_Error(*bloqueLeido == NULL, "No se pudo leer el bloque:'%ld'", (long int)numeroBloqueALeer );

	return EXIT_SUCCESS;

  Error_Handler:

	loguear(LOG_LEVEL_ERROR, __func__, "Hubo un problema al leer del archivo bin el bloque:'%ld'. OJO, puede haber problemas con los MUTEX", (long int)numeroBloqueALeer);
	Macro_Imprimir_Error("Hubo un problema al leer del archivo bin el bloque:'%ld'", (long int)numeroBloqueALeer);

	if (archivoBin != NULL) {
		iError = fclose(archivoBin);
		if( iError != 0 ){
			loguear(LOG_LEVEL_ERROR, __func__, "No se pudo Cerrar el Archivo");
		}
	}


	if (useMutex == true) {
		iError = pthread_rwlock_unlock(GLOBAL_LockArchivoBIN);
		if (iError != 0) {
			loguear(LOG_LEVEL_WARNING , __func__ , "(tid:%d) NO Pude Devolver rwlock de escribir bloque del bin. Numero error:%d" , process_get_thread_id() , iError);
		} else {
			loguear(LOG_LEVEL_DEBUG , __func__ , "(tid:%d) Devuelto rwlock de leer bloque del bin" , process_get_thread_id());
		}
	}




	Comun_LiberarMemoria((void**)bloqueLeido);
	return -1;
}

void imprimirListaDeArchivos(t_list* lista, char* nombreFuncion, char* nombreLista) {

	int i = 0;

	for (i = 0; i < lista->elements_count; ++i) {

		loguear(LOG_LEVEL_DEBUG, nombreFuncion, "%s: %d - %s", nombreLista, i, (char*) list_get(lista, i));
	}
}

//inicia el mutex para archivo bin
int initBinMutex() {
	int numeroError = 0;

	// place our shared data in shared memory
	int prot = PROT_READ | PROT_WRITE;
	int flags = MAP_SHARED | MAP_ANONYMOUS;
	GLOBAL_LockArchivoBIN = mmap(NULL, sizeof(pthread_mutex_t), prot, flags, -1, 0);
	assert(GLOBAL_LockArchivoBIN);

	// initialise mutex so it works properly in shared memory
	pthread_mutexattr_t attr;
	numeroError = pthread_mutexattr_init(&attr);
	if (numeroError != 0) {
		loguear(LOG_LEVEL_WARNING, __func__,
				"No se Inicializo Bien los Atributos del Mutex para archivo Bin");
		return EXIT_ERROR;
	}

	numeroError = pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
	if (numeroError != 0) {
		loguear(LOG_LEVEL_WARNING, __func__,
				"No se Establecieron Bien los Atributos del Mutex para archivo Bin");
		return EXIT_ERROR;
	}

	numeroError = pthread_mutex_init(GLOBAL_LockArchivoBIN, &attr);
	if (numeroError != 0) {
		loguear(LOG_LEVEL_WARNING, __func__, "El Mutex para archivo Bin no se Inicializo Bien");
		return EXIT_ERROR;
	}

	return EXIT_SUCCESS;
}

/*
uint32_t ArchivoBin_limpiar(const char* rutaArchivo) {

	uint32_t length = 0;
	int fileDescriptor = 0;
	char* ptr = NULL;
	int iError = 0;

	length = Bloques_obtener_tamanio_archivo(rutaArchivo);
	Macro_Check_And_Handle_Error(length == -1 , "Hubo un error al intentar determinar el tamaño del archivo .bin");

	fileDescriptor = open(rutaArchivo, O_RDWR);
	Macro_Check_And_Handle_Error(fileDescriptor < 0 , "Hubo un error al intentar abrir el archivo .bin");

	ptr = mmap(NULL, length, PROT_READ | PROT_WRITE, MAP_SHARED, fileDescriptor, 0);
	Macro_Check_And_Handle_Error(ptr == -1 , "Hubo un error al intentar mappear el archivo .bin en la memoria");

	memset(ptr, 0, length);

	iError = munmap(ptr, length);
	Macro_Check_And_Handle_Error(iError == -1 , "Hubo un error al intentar limpiar el archivo .bin de la memoria");

	Comun_LiberarMemoria((void**) ptr);

	iError= close(fileDescriptor);
	Macro_Check_And_Handle_Error(iError == -1 , "Hubo un error al intentar cerrar el archivo .bin");

	return EXIT_SUCCESS;

   Error_Handler:
    Comun_LiberarMemoria((void**) ptr);
	return EXIT_ERROR;
} */
