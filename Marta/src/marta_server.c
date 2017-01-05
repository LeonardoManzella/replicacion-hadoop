#include "../headers/marta_server.h"

#include <stdio.h>
#include <commons/string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#include "../../Sockets/Biblioteca_Sockets.h"
#include "../../Serializador/Protocolo_Marta_FS.h"
#include "../../Serializador/Protocolo_Marta_JOB_Nodo.h"

#include <commons/collections/list.h>

#define RUTA_ARCHIVO_POR_PEDIR "/Prueba.txt"

#define INFINITO 	1		//Para el While del Servidor de JOBs

bool FileSystemActivo(const char IPFS[LONGITUD_CHAR_IP], const char PuertoFS[LONGITUD_CHAR_PUERTOS]) {
	uint32_t iReturn = 0;
	bool EstaDisponible = false;

	Macro_ImprimirParaDebug("Antes\n");
	Macro_ImprimirParaDebug("IP:%s  Puerto:%s  \n", IPFS, PuertoFS);
	Macro_ImprimirParaDebug("Despues\n");

	loguear(LOG_LEVEL_INFO, __func__, "Viendo si el File System Esta Activo.. Su IP:%s , Su Puerto:%s", IPFS, PuertoFS);

	tipo_socket* socketConectado = Sockets_conectar_servidor(IPFS, PuertoFS);
	if (socketConectado == NULL) {
		loguear(LOG_LEVEL_ERROR, __func__, "ERROR: No puedo conectar al FS \n");
		return false;
	}

	//Envio Peticion para Saber si FS esta Disponible
	char* peticion = package_create("Estas Disponible", strlen("Estas Disponible") + 1, "FSDisponible", MARTA);
	iReturn = Sockets_enviar_datos(socketConectado, peticion);
	if (iReturn <= 0) {
		loguear(LOG_LEVEL_ERROR, __func__, "ERROR: No puedo enviar peiticion al FS \n");
		return false;
	}

	t_header headerRecibido;
	iReturn = Sockets_recibir_Header(socketConectado, &headerRecibido);
	if (iReturn <= 0) {
		loguear(LOG_LEVEL_ERROR, __func__, "ERROR: No puedo recibir la Lista, error al recibir Header desde FS \n");
		return false;
	}

	if (headerRecibido.sender_id == FILESYSTEM) {
		if (header_esOrden(headerRecibido, "FSEstaDisponible")) {
			loguear(LOG_LEVEL_INFO, __func__, "El File System SI Esta Disponible \n");
			EstaDisponible = true;
		} else if (header_esOrden(headerRecibido, "FSNoEstaDisponible")) {
			loguear(LOG_LEVEL_WARNING, __func__, "El File System NO Esta Disponible \n");
			EstaDisponible = false;
		} else {
			loguear(LOG_LEVEL_WARNING, __func__,
					"Llego una orden del FileSystem que no se la reconoce, la orden decia: %s \n",
					headerRecibido.order);
		}
	} else {
		loguear(LOG_LEVEL_WARNING, __func__, "Llego una Orden de alguien que no reconosco, quien mando: %d \n",
				headerRecibido.sender_id);
	}

	//NOTA: Puede que llegue a haber problemas porque no sacamos el Payloadl del Buzon del Socket (igual no lo necesitamos)

	Sockets_cerrar_desconectar(socketConectado);
	return EstaDisponible;

}

t_list* FileSystemCopiasBloqueNodo(const char IPFS[LONGITUD_CHAR_IP], const char PuertoFS[LONGITUD_CHAR_PUERTOS],
		const char* ArchivoConRuta) {
	uint32_t iReturn = 0;
	char* peticion = NULL;
	char* listaSerializada = NULL;

	//Cambiado para que use Comun Liberar Memoria y Macro Check and Handler ya que tenia muchos Memory Leaks y Habia BUGs que podria devolver que se ejecuto correctamente y era mentira..

	loguear(LOG_LEVEL_INFO, __func__, "Pidiendo Lista de Copias del Archivo:'%s' al FS \n", ArchivoConRuta);

	tipo_socket* socketConectado = Sockets_conectar_servidor(IPFS, PuertoFS);
	Macro_Check_And_Handle_Error(socketConectado == NULL, "No puedo conectar al FS");

	//Envio Peticion para Que me de Bloques de un Archivo
	peticion = package_create((char*) ArchivoConRuta, string_length((char*) ArchivoConRuta) + 1, "pedirBloquesArchivo",
			MARTA);

	loguear(LOG_LEVEL_TRACE, __func__, "Enviando Peticion al FS");

	iReturn = Sockets_enviar_datos(socketConectado, peticion);
	Macro_Check_And_Handle_Error(iReturn <= 0, "No puedo enviar peiticion al FS");

	Comun_LiberarMemoria((void**) &peticion);

	loguear(LOG_LEVEL_TRACE, __func__, "Espero La lista del FS");
	//Espero La lista
	t_header headerRecibido;
	iReturn = Sockets_recibir_Header(socketConectado, &headerRecibido);
	Macro_Check_And_Handle_Error(iReturn <= 0, "No puedo recibir la Lista, error al recibir Header desde FS");

	iReturn = Sockets_recibir_Datos(socketConectado, &listaSerializada, headerRecibido);
	Macro_Check_And_Handle_Error(iReturn < 0, "Error al recibir la lista Serializada desde FS");

	loguear(LOG_LEVEL_TRACE, __func__, "Recibi una Contestacion del FS");

	//Verifico si me dijo que Esta Disponible o NO el Archivo viendo la orden que llego
	if (header_esOrden(headerRecibido, "​cancelacionPedirArchivo")) {
		loguear(LOG_LEVEL_WARNING, __func__, "El FS me dijo que el Archivo No esta Disponible: \n");
		Comun_LiberarMemoria((void**) &listaSerializada);
		Sockets_cerrar_desconectar(socketConectado);
		return NULL;

	}
	//Chequeo que no Llego Cualquier Cosa..
	Macro_Check_And_Handle_Error(!header_esOrden(headerRecibido, "respuestaBloquesArchivo"),
			"Llego cualquier cosa como orden desde el FS: %s", headerRecibido.order);

	loguear(LOG_LEVEL_TRACE, __func__, "Des-Serializando Lista");

	//Des-Serializo la Lista
	t_list* listaBloques = DesSerializar_listaCopiasBloqueNodo(listaSerializada);
	Macro_Check_And_Handle_Error(listaBloques == NULL,
			"Llego cualquier cosa como Lista desde el FS, fijate que la 'listaSerializada' empieza con \\0 \n");

	loguear(LOG_LEVEL_INFO, __func__,
			"Se recibio Correctamente la Lista del FS. Se entiende que el Archivo esta Disponible \n");

	Comun_LiberarMemoria((void**) &listaSerializada);
	Sockets_cerrar_desconectar(socketConectado);

	return listaBloques;

	Error_Handler: loguear(LOG_LEVEL_ERROR, __func__, "No se pudo Pedir al FS la Lista de Copias de Bloques \n");

	Comun_LiberarMemoria((void**) &peticion);
	Comun_LiberarMemoria((void**) &listaSerializada);

	return NULL;
}

int FileSystemSubirArchivoNodo(const char IPFS[LONGITUD_CHAR_IP], const char PuertoFS[LONGITUD_CHAR_PUERTOS],
		const char* ArchivoConRuta, const char NodoNombre[NODO_LONGITUD_NOMBRE]) {
	//Variable para Manejo de Errores
	uint32_t iNumeroError = 0;
	uint32_t tamanioSerializacion = -1;
	char* payloadSerializado = NULL;
	char* peticion = NULL;

	//Cambiado para que use Comun Liberar Memoria y Macro Check and Handler ya que tenia muchos Memory Leaks y Habia BUGs que podria devolver que se ejecuto correctamente y era mentira..

	loguear(LOG_LEVEL_INFO, __func__, "Pidiendo que suba el Archivo Final:'%s' al FS \n", ArchivoConRuta);

	//Me conecto al FS
	tipo_socket* socketConectadoAlMarta = Sockets_conectar_servidor(IPFS, PuertoFS);
	Macro_Check_And_Handle_Error(socketConectadoAlMarta == NULL, " No puedo conectar al FS\n");

	loguear(LOG_LEVEL_TRACE, __func__, "Serializando Ruta y Nombre Archivo y Nodo");

	//Serializo la Peticion
	payloadSerializado = serializar_rutaFinalYNodoContenedor((char*) ArchivoConRuta, (char*) NodoNombre,
			&tamanioSerializacion);

	//Envio Peticion para Que me de Bloques de un Archivo
	peticion = package_create(payloadSerializado, tamanioSerializacion, "pedirSubirArchivoFinal", MARTA);

	Comun_LiberarMemoria((void**) &payloadSerializado);

	loguear(LOG_LEVEL_TRACE, __func__, "Enviando Ruta y Nombre Archivo y Nodo");

	iNumeroError = Sockets_enviar_datos(socketConectadoAlMarta, peticion);
	Macro_Check_And_Handle_Error(iNumeroError <= 0, "No puedo enviar peticion al FS\n");
	Comun_LiberarMemoria((void**) &peticion);

	loguear(LOG_LEVEL_TRACE, __func__, "Esperando Respuesta");

	//Espero La repuesta
	t_header headerRecibido;
	iNumeroError = Sockets_recibir_Header(socketConectadoAlMarta, &headerRecibido);
	Macro_Check_And_Handle_Error(iNumeroError <= 0,
			"No se pudo recibir el resultado de subir el archivo, error al recibir Header desde FS\n");

	loguear(LOG_LEVEL_TRACE, __func__, "Me respondio el FS! Analizo que llego");

	//Verifico si me dijo que se pudo el archivo
	if (header_esOrden(headerRecibido, "​cancelacionSubirArchivo")) {
		loguear(LOG_LEVEL_WARNING, __func__, "El FS me dijo que el Archivo No se pudo subir: %s\n",
				headerRecibido.order);
		iNumeroError = -1;
	} else if (!header_esOrden(headerRecibido, "respuestaArchivoSubido")) {
		loguear(LOG_LEVEL_WARNING, __func__, "Llego cualquier cosa como orden desde el FS: %s\n", headerRecibido.order);
		iNumeroError = -1;
	}

	//NOTA: Puede que llegue a haber problemas porque no sacamos el Payloadl del Buzon del Socket (igual no lo necesitamos)

	//Debido a que el Marta es el Administrador del Sistema, estaria bueno que muestre por pantalla cuando se Suben los Archivos Finales
	if (iNumeroError == -1) {
		loguear(LOG_LEVEL_ERROR, __func__, "El Archivo No se subio al FS \n");
		printf(
		ANSI_COLOR_RED "- NO Se Subio al FS el Archivo Final: %s" ANSI_COLOR_RESET "\n", ArchivoConRuta);
	} else {
		loguear(LOG_LEVEL_INFO, __func__, "El Archivo se Subio Correctamente al FS \n");
		printf(
		ANSI_COLOR_GREEN "+ Se Subio al FS el Archivo Final: %s" ANSI_COLOR_RESET "\n", ArchivoConRuta);
	}

	Sockets_cerrar_desconectar(socketConectadoAlMarta);
	return iNumeroError;

	Error_Handler: loguear(LOG_LEVEL_ERROR, __func__, "No se pudo Pedir al FS Que suba el Archivo Final \n");
	printf(
	ANSI_COLOR_RED "- NO se pudo Pedir al FS Que suba el Archivo Final: %s" ANSI_COLOR_RESET "\n", ArchivoConRuta);

	Comun_LiberarMemoria((void**) &peticion);
	Comun_LiberarMemoria((void**) &payloadSerializado);

	return -1;
}

void Marta_Servidor_Iniciar(void* puertoEscuchaMartaVoid) {
	//NOTA: en esta funcion uso la Macro_Check_And_Handle_Error y su Label ErrorHandler, les recomiendo que lean sus Comentarios de como funciona en Biblioteca_Comun (van a ver que aca no hace casi nada, pero la uso como introduccion porque en funcion de orden JOB si la uso monton)

	uint32_t iNumeroError = 0;	//Variable para Manejo de Errores. Por defecto es 0 que significa sin Errores
	tipo_socket* socketEnEscucha = NULL;
	tipo_socket* socketConectadoAlCliente = NULL;
	t_header headerRecibido;

	//Aca Completo el Macro_ImprimirEstadoInicio del Main de Marta.c (Marta Padre)
	Macro_ImprimirEstadoFinal(EJECUCION_CORRECTA);
	loguear(LOG_LEVEL_INFO, __func__, "Iniciando Servidor de JOBs...");

	//Hago una conversion a variable local asi no debo hacer muchos casteos
	char* marta_PuertoEscucha = string_itoa(*(int*) (puertoEscuchaMartaVoid));

	//Inicio Servidor
	socketEnEscucha = Sockets_ponerme_escuchar(marta_PuertoEscucha);
	Macro_Check_And_Handle_Error(socketEnEscucha==NULL, "No puedo ponerme a Escuchar Peticiones de Ordenes");

	loguear(LOG_LEVEL_INFO, __func__, ">>Servidor de JOBs Iniciado \n");
	Comun_Pantalla_Separador_Destacar("Servidor de JOBs Iniciado");

	//Acepto nuevos clientes infinitamente, bloqueandose hasta que se conecte un nuevo cliente
	while (INFINITO) {
		Macro_ImprimirParaDebug("Esperando algun JOB...\n");

		//Acepto un nuevo cliente
		socketConectadoAlCliente = Sockets_aceptar_cliente(socketEnEscucha);
		//Chequeo Errores sin parar la ejecucion
		if (socketConectadoAlCliente == NULL) {
			loguear(LOG_LEVEL_ERROR, __func__, "Hubo un Problema al Aceptar un Cliente");
			continue;
		}
		loguear(LOG_LEVEL_TRACE, __func__, "Se conecto alguien al Marta, su IP es: %s y su Puerto es: %d",
				Sockets_obtener_ip_cliente(socketConectadoAlCliente),
				Sockets_obtener_puerto_cliente(socketConectadoAlCliente));

		//Aca vamos a esperar a que me llegue una peticion
		//Recibimos el header
		iNumeroError = Sockets_recibir_Header(socketConectadoAlCliente, &headerRecibido);
		//Chequeo Errores
		if (iNumeroError == 0) {
			loguear(LOG_LEVEL_WARNING, __func__,
					"Se corto la conexion apenas recibimos al cliente, No tenemos manera de saber quien era asi que no podemos hacer nada. Podria haber sido un HeartBeat. \n");
			continue;

		} else if (iNumeroError < 0) {
			loguear(LOG_LEVEL_ERROR, __func__,
					"(Linea %d) Hubo un problema con la conexion apenas recibimos al cliente, se imprimio por pantalla que paso (la funcion de Socket lo hizo) \n",
					__LINE__);
			continue;
		}

		//Veo quien se me conecto y en base a eso voy a ver que orden/peticion me mandaron para atenderla
		switch (headerRecibido.sender_id) {
		//Veo si se me conecto El Marta
		case JOB:
			loguear(LOG_LEVEL_INFO, __func__, "Llego una Orden '%s' de un JOB \n", headerRecibido.order);

			if (header_esOrden(headerRecibido, "JobNuevo")) {
				printf("- Llego JOB Nuevo. IP:'%s' Puerto:'%d'\n", Sockets_obtener_ip_cliente(socketConectadoAlCliente),
						Sockets_obtener_puerto_cliente(socketConectadoAlCliente));
				Servidor_realizarOrden_AtenderJOB(socketConectadoAlCliente, headerRecibido);
				//NOTA: Como este es un llamado entre funciones hasta que por fin se hace el FORK para el Hijo, puede que sea un poco Lento al Aceptar JOBs, en ese caso hay que modificar para que Apenas llegue un Cliente Cree un Thread Nuevo y ahi se llame a la Funcion.

			} else {
				loguear(LOG_LEVEL_WARNING, __func__,
						"Llego una orden de un JOB que no se la reconoce, la orden decia: %s \n", headerRecibido.order);
			}
			break;
		default:
			loguear(LOG_LEVEL_WARNING, __func__, "Llego una Orden de alguien que no reconosco, quien mando: %d \n",
					headerRecibido.sender_id);
			break;
		} //Fin Switch de Seleccion

	} //While Infinito

	//En Teoria nunca deberia llegar a aca debajo ni seria necesario

	pthread_exit((void*) EXIT_SUCCESS);

	Error_Handler:
	Macro_ImprimirParaDebug(
			ANSI_COLOR_RED "Debido a un Fallo Critico el Servidor de JOBs del Marta debio Cerrarse.." ANSI_COLOR_RESET "\n");
	loguear(LOG_LEVEL_ERROR, __func__, "Debido a un Fallo Critico el Servidor de JOBs del Marta debio Cerrarse.. \n");

	pthread_exit((void*) EXIT_FAILURE);
}

void Servidor_realizarOrden_AtenderJOB(tipo_socket* socketConectadoAlJOB, t_header headerRecibido) {
	uint32_t iNumeroError = 0; //Variable para Manejo de Errores. Por defecto es 0 que significa sin Errores
	const char* jobIP = Sockets_obtener_ip_cliente(socketConectadoAlJOB);
	const int jobPuerto = Sockets_obtener_puerto_cliente(socketConectadoAlJOB);
	char* datosInicialesSerializados = NULL;
	char* listaSerializada = NULL;
	t_list* listaArchivos = NULL;

	printf("-Llego un JOB al Marta\n");

	loguear(LOG_LEVEL_INFO, __func__, "Iniciando Orden");

	iNumeroError = Sockets_recibir_Datos(socketConectadoAlJOB, &datosInicialesSerializados, headerRecibido);
	Macro_Check_And_Handle_Error(iNumeroError <= 0, "No se pudo recibir los Datos Iniciales del JOB");

	tipo_Datos_Conexion_Inicial datosConexionInicial = DesSerializar_DatosConexionInicial(datosInicialesSerializados);
	Comun_LiberarMemoria((void**) &datosInicialesSerializados);

	//Convierto Datos
	tipo_job_marta datosJobMarta;
	strcpy(datosJobMarta.jobIP, jobIP);
	strcpy(datosJobMarta.jobPuerto, datosConexionInicial.jobPuerto);
	strcpy(datosJobMarta.jobRutaYNombreArchivoFinal, datosConexionInicial.rutaYnombreArchivoFinal);
	datosJobMarta.archivosAProcesar = NULL;	//Por ahora la inicializo a NULL por Seguridad, luego lo modifico mas abajo al llegarme la Lista
	datosJobMarta.tieneCombiner = datosConexionInicial.soportaCombiner;

	loguear(LOG_LEVEL_TRACE, __func__, "Job Envio-> Puerto:%s    RutaYNombreArchivoFinal:%s", datosJobMarta.jobPuerto,
			datosJobMarta.jobRutaYNombreArchivoFinal);

	//Espero que envie la Lista
	t_header headerLista;
	iNumeroError = Sockets_recibir_Header(socketConectadoAlJOB, &headerLista);
	Macro_Check_And_Handle_Error(iNumeroError <= 0, "No se pudo recibir el Header de la Lista de Archivos");

	iNumeroError = Sockets_recibir_Datos(socketConectadoAlJOB, &listaSerializada, headerLista);
	Macro_Check_And_Handle_Error(iNumeroError <= 0, "No se pudo recibir la Lista de Archivos");

	listaArchivos = DesSerializar_listaArchivos(listaSerializada);

	Comun_LiberarMemoria((void**) &listaSerializada);

	//Paso el Puntero de la Lista al "tipo_job_marta". Como pase el puntero no hace falta liberar la memoria (sino se jode)
	datosJobMarta.archivosAProcesar = listaArchivos;

	loguear(LOG_LEVEL_TRACE, __func__, "Job Envio-> Cantidad de Archivos a Trabajar:%d",
			list_size(datosJobMarta.archivosAProcesar));

	Sockets_cerrar_desconectar(socketConectadoAlJOB);

	//Aca Llamo para Crear al Marta Hijo. No Controlo Errores porque el Servidor debe seguir Funcionando.
	//Dejo que la Misma Funcion se encargue de manejar errores
	iniciarProcesoJob(datosJobMarta);

	loguear(LOG_LEVEL_TRACE, __func__, "Volvio de iniciar al JOB");

	loguear(LOG_LEVEL_INFO, __func__, "Orden Finalizada Correctamente \n");

	return;

	Error_Handler: loguear(LOG_LEVEL_ERROR, __func__,
			"Hubo un Problema al Empezar a Atender al JOB, Le envio Orden ENDJOB al JOB \n");

	//No hace Falta Manejar Error de esta Funcion
	Servidor_enviarOrden_EndJOB(jobIP, string_itoa(jobPuerto),
			"Hubo un Problema al Empezar a Atender al JOB en el Servidor de Marta");

	Comun_LiberarMemoria((void**) &datosInicialesSerializados);
	Comun_LiberarMemoria((void**) &listaSerializada);
	if (listaArchivos != NULL) {
		list_destroy_and_destroy_elements(listaArchivos, (void*) &destructor_elementoListaArchivo);
	}

	return;
}

int Servidor_enviarOrden_EndJOB(const char job_IP[LONGITUD_CHAR_IP], const char job_Puerto[LONGITUD_CHAR_PUERTOS],
		const char* mensajeEnviar) {
	uint32_t iNumeroError = 0;	//Variable para Manejo de Errores. Por defecto es 0 que significa sin Errores
	tipo_socket* socketConectadoAlJob = NULL;
	char* mensajeSerializado = NULL;

	//Como Siempre se pasa por Aca para Avisar la Terminacion al JOB, sea tanto Correcta o por Falla, Aprovecho para Loguear el Resultado de los JOBs y Mostrarlo por Pantalla
	if (string_equals_ignore_case((char*) mensajeEnviar, "Terminacion Correcta")) {
		loguear(LOG_LEVEL_INFO, __func__, "Se Termino Correctamente el JOB %s:%s", job_IP, job_Puerto);
		printf(
		ANSI_COLOR_GREEN "+ Se Termino Correctamente el JOB %s:%s \n" ANSI_COLOR_RESET "\n", job_IP, job_Puerto);
	} else {
		loguear(LOG_LEVEL_WARNING, __func__, "Se Termino MAL el JOB %s:%s Razon: '%s'", job_IP, job_Puerto,
				mensajeEnviar);
		printf(
		ANSI_COLOR_RED "- Se Termino MAL el JOB %s:%s. Razon: '%s' \n" ANSI_COLOR_RESET "\n", job_IP, job_Puerto,
				mensajeEnviar);
	}

	loguear(LOG_LEVEL_INFO, __func__, "Enviando Orden EndJOB con Razon '%s'", mensajeEnviar);

	socketConectadoAlJob = Sockets_conectar_servidor(job_IP, job_Puerto);
	Macro_Check_And_Handle_Error(socketConectadoAlJob == NULL, "No se pudo Conectar al JOB");

	mensajeSerializado = package_create((char*) mensajeEnviar, strlen(mensajeEnviar) + 1, "EndJob", MARTA);

	iNumeroError = Sockets_enviar_datos(socketConectadoAlJob, mensajeSerializado);
	Macro_Check_And_Handle_Error(iNumeroError <= 0, "No se pudo enviar la Orden al JOB");

	Comun_LiberarMemoria((void**) &mensajeSerializado);

	loguear(LOG_LEVEL_INFO, __func__, "Enviada Orden EndJOB \n");
	Sockets_cerrar_desconectar(socketConectadoAlJob);
	return 0;

	Error_Handler: loguear(LOG_LEVEL_ERROR, __func__, "No se pudo enviar la Orden EndJOB\n");
	Comun_LiberarMemoria((void**) &mensajeSerializado);
	return -1;
}

int Servidor_enviarOrden_MAP_JOB(const char job_IP[LONGITUD_CHAR_IP], const char job_Puerto[LONGITUD_CHAR_PUERTOS],
		tipo_nodo_marta* nodo, const char* archivoResultado) {
	//Variable para Manejo de Errores
	uint32_t iNumeroError = 0;
	uint32_t tamanioSerializacion = -1;
	char* payloadSerializado = NULL;
	char* peticion = NULL;
	char* datosPeticion = NULL;

	loguear(LOG_LEVEL_INFO, __func__, "Pidiendo que mapee JOB el nodo: %s\n", nodo->nodoNombre);

	//Me conecto al job
	tipo_socket* socketConectadoAlJob = Sockets_conectar_servidor(job_IP, job_Puerto);
	Macro_Check_And_Handle_Error(socketConectadoAlJob == NULL, "No puedo conectar al JOB\n");

	loguear(LOG_LEVEL_TRACE, __func__, "Serializando Nodo");

	//Serializo la Peticion
	payloadSerializado = Serializar_DatosNodo((char*) nodo->nodoIP, (char*) nodo->nodoPuerto, &tamanioSerializacion);

	//Envio Peticion para Que me de Bloques de un Archivo
	peticion = package_create(payloadSerializado, tamanioSerializacion, "ejecutarMapping", MARTA);

	Comun_LiberarMemoria((void**) &payloadSerializado);

	//Serializo datos
	payloadSerializado = Serializar_DatosMapping(nodo->numeroDeBloqueInterno, nodo->tamanioEnNodo,
			(char*) archivoResultado, &tamanioSerializacion);

	datosPeticion = package_create(payloadSerializado, tamanioSerializacion, "datosMapping", MARTA);

	Comun_LiberarMemoria((void**) &payloadSerializado);

	loguear(LOG_LEVEL_TRACE, __func__, "Enviando Nodo y Datos mapping");

	iNumeroError = Sockets_enviar_datos(socketConectadoAlJob, peticion);
	Macro_Check_And_Handle_Error(iNumeroError <= 0, "No puedo enviar peticion al JOB\n");
	Comun_LiberarMemoria((void**) &peticion);

	//Se Colgaba el JOB porque esperaba que le envies los Datos...
	iNumeroError = Sockets_enviar_datos(socketConectadoAlJob, datosPeticion);
	Macro_Check_And_Handle_Error(iNumeroError <= 0, "No puedo enviar peticion al JOB\n");
	Comun_LiberarMemoria((void**) &datosPeticion);

	loguear(LOG_LEVEL_TRACE, __func__, "Esperando Respuesta");

	//Espero La repuesta
	t_header headerRecibido;
	iNumeroError = Sockets_recibir_Header(socketConectadoAlJob, &headerRecibido);
	Macro_Check_And_Handle_Error(iNumeroError <= 0,
			"No se pudo recibir el resultado de mapping, error al recibir Header desde JOB\n");

	loguear(LOG_LEVEL_TRACE, __func__, "Me respondio el JOB! Analizo que llego");

	//Verifico si me dijo que se pudo el archivo
	if (!header_esOrden(headerRecibido, "TerminacionRutina")) {
		iNumeroError = -1;
		Macro_Check_And_Handle_Error(iNumeroError <= 0, "Llego cualquier cosa como resultado desde el JOB: %s\n",
				headerRecibido.order);
	}

	iNumeroError = Sockets_recibir_Datos(socketConectadoAlJob, &payloadSerializado, headerRecibido);
	Macro_Check_And_Handle_Error(iNumeroError <= 0, "Problema al recibir la respuesta de orden de marta");

	tipo_Datos_RespuestaMarta respuestaMarta = DesSerializar_RespuestaMarta(payloadSerializado);
	Comun_LiberarMemoria((void**) &payloadSerializado);

	if (respuestaMarta.terminacionCorrecta == TERMINACION_CORRECTA) {
		loguear(LOG_LEVEL_WARNING, __func__, "map correcto EEEEEEN %s\n", nodo->nodoNombre);
		iNumeroError = 1;
	} else {
		loguear(LOG_LEVEL_WARNING, __func__, "map incorrecto EEEEEEN %s %d\n", nodo->nodoNombre,
				respuestaMarta.terminacionCorrecta);

		iNumeroError = respuestaMarta.terminacionCorrecta;
	}

	Sockets_cerrar_desconectar(socketConectadoAlJob);
	return iNumeroError;

	Error_Handler: loguear(LOG_LEVEL_ERROR, __func__, "No se pudo mappear orden al NODO\n");
	printf(
	ANSI_COLOR_RED "- No se pudo mappear orden al NODO: %s" ANSI_COLOR_RESET "\n", nodo->nodoNombre);

	Comun_LiberarMemoria((void**) &peticion);
	Comun_LiberarMemoria((void**) &datosPeticion);
	Comun_LiberarMemoria((void**) &payloadSerializado);

	return -3;
}

int Servidor_enviarOrden_REDUCE_LOCAL_JOB(const char job_IP[LONGITUD_CHAR_IP],
		const char job_Puerto[LONGITUD_CHAR_PUERTOS], tipo_nodo_marta* nodo, t_list* listaDeArchivos,
		const char* archivoResultado) {

	//Variable para Manejo de Errores
	uint32_t iNumeroError = 0;
	uint32_t tamanioSerializacion = -1;
	char* payloadSerializado = NULL;
	char* peticion = NULL;
	char* datosPeticion = NULL;

	loguear(LOG_LEVEL_INFO, __func__, "Pidiendo que haga reduce JOB el nodo: %s\n", nodo->nodoNombre);

	//Me conecto al job
	tipo_socket* socketConectadoAlJob = Sockets_conectar_servidor(job_IP, job_Puerto);
	Macro_Check_And_Handle_Error(socketConectadoAlJob == NULL, "No puedo conectar al JOB\n");

	loguear(LOG_LEVEL_TRACE, __func__, "Serializando Nodo");

	//Serializo la Peticion
	payloadSerializado = Serializar_DatosNodo((char*) nodo->nodoIP, (char*) nodo->nodoPuerto, &tamanioSerializacion);

	peticion = package_create(payloadSerializado, tamanioSerializacion, "ejecutarReduceConCombine", MARTA);
	Comun_LiberarMemoria((void**) &payloadSerializado);

	loguear(LOG_LEVEL_TRACE, __func__, "Enviando Nodo");
	iNumeroError = Sockets_enviar_datos(socketConectadoAlJob, peticion);
	Macro_Check_And_Handle_Error(iNumeroError <= 0, "No puedo enviar nodo peticion al JOB\n");
	Comun_LiberarMemoria((void**) &peticion);

	//Serializo datos de la peticion
	payloadSerializado = Serializar_DatosReduceConCombiner(listaDeArchivos, (char*) archivoResultado,
			&tamanioSerializacion);

	datosPeticion = package_create(payloadSerializado, tamanioSerializacion, "datosReduce", MARTA);
	Comun_LiberarMemoria((void**) &payloadSerializado);

	loguear(LOG_LEVEL_TRACE, __func__, "Enviando Datos reduce");
	iNumeroError = Sockets_enviar_datos(socketConectadoAlJob, datosPeticion);
	Macro_Check_And_Handle_Error(iNumeroError <= 0, "No puedo enviar datos peticion al JOB\n");
	Comun_LiberarMemoria((void**) &datosPeticion);

	loguear(LOG_LEVEL_TRACE, __func__, "Esperando Respuesta");

	//Espero La repuesta
	t_header headerRecibido;
	iNumeroError = Sockets_recibir_Header(socketConectadoAlJob, &headerRecibido);
	Macro_Check_And_Handle_Error(iNumeroError <= 0,
			"No se pudo recibir el resultado de reduce, error al recibir Header desde JOB\n");

	loguear(LOG_LEVEL_TRACE, __func__, "Me respondio el JOB! Analizo que llego");

	//Verifico si me dijo que se pudo el archivo
	if (!header_esOrden(headerRecibido, "TerminacionRutina")) {
		iNumeroError = -1;
		Macro_Check_And_Handle_Error(iNumeroError <= 0, "Llego cualquier cosa como resultado desde el JOB: %s\n",
				headerRecibido.order);
	}

	iNumeroError = Sockets_recibir_Datos(socketConectadoAlJob, &payloadSerializado, headerRecibido);
	Macro_Check_And_Handle_Error(iNumeroError <= 0, "Problema al recibir la respuesta de orden de marta");

	tipo_Datos_RespuestaMarta respuestaMarta = DesSerializar_RespuestaMarta(payloadSerializado);
	Comun_LiberarMemoria((void**) &payloadSerializado);

	if (respuestaMarta.terminacionCorrecta == TERMINACION_CORRECTA) {
		iNumeroError = 1;
		loguear(LOG_LEVEL_WARNING, __func__, "Llego bien el reduce local del nodo: %s", nodo->nodoNombre);
	} else {
		iNumeroError = respuestaMarta.terminacionCorrecta;
		loguear(LOG_LEVEL_WARNING, __func__, "Fallo el reduce local del nodo: %s, %d", nodo->nodoNombre,
				respuestaMarta.terminacionCorrecta);
	}

	Sockets_cerrar_desconectar(socketConectadoAlJob);
	return iNumeroError;

	Error_Handler: loguear(LOG_LEVEL_ERROR, __func__, "No se pudo mappear orden al NODO\n");
	printf(
	ANSI_COLOR_RED "- No se pudo hacer reduce local orden al NODO: %s" ANSI_COLOR_RESET "\n", nodo->nodoNombre);

	Comun_LiberarMemoria((void**) &peticion);
	Comun_LiberarMemoria((void**) &datosPeticion);
	Comun_LiberarMemoria((void**) &payloadSerializado);

	return -3;
}

int Servidor_enviarOrden_REDUCE_FINAL_JOB(const char job_IP[LONGITUD_CHAR_IP],
		const char job_Puerto[LONGITUD_CHAR_PUERTOS], tipo_nodo_marta* nodo, t_list* listaDeArchivos,
		t_list* listaNodosExternos, const char* archivoResultado) {

	//Variable para Manejo de Errores
	uint32_t iNumeroError = 0;
	uint32_t tamanioSerializacion = -1;
	char* payloadSerializado = NULL;
	char* peticion = NULL;
	char* datosPeticion = NULL;

	loguear(LOG_LEVEL_INFO, __func__, "Pidiendo que haga reduce JOB el nodo: %s\n", nodo->nodoNombre);

	//Me conecto al job
	tipo_socket* socketConectadoAlJob = Sockets_conectar_servidor(job_IP, job_Puerto);
	Macro_Check_And_Handle_Error(socketConectadoAlJob == NULL, "No puedo conectar al JOB\n");

	loguear(LOG_LEVEL_TRACE, __func__, "Serializando Nodo");

	//Serializo la Peticion*****
	payloadSerializado = Serializar_DatosNodo((char*) nodo->nodoIP, (char*) nodo->nodoPuerto, &tamanioSerializacion);

	peticion = package_create(payloadSerializado, tamanioSerializacion, "ejecutarReduceSinCombine", MARTA);
	Comun_LiberarMemoria((void**) &payloadSerializado);

	loguear(LOG_LEVEL_TRACE, __func__, "Enviando Nodo reduce");
	iNumeroError = Sockets_enviar_datos(socketConectadoAlJob, peticion);
	Macro_Check_And_Handle_Error(iNumeroError <= 0, "No puedo enviar peticion al JOB\n");
	Comun_LiberarMemoria((void**) &peticion);

	//Serializo datos*****
	payloadSerializado = Serializar_DatosReduceSinCombiner(listaDeArchivos, listaNodosExternos,
			(char*) archivoResultado, &tamanioSerializacion);

	datosPeticion = package_create(payloadSerializado, tamanioSerializacion, "datosReduce ", MARTA);
	Comun_LiberarMemoria((void**) &payloadSerializado);

	loguear(LOG_LEVEL_TRACE, __func__, "Enviando Datos reduce");
	iNumeroError = Sockets_enviar_datos(socketConectadoAlJob, datosPeticion);
	Macro_Check_And_Handle_Error(iNumeroError <= 0, "No puedo enviar peticion al JOB\n");
	Comun_LiberarMemoria((void**) &datosPeticion);

	loguear(LOG_LEVEL_TRACE, __func__, "Esperando Respuesta");

	//Espero La repuesta
	t_header headerRecibido;
	iNumeroError = Sockets_recibir_Header(socketConectadoAlJob, &headerRecibido);
	Macro_Check_And_Handle_Error(iNumeroError <= 0,
			"No se pudo recibir el resultado de reduce, error al recibir Header desde JOB\n");

	loguear(LOG_LEVEL_TRACE, __func__, "Me respondio el JOB! Analizo que llego");

	//Verifico si me dijo que se pudo el archivo
	if (!header_esOrden(headerRecibido, "TerminacionRutina")) {
		loguear(LOG_LEVEL_WARNING, __func__, "Llego cualquier cosa como resultado desde el JOB: %s\n",
				headerRecibido.order);
		iNumeroError = -1;
	}

	iNumeroError = Sockets_recibir_Datos(socketConectadoAlJob, &payloadSerializado, headerRecibido);
	Macro_Check_And_Handle_Error(iNumeroError <= 0, "Problema al recibir la respuesta de orden de marta");

	tipo_Datos_RespuestaMarta respuestaMarta = DesSerializar_RespuestaMarta(payloadSerializado);
	Comun_LiberarMemoria((void**) &payloadSerializado);

	if (respuestaMarta.terminacionCorrecta == TERMINACION_CORRECTA) {
		loguear(LOG_LEVEL_TRACE, __func__, "llego bien del reduce final");
		iNumeroError = 1;
	} else {
		loguear(LOG_LEVEL_TRACE, __func__, "llego mal del reduce final %d", respuestaMarta.terminacionCorrecta);
		iNumeroError = respuestaMarta.terminacionCorrecta;
	}

	Sockets_cerrar_desconectar(socketConectadoAlJob);
	return iNumeroError;

	Error_Handler: loguear(LOG_LEVEL_ERROR, __func__, "No se pudo mappear orden al NODO\n");
	printf(
	ANSI_COLOR_RED "- No se pudo mappear orden al NODO: %s" ANSI_COLOR_RESET "\n", nodo->nodoNombre);

	Comun_LiberarMemoria((void**) &peticion);
	Comun_LiberarMemoria((void**) &datosPeticion);
	Comun_LiberarMemoria((void**) &payloadSerializado);

	return -3;
}

