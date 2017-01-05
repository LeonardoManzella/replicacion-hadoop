#include "../headers/Servidor_FileSystem.h"

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h> //Librerias para Threads
//#include <time.h>
#include <signal.h> //Para el Manejo de Señales de C
#include <libgen.h> //Para el "basename" y "dirname"
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <commons/config.h>
#include <commons/string.h>
#include <commons/collections/list.h>

#include "../../Serializador/Protocolo_Marta_FS.h"
#include "../../Serializador/Protocolo_Nodos_FS.h"
#include "../../Biblioteca_Comun/Biblioteca_Comun.h"
#include "../headers/Biblioteca_Bloques.h"
#include "../headers/fslogic.h"


//Inicializo un Mutex Global para los Logs, asi se puede loguear desde donde sea sin problemas.
pthread_mutex_t GLOBAL_mutex_interno_paraLogs = PTHREAD_MUTEX_INITIALIZER;



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
		 Servidor_loguear(LOG_LEVEL_ERROR, "threadBaseServidor" , "(Linea %d) No se pudo limpiar las Seniales por Defecto, Error Detectado: %s", __LINE__ , strerror(errno));
		 return;
	 }
	 accionSenial_SIGCHLD.sa_flags = SA_RESTART | SA_NOCLDSTOP;


	//Ahora si establesco la Accion para la Señal, a partir de ahora es automatico, no hace falta tocar nada mas
	if (sigaction( SIGCHLD, &accionSenial_SIGCHLD, NULL) ){
		Servidor_loguear(LOG_LEVEL_ERROR, "threadBaseServidor" , "(Linea %d) No se pudo redefinir la Señal SIGCHLD, Error Detectado: %s", __LINE__ , strerror(errno));
		return;
	}

	//Creo el Socket de Escucha del Servidor
	tipo_socket* socketEnEscucha = Sockets_ponerme_escuchar(sPuertoEscucha);
	//Chequeo errores
	if (socketEnEscucha==NULL) {
		Servidor_loguear(LOG_LEVEL_ERROR, "threadBaseServidor", "(Linea %d) Hubo un problema con la conexion apenas nos pusimos en escucha, se imprimio por pantalla que paso (la funcion de Socket lo hizo)", __LINE__ );
		return;
	}

	//Acepto nuevos clientes infinitamente, bloqueandose hasta que se conecte un nuevo cliente
	while ( INFINITO) {
		//Variable para Manejo de Errores
		int iNumeroError = 0;


		Macro_ImprimirParaDebug("Esperando Nuevo Cliente...\n");

		//Acepto un nuevo cliente
		tipo_socket* socketConectadoAlCliente = Sockets_aceptar_cliente(socketEnEscucha);
		//Chequeo errores
		if (socketEnEscucha == NULL) {
			Servidor_loguear(LOG_LEVEL_ERROR ,"threadBaseServidor", "(Linea %d) Hubo un problema con la conexion cuando intentamos aceptar un cliente, se imprimio por pantalla que paso (la funcion de Socket lo hizo)", __LINE__ );
			return;
		}

		//Armo el string para loguear que se conecto un cliente
		Servidor_loguear(LOG_LEVEL_TRACE, "threadBaseServidor", "Se conecto alguien al File System, su IP es: %s y su Puerto es: %d", Sockets_obtener_ip_cliente(socketConectadoAlCliente), Sockets_obtener_puerto_cliente(socketConectadoAlCliente) );


		//Creo una variable para el ID del Thread, no nos importa realmente que se pierda el ID de los Threads Hijos
		pthread_t idThread;

		//Creo un nuevo Thread para atender al cliente y le paso el socketConectadoAlCliente
		iNumeroError = pthread_create(&idThread, NULL, (void*) &threadCreadoPorCliente,(void*) socketConectadoAlCliente);

		//Chequeo Errores al crear el thread
		if (iNumeroError != 0) {
			Servidor_loguear(LOG_LEVEL_ERROR ,"threadBaseServidor", "Ocurrio un Error al crear el Thread. El Error es %s", Macro_Obtener_Errno());
			continue;
		}

		//Le digo al Thread que cuando se termine libere los recursos que uso, lo hace el solo y nos olvidamos nosotros
		iNumeroError = pthread_detach(idThread);

		//Chequeo Errores en el detach
		if (iNumeroError != 0) {
			Servidor_loguear(LOG_LEVEL_ERROR ,"threadBaseServidor", "Ocurrio un Error al hacer Detach. El Error es %s", Macro_Obtener_Errno());
			continue;
		}

	}

	//En teoria nunca ejecutaria aca abajo ni en necesario

	return;
}


//Rutina para Eliminar Procesos Zombies (se encarga automaticamente)
void Handler_Eliminar_Zombies(int valorQueNoUso) {
	//NOTA: Los Printf Comentados son Importantes, no los Borren. Les serviran para Debug si hay algun problema con el Handler


	//Les Recomiendo que no Quiten este Mensaje, porque sino no saben cuando se llamo al Handler. Tambien les Permite saber cuando se Termino Cualquier Proceso (Normal o Zombie)
	Servidor_loguear(LOG_LEVEL_TRACE, "Handler_Eliminar_Zombies" , "Se termino un Proceso (Sea Normal o Zombie) \n" );

	//printf("Voy a Eliminar Proceso Zombie.");

	//Primero Veo si es un Proceso Zombie, en caso que no lo Sea Nunca entra al While
	pid_t idProceso = waitpid((pid_t) -1 , 0 , WNOHANG);
	//printf("ID: %d\n" , (int) idProceso);

	//Elimino todos los Procesos Hijos (creados con fork) (que se Acaban de Terminar)  que encuentra
	while (idProceso > 0) {
		//printf("·");

		//Para Debug,Hago un Logueo cada ves que se termino de ejecutar.
		Servidor_loguear(LOG_LEVEL_TRACE, "Handler_Eliminar_Zombies" , "Se detecto que un Proceso de ID %d se termino, para evitar que se convierta en Zombie, se Liberaron Sus Recursos \n" , idProceso );

		//Ya que estamos (para Aprobechar Recursos y Rendimiento), veo si hay otros Procesos Terminados para Liberar sus Recursos
		idProceso = waitpid((pid_t) -1 , 0 , WNOHANG);
	}
	return;
}


void threadCreadoPorCliente(void* socket) {
	/*	Basicamente lo que hace es leer el header y en base a el lee cual es la orden que nos envian.
	 *	Luego en base a la orden llama a una funcion que realiza lo que pide la orden
	 *	Entonces hay tantas funciones como Orden
	 */


	//Para evitar hacer muchos casteos cada vez que llamamos a "socket"
	tipo_socket* socketConectadoAlCliente = (tipo_socket*) socket;

	//Variable para Controlar Errores, por defecto es 0 que significa que no hay error.
	int iNumeroError =0;
	//Variable para el header
	t_header headerRecibido;

	//Aca vamos a esperar a que me llegue una peticion
	//Recibimos el header
	iNumeroError=Sockets_recibir_Header(socketConectadoAlCliente, &headerRecibido);

	//Chequeo Errores
	if(iNumeroError==0){

		Servidor_loguear(LOG_LEVEL_WARNING, "threadCreadoPorCliente", "Se corto la conexion apenas recibimos al cliente, No tenemos manera de saber si era un Nodo o el Marta, asi que no podemos hacer nada \n" );
		return;

	}else if(iNumeroError<0){
		Servidor_loguear(LOG_LEVEL_ERROR, "threadCreadoPorCliente", "(Linea %d) Hubo un problema con la conexion apenas recibimos al cliente, se imprimio por pantalla que paso (la funcion de Socket lo hizo)", __LINE__ );
		return;
	}


	//Veo quien se me conecto y en base a eso voy a ver que orden/peticion me mandaron para tenderla
	switch (headerRecibido.sender_id) {
		//Veo si se me conecto El Marta
		case MARTA:
			if(header_esOrden(headerRecibido, "pedirBloquesArchivo")){
				Servidor_realizarOrden_pedirBloquesArchivo(socketConectadoAlCliente, headerRecibido);

			}else if(header_esOrden(headerRecibido, "pedirSubirArchivoFinal")){
				Servidor_realizarOrden_pedirSubirArchivoFinal(socketConectadoAlCliente, headerRecibido);

			}else if(header_esOrden(headerRecibido, "FSDisponible")){
				Servidor_realizarOrden_FSDisponible(socketConectadoAlCliente, headerRecibido);

			} else {
				Servidor_loguear(LOG_LEVEL_WARNING, "threadCreadoPorCliente", "Llego una orden del Marta que no se la reconoce, la orden decia: %s", headerRecibido.order );
			}
			break;

		//Veo si se me conecto Algun Nodo
		case NODO:
			if (header_esOrden(headerRecibido, "avisarNodoConectado")) {
				Servidor_realizarOrden_avisarNodoConectado(socketConectadoAlCliente, headerRecibido);

			} else {
				Servidor_loguear(LOG_LEVEL_WARNING, "threadCreadoPorCliente", "Llego una orden de un NODO que no se la reconoce, la orden decia: %s", headerRecibido.order );
			}
			break;

		default:
			//Ese Punto y Coma suelto ES NECESARIO porque no se pueden hacer declaraciones de variables apenas empieza un "case o default" (una restriccion que viene desde ANSI C)
			;
			Servidor_loguear(LOG_LEVEL_WARNING, "threadCreadoPorCliente", "Llego una Orden de alguien que no reconosco, quien mando: %d", headerRecibido.sender_id);
			break;
	}

	//Salgo de la Funcion
	pthread_exit( EXIT_SUCCESS);
}


void Servidor_loguear(t_log_level nivelDeLogueo, const char* nombreTareaFuncion, const char* textoPorLoguear, ... ){
	//Inicio Mutex
	pthread_mutex_lock(&GLOBAL_mutex_interno_paraLogs);

	//Creo mi Log, defino que NO se muestre por pantalla y defino el Nivel de Log.
		//NOTA: Si no existe el Log, lo Crea. Si ya existia agrega cosas al final
	t_log *miLog = log_create( ARCHIVO_DE_LOG, (char*) nombreTareaFuncion, false, LOG_LEVEL_TRACE);

	//Necesito juntar en un Unico String todo los char que se le pasen, sino no puedo loguear
	char* textoJuntadoParaLoguear;

	//Utilizo una Lista de Argumentos Variables y Una funcion de las Commons para juntar los Strings
	va_list listaArgumentosVariables;
	va_start(listaArgumentosVariables, textoPorLoguear);

	textoJuntadoParaLoguear = string_from_vformat(textoPorLoguear, listaArgumentosVariables);

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
			Macro_ImprimirParaDebug("Me mandaste a imprimir un nivel de Logueo que no existe, revisa los Argumentos");
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


void Servidor_realizarOrden_FSDisponible(tipo_socket* socketConectadoAlMarta, t_header headerRecibido) {
	Servidor_loguear(LOG_LEVEL_INFO, "Servidor_realizarOrden_FSDisponible", "Se Inicia la orden" );

	int iReturn = 0;
	// Verificar que esten los nodos conectados
	int cantidadNodosConectados = 0;
	iReturn = FS_ContarNodosActivos(&cantidadNodosConectados);
	if (iReturn < 0) {
		Macro_ImprimirParaDebugConDatos("Exploto al Tratar de Ver la primera Vez Cuantos Nodos hay Activos\n");
		//Como no uso mas el socket, lo cierro
		Sockets_cerrar_desconectar(socketConectadoAlMarta);
		return;
	}
	char* peticion;

	/*
	 * Modificado por Leo 2015 06 25
	 * Estaba Mal, podia Generar BUGS, Suponete el Caso donde Hay 1 solo Nodo con todas las copias del Archivo y en el archivo config las copias minimas son 5. Le Daria que el FS No esta disponible cuando en realidad se podria acceder sin problemas al Archivo.
	 * Ademas, aclararon en el FE DE ERRATAS que una vez Disponible el FS NUNCA mas se pone en No Disponible
	 *
	t_config* archivoConfig = config_create(RUTA_ARCHIVO_CONFIG_FS);
	int cantidadNodosMinimos = config_get_int_value(archivoConfig , "CANTIDAD_NODOS_MINIMOS");
	if (cantidadNodosConectados < cantidadNodosMinimos) {
		peticion = package_create(" " , 1 , "FSNoEstaDisponible" , FILESYSTEM);
	} else {
		peticion = package_create(" " , 1 , "FSEstaDisponible" , FILESYSTEM);
	}
	*/

	if (cantidadNodosConectados >= 1) {
		peticion = package_create(" " , 1 , "FSEstaDisponible" , FILESYSTEM);
	} else {
		peticion = package_create(" " , 1 , "FSNoEstaDisponible" , FILESYSTEM);
	}

	// Enviar Mensaje si se encuentra disponible
	iReturn = Sockets_enviar_datos(socketConectadoAlMarta , peticion);
	if (iReturn <= 0) {
		Macro_ImprimirParaDebugConDatos("ERROR: No puedo enviar peiticion al FS\n");
		package_destroy(peticion);
		return;
	}
	package_destroy(peticion);

	Servidor_loguear(LOG_LEVEL_INFO, "Servidor_realizarOrden_FSDisponible", "Se Termino Correctamente la Orden \n" );

	//Como no uso mas el socket, lo cierro
	Sockets_cerrar_desconectar(socketConectadoAlMarta);
	pthread_exit(EXIT_SUCCESS);
}


void Servidor_realizarOrden_pedirBloquesArchivo(tipo_socket* socketConectadoAlMarta, t_header headerRecibido) {
	Servidor_loguear(LOG_LEVEL_INFO, "Servidor_realizarOrden_pedirBloquesArchivo", "Se Inicia la orden" );

	//Variable para Controlar Errores, por defecto es 0 que significa que no hay error.
	int iNumeroError = 0;

	//Voy a retirar del socket el Payload de la orden que me envio el MARTA
	char* ordenSerializadaPayload;
	iNumeroError = Sockets_recibir_Datos(socketConectadoAlMarta, &ordenSerializadaPayload, headerRecibido);

	//Chequeo Errores
	if ( iNumeroError<=0 ) {
		Servidor_loguear(LOG_LEVEL_ERROR, "Servidor_realizarOrden_pedirBloquesArchivo", "(Linea %d) Hubo un problema al Intentar Obtener el Payload de la orden", __LINE__);
		return;
	}


	//Ahora tengo que des-serializar la orden
	//La orden (payload) en si es el nombre (y ruta dentro del MDFS) del archivo, asi que no tengo que des-serializar nada
	char* archivoRutaFS = string_new();
	char* archivoNombre = string_new();
	//Separo la Ruta SOLA sin la parte final del nombre del archivo
	string_append(&archivoRutaFS, Comun_obtenerRutaDirectorio(ordenSerializadaPayload));

	Macro_ImprimirParaDebugConDatos("RutaYNombreArchivo que Llego: %s\n", ordenSerializadaPayload);

	Servidor_loguear(LOG_LEVEL_TRACE, "Servidor_realizarOrden_pedirBloquesArchivo", "RutaYNombreArchivo que Llego: %s", ordenSerializadaPayload );

	//Obtengo solo el Nombre del archivo
	string_append(&archivoNombre, basename(ordenSerializadaPayload));

	//Como ya "des-serialice", libero la memoria
	liberar_memoria_payload(ordenSerializadaPayload);


	//Valido que el archivo este en la Carpeta que le corresponde y Que Exista la Ruta. Si hay error le envio que no esta disponible al Marta
	tipo_id archivoID = FS_Validar_Archivo_Ruta(archivoNombre, archivoRutaFS);
	//Chequeo Errores
	if (archivoID < 0) {
		Servidor_loguear(LOG_LEVEL_WARNING, "Servidor_realizarOrden_pedirBloquesArchivo" , "(Linea %d) La ruta del archivo %s no existe o no existe el archivo o no esta ubicado JUSTO en esa ruta" , __LINE__, archivoNombre );

		char* ordenSerializadaConHeader = package_create("El archivo NO esta disponible" , strlen("El archivo NO esta disponible") + 1 , "cancelacionPedirArchivo" , FILESYSTEM);
		iNumeroError = Sockets_enviar_datos(socketConectadoAlMarta , ordenSerializadaConHeader);
		if (iNumeroError <= 0) {
			Servidor_loguear(LOG_LEVEL_ERROR, "Servidor_realizarOrden_pedirBloquesArchivo" , "(Linea %d) Hubo un Error al enviarle al Marta que el archivo  %s  No estaba disponible" , __LINE__ , archivoNombre );
			package_destroy(ordenSerializadaConHeader);
			return;
		}

		//Libero la memoria de la orden
		package_destroy(ordenSerializadaConHeader);
		//Como no uso mas el socket, lo cierro
		Sockets_cerrar_desconectar(socketConectadoAlMarta);

		return;
	}

	Servidor_loguear(LOG_LEVEL_TRACE, "Servidor_realizarOrden_pedirBloquesArchivo", "Valide que me Pidieron un Archivo que existe, en una ruta que existe y que el archivo esta JUSTO en esa ruta");


	//Obtengo de la Base los Datos del Archivo para obtener la cantidad de bloques del archivo
	tipo_datos_file archivoDatos;
	iNumeroError = FILE_ConsultarId(archivoID, &archivoDatos);
	//Chequeo Errores
	if (iNumeroError < 0) {
		Servidor_loguear(LOG_LEVEL_ERROR, "Servidor_realizarOrden_pedirBloquesArchivo", "(Linea %d) Hubo un Error al enviarle al Obtener Datos archivo  %s , es raro porque recien Pude Obtener su ID asi que deberia existir", __LINE__, archivoNombre );

		char* ordenSerializadaConHeader = package_create("El archivo NO esta disponible" , strlen("El archivo NO esta disponible") + 1 , "cancelacionPedirArchivo" , FILESYSTEM);
		iNumeroError = Sockets_enviar_datos(socketConectadoAlMarta , ordenSerializadaConHeader);
		if (iNumeroError <= 0) {
			Servidor_loguear(LOG_LEVEL_ERROR, "Servidor_realizarOrden_pedirBloquesArchivo" , "(Linea %d) Hubo un Error al enviarle al Marta que el archivo  %s  No estaba disponible" , __LINE__ , archivoNombre );
			package_destroy(ordenSerializadaConHeader);
			return;
		}

		//Libero la memoria de la orden
		package_destroy(ordenSerializadaConHeader);
		//Como no uso mas el socket, lo cierro
		Sockets_cerrar_desconectar(socketConectadoAlMarta);


		return;
	}


	uint32_t archivoCantidadBloques = archivoDatos.iCantBloques;

	//Valido si el archivo esta disponible o no
	//En caso de Error aviso al Marta que el Archivo no esta Disponible
	if(archivoDatos.disponible==false){
		char* ordenSerializadaConHeader = package_create("El archivo NO esta disponible" , strlen("El archivo NO esta disponible") + 1 , "cancelacionPedirArchivo" , FILESYSTEM);
		iNumeroError = Sockets_enviar_datos(socketConectadoAlMarta , ordenSerializadaConHeader);
		if (iNumeroError <= 0) {
			Servidor_loguear(LOG_LEVEL_ERROR, "Servidor_realizarOrden_pedirBloquesArchivo" , "(Linea %d) Hubo un Error al enviarle al Marta que el archivo  %s  No estaba disponible" , __LINE__ , archivoNombre );
			package_destroy(ordenSerializadaConHeader);
			return;
		}

		//Libero la memoria de la orden
		package_destroy(ordenSerializadaConHeader);
		//Como no uso mas el socket, lo cierro
		Sockets_cerrar_desconectar(socketConectadoAlMarta);
		return;
	}

	Servidor_loguear(LOG_LEVEL_TRACE, "Servidor_realizarOrden_pedirBloquesArchivo", "Valide que el Archivo esta Disponible. Voy a armar la Lista de BloquesNodos para enviar al Marta");


	//Voy a usar una lista Dinamica (de las Commons) para guardar todos las copias, pero agrupadas de a 3 por cada numero de bloque
	t_list* listaCopiasBloqueNodo = list_create();

	bool hayErrorCritico = false; //Variable para Saber si hay problemas graves para dar error al marta.

	//Tengo que obtener de la Base los copias/pares "Bloque-Nodo" para enviarselos al MARTA. Para lo cual voy a usar un For con "archivoCantidadBloques"
	uint32_t bloqueActual;
	for (bloqueActual = 1; bloqueActual <= archivoCantidadBloques; bloqueActual++) {
		//Creo el struct que contendra hasta 3 copias para el mismo numero de bloque

		tipo_triadaCopias* triadaCopias = TriadaCopias_crear();
		/*
		//Veo si existe el Numero de Bloque Actual, sino Doy Error
		tipo_id bloqueBuscadoID = -1;
		iNumeroError = BLOQUES_buscarBloqueEspecificoArchivo(archivoID , bloqueActual , &bloqueBuscadoID);
		//Chequeo Errores
		if (iNumeroError < 0) {
			char* stringParaLoguear = string_from_format("(Linea %d)[No deberia Pasar esto porque y ya valide que el archivo existe y esta disponible] Hubo un problema al Intentar Obtener el ID Bloque %d del Archivo %s. Osea que ese Bloque No Existe" , __LINE__ , bloqueActual , archivoNombre);
			Servidor_loguear("Servidor_realizarOrden_pedirBloquesArchivo" , stringParaLoguear , LOG_LEVEL_WARNING);
			free(stringParaLoguear);
			//Hay Error Critico, seteo la variable y salgo del For
			hayErrorCritico = false;
			break;
		}*/

		//Obtengo la Cantidad de Copias del Bloque del Archivo
		int cantidadCopias = 0;
		iNumeroError = FS_CantidadCopiasBloqueArchivo(archivoID , bloqueActual , &cantidadCopias);
		//Chequeo Errores
		if (iNumeroError < 0) {
			Servidor_loguear(LOG_LEVEL_WARNING, "Servidor_realizarOrden_pedirBloquesArchivo" , "(Linea %d)[No deberia Pasar esto porque y ya valide que el archivo existe y esta disponible] Hubo un problema al Intentar Obtener la cantidad de Copias del Bloque %d del Archivo %s" , __LINE__, bloqueActual, archivoNombre );
			//Hay Error Critico, seteo la variable y salgo del For
			hayErrorCritico = true;
			break;
		}

		Servidor_loguear(LOG_LEVEL_TRACE, "Servidor_realizarOrden_pedirBloquesArchivo", "El Bloque %d tiene %d Copias Totales (entre Validas y No Validas)", bloqueActual, cantidadCopias );

		//Uso otro FOR para las copias de un mismo numero de bloque
		int copiaActual;
		for (copiaActual = 1; copiaActual <= cantidadCopias; copiaActual++) {
			// TODO: Verificar que no este explotando el encontrar una copia que se borro
			//Obtengo de a un par "Bloque-Nodo" y lo ire agregando a la Lista

			//Obtengo los Datos de la Copia del Bloque
			tipo_datos_bloques datosParBloqueNodo;
			iNumeroError = FS_ObtenerCopiaBloqueArchivo(archivoID , bloqueActual , copiaActual , &datosParBloqueNodo);
			if (iNumeroError < 0) {
				Servidor_loguear(LOG_LEVEL_WARNING, "Servidor_realizarOrden_pedirBloquesArchivo" , "(Linea %d) Hubo un problema al Intentar Obtener los Datos Asociados al Bloque %d Copia %d del Archivo %s" , __LINE__, bloqueActual,copiaActual, archivoNombre );
				//Paso a la Siguiente Copia
				continue;
			}


			//Verifico si la Copia esta Disponible o NO, para saltar a la proxima copia
			if (datosParBloqueNodo.disponible == false) {
				Servidor_loguear(LOG_LEVEL_TRACE, "Servidor_realizarOrden_pedirBloquesArchivo", "La Copia %d No esta Disponible", copiaActual );
				continue;
			}

			Servidor_loguear(LOG_LEVEL_TRACE, "Servidor_realizarOrden_pedirBloquesArchivo", "La Copia %d Si esta Disponible", copiaActual );

			//Obtengo los Datos del Nodo del cual es parte este Bloque
			tipo_datos_nodo datosNodo;
			iNumeroError = NODO_Consultar(datosParBloqueNodo.tIdNodo , &datosNodo);
			if (iNumeroError < 0) {
				Servidor_loguear(LOG_LEVEL_WARNING, "Servidor_realizarOrden_pedirBloquesArchivo" , "(Linea %d) Hubo un problema al Intentar Obtener los Datos del Nodo %s Asociado al Bloque %d Copia %d del Archivo %s" , __LINE__, datosNodo.sNombre, bloqueActual,copiaActual, archivoNombre );
				//Paso a la Siguiente Copia
				continue;
			}

			Servidor_loguear(LOG_LEVEL_TRACE, "Servidor_realizarOrden_pedirBloquesArchivo", "Esta Copia esta dentro del Nodo: %s", datosNodo.sNombre );

			//Paso/Copio los Datos entre los 2 tipos de Datos, porque yo necesito trabajar con "tipo_copia_bloqueNodo" y no "tipo_datos_nodo"
			tipo_copia_bloqueNodo parBloqueNodo;
			// --- Julian 2015-06-21
			//strcpy(parBloqueNodo.nodoNombre , datosNodo.sNombre);
			//strcpy(parBloqueNodo.nodoIP , datosNodo.nodoIP);
			//strcpy(parBloqueNodo.nodoPuerto , datosNodo.nodoPuerto);
			//parBloqueNodo.numeroBloqueDentroNodo = datosParBloqueNodo.numeroBloqueDentroNodo;
			memcpy(parBloqueNodo.nodoNombre, datosNodo.sNombre, strlen(datosNodo.sNombre)+1);
			memcpy(parBloqueNodo.nodoIP, datosNodo.nodoIP, strlen(datosNodo.nodoIP)+1);
			memcpy(parBloqueNodo.nodoPuerto, datosNodo.nodoPuerto, strlen(datosNodo.nodoPuerto)+1);
			parBloqueNodo.numeroBloqueDentroNodo = datosParBloqueNodo.numeroBloqueDentroNodo;
			parBloqueNodo.numeroBloqueDeArchivo = datosParBloqueNodo.numeroBloqueDentroArchivo;
			parBloqueNodo.sizeArchivoDentroDelBloque = datosParBloqueNodo.tamano;

			//Con el "parBloqueNodo" lo agrego al struct que contiene las 3 copias, en la posicion siguiente a la ultima vez (la primera vez lo agrega en la posicion 1)
			TriadaCopias_agregarCopia(triadaCopias , parBloqueNodo , triadaCopias->cantidadCopiasValidas+1);

			triadaCopias->cantidadCopiasValidas = copiaActual;

			// Julian 2015-06-21
			//Cuando Cargue las 3 copias del mismo bloque, Salgo del For de Copias, porque con 3 me Basta para devolverle al Marta
			//if(triadaCopias->cantidadCopiasValidas==3){
			//	break;
			//}


			//Vuelvo al FOR, para la Proxima Copia del Bloque
		}

		//Ya estan cargadas las 3 Copias (o menos), Reviso que no sea 0 la cantidad de copias validas, porque en ese caso es que almenos 1 bloque (el bloque que estoy trabajando ahora) del archivo no esta disponible, y por ende el archivo no esta disponible
		if(triadaCopias->cantidadCopiasValidas==0){
			//El archivo esta no disponible, se lo voy a decir al marta. Se cancela la orden.

			//Libero la memoria de la Triada ultima, que no se llego a usar
			TriadaCopias_destruir(triadaCopias);

			//Destruyo la lista y todos los elementos que contenia
			list_destroy_and_destroy_elements(listaCopiasBloqueNodo, (void*)&TriadaCopias_destruir);

			Servidor_loguear(LOG_LEVEL_WARNING,"Servidor_realizarOrden_pedirBloquesArchivo", "Pidieron del Marta un archivo que no esta disponible:  %s  en  %s", archivoNombre, archivoRutaFS );

			//Libero Memoria de Char*s que no necesito mas
			free(archivoNombre);
			free(archivoRutaFS);

			char* ordenSerializadaConHeader = package_create("El archivo NO esta disponible" , strlen("El archivo NO esta disponible") + 1 , "cancelacionPedirArchivo" , FILESYSTEM);
			iNumeroError = Sockets_enviar_datos(socketConectadoAlMarta , ordenSerializadaConHeader);
			if (iNumeroError <= 0) {
				Servidor_loguear(LOG_LEVEL_ERROR, "Servidor_realizarOrden_pedirBloquesArchivo" , "(Linea %d) Hubo un Error al enviarle al Marta que el archivo  %s  No estaba disponible" , __LINE__ , archivoNombre );
				package_destroy(ordenSerializadaConHeader);
				return;
			}

			//Libero la memoria de la orden
			package_destroy(ordenSerializadaConHeader);
			//Como no uso mas el socket, lo cierro
			Sockets_cerrar_desconectar(socketConectadoAlMarta);

			//Se cancela la orden.
			return;
		}else{
			//Como hay 1 copia valida almenos, lo agrego a la lista el Bloque del Archivo.
			list_add(listaCopiasBloqueNodo, triadaCopias);
		}
		//Vuelvo al FOR, para el Proximo Bloque
	}
	//Controlo que No se haya terminado el For y hayan habidos Errores Criticos, porque sino le digo al marta que no esta disponible el archivo
	if (hayErrorCritico) {
		//Destruyo la lista y todos los elementos que contenia
		list_destroy_and_destroy_elements(listaCopiasBloqueNodo , (void*) &TriadaCopias_destruir);

		Servidor_loguear(LOG_LEVEL_WARNING, "Servidor_realizarOrden_pedirBloquesArchivo" , "Pidieron del Marta un archivo que no esta disponible:  %s  en  %s" , archivoNombre , archivoRutaFS );

		//Libero Memoria de Char*s que no necesito mas
		free(archivoNombre);
		free(archivoRutaFS);


		char* ordenSerializadaConHeader = package_create("El archivo NO esta disponible" , strlen("El archivo NO esta disponible") + 1 , "cancelacionPedirArchivo" , FILESYSTEM);
		iNumeroError = Sockets_enviar_datos(socketConectadoAlMarta , ordenSerializadaConHeader);
		if (iNumeroError <= 0) {
			Servidor_loguear(LOG_LEVEL_ERROR, "Servidor_realizarOrden_pedirBloquesArchivo" , "(Linea %d) Hubo un Error al enviarle al Marta que el archivo  %s  No estaba disponible" , __LINE__ , archivoNombre );
			package_destroy(ordenSerializadaConHeader);
			return;
		}

		//Libero la memoria de la orden
		package_destroy(ordenSerializadaConHeader);
		//Como no uso mas el socket, lo cierro
		Sockets_cerrar_desconectar(socketConectadoAlMarta);

		//Se cancela la orden.
		return;
	}


	Servidor_loguear(LOG_LEVEL_TRACE, "Servidor_realizarOrden_pedirBloquesArchivo", "Termine de Cargar BloquesNodos en la Lista. Voy a Serializar la Lista");

	//Libero Memoria de Char*s que no necesito mas
	free(archivoNombre);
	free(archivoRutaFS);

	//Ahora que tengo la Lista Cargada con las Triadas, hay que serializarla
	uint32_t tamanioSerializacion = 0;
	char* listaSerializada = Serializar_listaCopiasBloqueNodo( listaCopiasBloqueNodo,&tamanioSerializacion );

	//Como ya no la uso mas, libero la memoria de la Lista Dinamica y sus Elementos
	list_destroy_and_destroy_elements(listaCopiasBloqueNodo, (void*)&TriadaCopias_destruir);

	//Ahora que esta serializada, puedo crear el paquete con header y enviarlo a Marta
	char* paqueteConHeader = package_create(listaSerializada,tamanioSerializacion, "respuestaBloquesArchivo", FILESYSTEM);

	Servidor_loguear(LOG_LEVEL_TRACE, "Servidor_realizarOrden_pedirBloquesArchivo", "Termine de Serializar la Lista. Voy a Enviarla al Marta");


	iNumeroError = Sockets_enviar_datos(socketConectadoAlMarta, paqueteConHeader);
	if (iNumeroError <= 0) {

		//Libero Memoria
		free(listaSerializada);
		package_destroy(paqueteConHeader);

		Servidor_loguear(LOG_LEVEL_ERROR, "Servidor_realizarOrden_pedirBloquesArchivo", "(Linea %d) Hubo un Error al enviarle al Marta los BloquesNodo", __LINE__ );
		return;
	}

	Servidor_loguear(LOG_LEVEL_TRACE, "Servidor_realizarOrden_pedirBloquesArchivo", "Enviada la Lista al Marta" );

	//Ahora que la envie, hago el FREE necesario de la lista serializada
	free(listaSerializada);

	//Libero la memoria del Paquete
	package_destroy(paqueteConHeader);

	Servidor_loguear(LOG_LEVEL_INFO, "Servidor_realizarOrden_pedirBloquesArchivo", "Se Finalizo Correctamente la orden \n" );
	//Como no uso mas el socket, lo cierro
	Sockets_cerrar_desconectar(socketConectadoAlMarta);

	pthread_exit(EXIT_SUCCESS);
}


void Servidor_realizarOrden_pedirSubirArchivoFinal(tipo_socket* socketConectadoAlMarta, t_header headerRecibido){
	//LEO: Tuve que Re-Organizar la Funcion por tener Muchos Memory Leaks (La arme mal Yo), Podia Retornar Sin Avisar al Marta, No controlaba si ya existia el archivo... En resumen: Tenia Mas Agujeros que un Colador..
	//No me di cuenta antes porque es un Quilombo de Codigo esta Funcion..

	//Variable para Controlar Errores, por defecto es 0 que significa que no hay error.
	int 			iNumeroError = 0;
	char* 			ordenSerializadaPayload = NULL;
	char* 			nodoNombre = NULL;
	char* 			rutaYnombreArchivoFinal = NULL;
	t_bitarray 		*tBloquesActivos = NULL;
	char 			*cBloquesActivos = NULL;
	char*			archivoFinalRutaFS = NULL;
	char* 			archivoFinalNombre = NULL;
	char* 			peticion = NULL;
	char* 			nodoIP = NULL;
	char*			nodoPuerto = NULL;
	bool 			llegueConectarNodo = false;
	char* 			ordenSerializadaConHeader = NULL;
	tipo_id 		tIdArch = 0;		//Si es 0 Significa que no Se Creo el Archivo
	tipo_bloque* 	punteroBloque = NULL;

	Servidor_loguear(LOG_LEVEL_INFO, "Servidor_realizarOrden_pedirSubirArchivoFinal", "Se Inicia la orden" );


	//Voy a RETIRAR del socket el Payload de la ORDEN que me envio el MARTA

	iNumeroError = Sockets_recibir_Datos(socketConectadoAlMarta, &ordenSerializadaPayload, headerRecibido);
	if( iNumeroError <= 0 ){
		socketConectadoAlMarta = NULL;		//Porque se Cerro Solo por "Sockets_recibir_Datos"
		Macro_Check_And_Handle_Error(true , "(Linea %d) Hubo un problema al Intentar Obtener el Payload de la orden del MARTA", __LINE__ );
		//No hace Falta Return, Va al Error_Handler Seguro
	}


	//Ahora tengo que DES-SERIALIZAR la ORDEN del Marta
	//Me tienen que enviar que Nodo tiene el archivo Final, como se llama ese archivo y en que ruta del File System lo quiere subir

	deserializar_rutaFinalYNodoContenedor(ordenSerializadaPayload, &rutaYnombreArchivoFinal, &nodoNombre);


	Macro_ImprimirParaDebug("nodoNombre:'%s' rutaYnombreArchivoFinal:'%s'\n", nodoNombre, rutaYnombreArchivoFinal);

	Servidor_loguear(LOG_LEVEL_TRACE, "Servidor_realizarOrden_pedirSubirArchivoFinal", "nodoNombre: %s rutaYnombreArchivoFinal: %s", nodoNombre, rutaYnombreArchivoFinal );

	archivoFinalRutaFS = string_new();
	archivoFinalNombre = string_new();
	//Separo la Ruta SOLA sin la parte final del nombre del archivo
	string_append(&archivoFinalRutaFS, Comun_obtenerRutaDirectorio(rutaYnombreArchivoFinal));

	//Obtengo solo el Nombre del archivo
	string_append(&archivoFinalNombre, basename(rutaYnombreArchivoFinal));

	Servidor_loguear(LOG_LEVEL_TRACE, "Servidor_realizarOrden_pedirSubirArchivoFinal", "archivoFinalRutaFS: %s archivoFinalNombre: %s", archivoFinalRutaFS, archivoFinalNombre );

	//Como ya des-serialice, libero la memoria
	Comun_LiberarMemoria((void**)&ordenSerializadaPayload);

	//VALIDO que exista RUTA el DIRECTORIO donde subir el archivo, sino, doy error
	tipo_id tIdDirDondeSubirlo = DIR_validarRuta(archivoFinalRutaFS,0);
	Macro_Check_And_Handle_Error(tIdDirDondeSubirlo == -1, "(Linea %d) No esta creada la Ruta donde hay que subir el Archivo Final, la ruta es: %s" , __LINE__ , archivoFinalRutaFS );

	Servidor_loguear(LOG_LEVEL_TRACE, __func__ , "Validada que Si Existe la Ruta a Donde subir el Archivo");

	iNumeroError = FS_Validar_Archivo_Ruta( archivoFinalNombre, archivoFinalRutaFS);
	if( (iNumeroError >= 0) || (iNumeroError == -1) || (iNumeroError == -4) || (iNumeroError == -5) ){
		Servidor_loguear(LOG_LEVEL_WARNING, __func__ , "Pidieron Subir el Archivo:'%s' Y Ya existia!", rutaYnombreArchivoFinal);
		Macro_Check_And_Handle_Error(true , "Pidieron Subir el Archivo:'%s' Y Ya existia!", rutaYnombreArchivoFinal );
		//No hace Falta Return, Va al Error_Handler Seguro
	}

	Servidor_loguear(LOG_LEVEL_TRACE, __func__, "Validado Que el Archivo No Existia Previamente en Esa Ruta");

	//Libero Memoria Char* que no necesito mas
	Comun_LiberarMemoria((void**)&archivoFinalRutaFS);


	//Con el nombre de NODO tengo que BUSCAR su IP y PUERTO (va a estar seguro en file system xq se acaba de usar)
	nodoIP = string_new();
	nodoPuerto = string_new();
	tipo_id nodoID;
	tipo_datos_nodo nodoDatos;
	int cantidadNodosConMismoNombre;

	//Obtengo el ID
	iNumeroError = NODO_BuscarNombre(nodoNombre , &nodoID , DB_SET , &cantidadNodosConMismoNombre);
	Macro_Check_And_Handle_Error(iNumeroError < 0, "(Linea %d) No esta dentro del File System el Nodo: %s. Esto esta re mal, Deberia estar el Nodo, ya que se uso recientemente en un JOB" , __LINE__ , nodoNombre);

	//Ahora si obtengo el Ip y Puerto
	iNumeroError = NODO_Consultar(nodoID , &nodoDatos);
	Macro_Check_And_Handle_Error(iNumeroError < 0 , "(Linea %d) No pude obtener la IP y Puerto del Nodo: %s. Esto esta re mal, No deberia suceder esto, ya que acaba de encontrar al nodo" , __LINE__ , nodoNombre);

	//Traspaso los Datos a variables locales
	string_append(&nodoIP, nodoDatos.nodoIP);
	string_append(&nodoPuerto, nodoDatos.nodoPuerto);

	Servidor_loguear(LOG_LEVEL_TRACE, "Servidor_realizarOrden_pedirSubirArchivoFinal", "Encontre el Nodo al cual debo Pedirle el archivo. nodoIP: %s nodoPuerto: %s", nodoIP, nodoPuerto );



	//Me CONECTO al NODO, si no pude conectarme lo considero Caido
	llegueConectarNodo = true;

	tipo_socket* socketConectadoAlNodo = Sockets_conectar_servidor(nodoIP,nodoPuerto);
	//Controlo Errores
	Macro_Check_And_Handle_Error(socketConectadoAlNodo==NULL, "No me Pude Conectar al Nodo");

	Servidor_loguear(LOG_LEVEL_TRACE, "Servidor_realizarOrden_pedirSubirArchivoFinal", "Me pude Conectar al Nodo, asi que esta Disponible");

	//Le ENVIO al NODO la ORDEN "pedirSubirArchivoFinal".

	ordenSerializadaConHeader = package_create(archivoFinalNombre, strlen(archivoFinalNombre)+1, "pedirSubirArchivoFinal", FILESYSTEM);
	iNumeroError = Sockets_enviar_datos(socketConectadoAlNodo, ordenSerializadaConHeader);
	Macro_Check_And_Handle_Error(iNumeroError <= 0, "(Linea %d) No se pudo enviar la Orden al Nodo: %s en IP: %s Puerto: %s . Hay mas informacion en Consola (por funciones de Sockets)", __LINE__, nodoNombre, nodoIP, nodoPuerto);


	Servidor_loguear(LOG_LEVEL_TRACE, "Servidor_realizarOrden_pedirSubirArchivoFinal", "Pude enviarle la orden 'pedirSubirArchivoFinal'. Ahora espero que me Envie Contestaciones");

	//Libero la memoria de la orden
	Comun_LiberarMemoria((void**)&ordenSerializadaConHeader);

	//ESPERO a que me ENVIE el tamaño del archivo, la cantidad de Bloques y el MD5 del Archivo
	t_header headerNODO;
	//Recibo el header
	iNumeroError = Sockets_recibir_Header(socketConectadoAlNodo, &headerNODO);
		//Chequeo Errores
	Macro_Check_And_Handle_Error(iNumeroError <= 0,  "(Linea %d) Hubo un problema al Intentar Obtener el Header de la orden del Nodo: %s en IP: %s Puerto: %s , se lo considero como Caido", __LINE__, nodoNombre, nodoIP, nodoPuerto );


	//RECIBO la cantidad de Bloques y el MD5 del Archivo serializados
	iNumeroError = Sockets_recibir_Datos(socketConectadoAlNodo, &ordenSerializadaPayload, headerNODO);
	//Chequeo Errores
	Macro_Check_And_Handle_Error(iNumeroError <= 0, "(Linea %d) Hubo un problema al Intentar Obtener el Payload de la orden del Nodo: %s en IP: %s Puerto: %s , se lo considero como Caido", __LINE__, nodoNombre, nodoIP, nodoPuerto );


	//Ahora tengo que DES-SERIALIZAR los DATOS que envio el Nodo
	//Me tienen que enviar el tamaño del archivo, la  cantidad de Bloques y el MD5 del Archivo
	t_datos_archivo_final datosRecibidosArchivo = deserializar_datos_archivo_final(ordenSerializadaPayload);
	uint32_t tamanioArchivo = datosRecibidosArchivo.tamanio;
	uint32_t cantidadBloques = datosRecibidosArchivo.cantidad_bloques;
	char md5[33];
	strcpy(md5 , datosRecibidosArchivo.md5 );

	Macro_ImprimirParaDebugConDatos("tamanioArchivo: %d \n cantidadBloques: %d \n md5: %s", tamanioArchivo, cantidadBloques, md5);

	Servidor_loguear(LOG_LEVEL_TRACE, "Servidor_realizarOrden_pedirSubirArchivoFinal", "tamanioArchivo: %d \n cantidadBloques: %d \n md5: %s", tamanioArchivo, cantidadBloques, md5 );

	//Verifico que sean Valido los Datos que llegaron
	//NOTA: el -1 de MD5 es por el Caracter de \0 no debo contarlo
	Macro_Check_And_Handle_Error((tamanioArchivo <= 0) || (cantidadBloques <= 0) || (string_length(md5)!=(MD5_LENGTH -1)), "Llego Cualquier Cosa de Datos de Archivo del Nodo. Cancelo Subir archivo" );

	//Como ya des-serialice, libero la memoria
	Comun_LiberarMemoria((void**)&ordenSerializadaPayload);


	//Verificamos que Halla Espacio Suficiente para esa Cantidad de Bloques
	bool HayEspacio = false;
	tipo_id **NodosReservados = NULL;
	HayEspacio = FS_VerificarEspacioParaCopias(cantidadBloques,CANTIDAD_COPIAS_MINIMAS, &NodosReservados);
	Macro_Check_And_Handle_Error( HayEspacio == false, "No hay Espacio Suficiente Para el Archivo:'%s'", rutaYnombreArchivoFinal);


	//SUBO el ARCHIVO a la Base de Datos, asi ya puedo empezar a cargarle los Bloques

	// TODO
	// FIXME
	// Antes de cargar el archivo, verifico que no exista ya uno con el mismo nombre
	tipo_datos_file datosDelArchivo;
	while ((FILE_BuscarNombre(archivoFinalNombre, &tIdArch, DB_NEXT_DUP, NULL)) != DB_NOTFOUND) {
		FILE_ConsultarId(tIdArch,&datosDelArchivo);
		if (datosDelArchivo.dirPadre == tIdDirDondeSubirlo) {
			// Se encontro un archivo Duplicado
			llegueConectarNodo = false;		//Asi no Desactiva al Nodo, solo es en este caso Particular

			//Cierro la Conexion
			Sockets_cerrar_desconectar(socketConectadoAlNodo);
			socketConectadoAlNodo = NULL;

			Macro_Check_And_Handle_Error(true , "(Linea %d) Se encontro que el archivo '%s' esta duplicado", __LINE__, rutaYnombreArchivoFinal );
			break;
		}
	}


	// Creo el Archivo con su MD5, me devolvera por referencia el ID del archivo
	iNumeroError = FS_CrearIdArchivo(archivoFinalNombre, md5, tIdDirDondeSubirlo, tamanioArchivo, &tIdArch);
	//Chequeo Errores
	if (iNumeroError < 0) {
		llegueConectarNodo = false;		//Asi no Desactiva al Nodo, solo es en este caso Particular

		//Cierro la Conexion
		Sockets_cerrar_desconectar(socketConectadoAlNodo);
		socketConectadoAlNodo = NULL;

		Macro_Check_And_Handle_Error(true , "(Linea %d) Hubo un problema al Intentar Crear el archivo: %s", __LINE__, rutaYnombreArchivoFinal );
		//No hace Falta Return, Va al Error_Handler Seguro
	}

	Comun_LiberarMemoria((void**)&archivoFinalNombre);


	Servidor_loguear(LOG_LEVEL_TRACE, "Servidor_realizarOrden_pedirSubirArchivoFinal", "Ahora empiezo a Atajar los Bloques para Subirlos al FS (enviar Copia a otros Nodos)");


	//Ahora EMPIEZO a ATAJAR los BLOQUES en un For, si llegara a haber algun problema, considero al Nodo como Caido y marco deshabilitado al Archivo
	uint32_t bloqueActual;
	for (bloqueActual = 1; bloqueActual <= cantidadBloques; bloqueActual++) {

		//ESPERO un BLOQUE para CARGARLO al FS y ENVIARLO a los NODOS


		//Recibo el header
		iNumeroError = Sockets_recibir_Header(socketConectadoAlNodo, &headerNODO);
		Macro_Check_And_Handle_Error(iNumeroError <= 0, "(Linea %d) Hubo un problema al Intentar Obtener el Header de la orden del Nodo: %s en IP: %s Puerto: %s , se lo considero como Caido", __LINE__, nodoNombre, nodoIP, nodoPuerto);


		//Recibo el Bloque Serializado
		iNumeroError = Sockets_recibir_Datos(socketConectadoAlNodo, &ordenSerializadaPayload, headerNODO);
		//Chequeo Errores
		Macro_Check_And_Handle_Error(iNumeroError <= 0, "(Linea %d) Hubo un problema al Intentar Obtener el Payload de la orden del Nodo: %s en IP: %s Puerto: %s , se lo considero como Caido", __LINE__, nodoNombre, nodoIP, nodoPuerto );

		punteroBloque = Bloques_des_serializar(ordenSerializadaPayload);
		//Libero la memoria ahora, porque son 20mb el Char*
		Comun_LiberarMemoria((void**)&ordenSerializadaPayload);

		Servidor_loguear(LOG_LEVEL_TRACE, "Servidor_realizarOrden_pedirSubirArchivoFinal", "Recibi el Bloque %d con un Tamaño de: %d. Ahora voy a Enviar a los Nodos para generar las Copias", bloqueActual, punteroBloque->tamanioBloque );

		//Ordena al File System para agarre el Bloque, elija a que nodos enviar y en que bloques
		//iNumeroError = FS_CargarBloqueDeArchivo( tIdArch, bloqueActual, punteroBloque, CANTIDAD_COPIAS_MINIMAS);

		char mensajecopia[80];
		sprintf(mensajecopia,"Copiando bloque %d de %d (%d copias de cada uno)",bloqueActual, cantidadBloques, CANTIDAD_COPIAS_MINIMAS);
		Comun_ImprimirMensajeConBarras(mensajecopia);
		printf("\n");
		char* md5bloque = Comun_obtener_MD5_Bloque(punteroBloque,true);
		printf("MD5 del bloque: %s\n", md5bloque);
		iNumeroError = FS_CargarBloqueDeArchivo2(tIdArch, NodosReservados[bloqueActual-1], punteroBloque, bloqueActual, CANTIDAD_COPIAS_MINIMAS, md5bloque);
		if (iNumeroError < 0) {
			bool bResultado;
			bResultado = FS_ReasignarEspacioParaCopias(cantidadBloques, bloqueActual, CANTIDAD_COPIAS_MINIMAS, &NodosReservados);
			printf("Intento reasignar los bloques\n");
			if (bResultado == false) {

				Servidor_loguear(LOG_LEVEL_ERROR, "Servidor_realizarOrden_pedirSubirArchivoFinal", " Hubo un Error al copiar los bloques a los Nodos del Archivo:'%s' .Borro el Archivo y todos sus bloques ya cargados", rutaYnombreArchivoFinal );

				llegueConectarNodo = false;		//Asi no Desactiva al Nodo, solo es en este caso Particular

				//Cierro la Conexion
				Sockets_cerrar_desconectar(socketConectadoAlNodo);
				socketConectadoAlNodo = NULL;

				Macro_Check_And_Handle_Error(true , "Hubo Un problema al Cargar el Bloque %d Del Archivo:'%s'", bloqueActual, rutaYnombreArchivoFinal);
				//No hace Falta Return, Va al Error_Handler Seguro
			} else {
				iNumeroError = 0;
				bloqueActual--;
			}
		}

		//Chequeo si hubo errores al copiar los bloques en los nodos
		if( iNumeroError <0 ){
			Servidor_loguear(LOG_LEVEL_ERROR, "Servidor_realizarOrden_pedirSubirArchivoFinal", " Hubo un Error al copiar los bloques a los Nodos del Archivo:'%s' .Borro el Archivo y todos sus bloques ya cargados", rutaYnombreArchivoFinal );

			llegueConectarNodo = false;		//Asi no Desactiva al Nodo, solo es en este caso Particular

			//Cierro la Conexion
			Sockets_cerrar_desconectar(socketConectadoAlNodo);
			socketConectadoAlNodo = NULL;

			Macro_Check_And_Handle_Error(true , "Hubo Un problema al Cargar el Bloque %d Del Archivo:'%s'", bloqueActual, rutaYnombreArchivoFinal);
			//No hace Falta Return, Va al Error_Handler Seguro
		}

		Servidor_loguear(LOG_LEVEL_TRACE, "Servidor_realizarOrden_pedirSubirArchivoFinal", "Se Cargo Correctamente el Bloque");

		//Antes de atender al proximo bloque libero la memoria usada por el bloque (20mb)
		Comun_LiberarMemoria((void**)&punteroBloque);

		//Ahora Vuelvo al FOR
	}

	Servidor_loguear(LOG_LEVEL_TRACE, "Servidor_realizarOrden_pedirSubirArchivoFinal", "Listo Todos los Bloques, Procedo a Poner en Disponible al Archivo");

	//Cierro la Conexion porque Ya no necesito Comunicarme Mas al Nodo
	Sockets_cerrar_desconectar(socketConectadoAlNodo);
	socketConectadoAlNodo = NULL;

	// Libero la memoria de los nodos reservados
	FS_LiberarEspacioReservado(cantidadBloques,CANTIDAD_COPIAS_MINIMAS,NodosReservados);
	Comun_LiberarMemoria((void**)&NodosReservados);

	//Llegado a este punto se copiaron en los nodos todos los bloques, entonces el archivo esta listo para ser usado

	// Creo un bit array con la cantidad de bloques del archivo y los pongo todos en 1
	int iCantBytesBitArray = (cantidadBloques / 8) + 1;
	cBloquesActivos = malloc(sizeof(char) * iCantBytesBitArray);
	memset(cBloquesActivos , 1 , sizeof(char) * iCantBytesBitArray);
	tBloquesActivos = bitarray_create(cBloquesActivos , sizeof(char) * iCantBytesBitArray);

	iNumeroError = FILE_PonerDisponible(tIdArch , tBloquesActivos , cantidadBloques);
	Macro_Check_And_Handle_Error(iNumeroError < 0, "Hubo un Error al poner Disponible al Archivo .Borro el Archivo y todos sus bloques ya cargados" );


	//Envio Respuesta de que el archivo se subio exitosamente
	peticion = package_create(" " , 1 , "respuestaArchivoSubido" , FILESYSTEM);
	iNumeroError = Sockets_enviar_datos(socketConectadoAlMarta , peticion);
	if (iNumeroError <= 0) {
		socketConectadoAlMarta = NULL;
		Macro_Check_And_Handle_Error(true ,"No puedo enviar la orden a Marta\n");
		//No hace Falta Return, Va al Error_Handler Seguro
	}



	//Libero Memoria de Chars* que no utilizo mas
	Comun_LiberarMemoria((void**)&peticion);
	Comun_LiberarMemoria((void**)&nodoNombre);
	Comun_LiberarMemoria((void**)&rutaYnombreArchivoFinal);
	Comun_LiberarMemoria((void**)&nodoIP);
	Comun_LiberarMemoria((void**)&nodoPuerto);


	Servidor_loguear(LOG_LEVEL_INFO, __func__, "Archivo Subido, Disponible y Listo para Usar. Se Finaliza Correctamente la orden \n" );

	//Como no uso mas el socket, lo cierro
	Sockets_cerrar_desconectar(socketConectadoAlMarta);

	pthread_exit(EXIT_SUCCESS);




Error_Handler:
	Servidor_loguear(LOG_LEVEL_ERROR, __func__,"No se Pudo Subir el Archivo Final. Aviso al Marta que el Archivo No se Subio");

	Comun_LiberarMemoria((void**)&peticion);		//Por Si venia Con Algo..

	//En caso que no Sea NULL el Socket (solo pasaria si intente enviar el mensaje de Respuesta OK o Apenas Empezamos) le envio al Marta que se Cancelo la Subida del Archivo
	if (socketConectadoAlMarta != NULL) {
		//Envio Respuesta de que el archivo NO se subio exitosamente
		peticion = package_create(" " , 1 , "​cancelacionSubirArchivo" , FILESYSTEM);
		iNumeroError = Sockets_enviar_datos(socketConectadoAlMarta , peticion);
		if (iNumeroError <= 0) {
			Servidor_loguear(LOG_LEVEL_ERROR , __func__ , "No puedo enviar la orden a Marta\n");
		}
		Comun_LiberarMemoria((void**) &peticion);
	}


	//Si pude llegar a Saber el Nombre del Nodo Y Llegue a la parte de Conectarme al Nodo, Significa que Exploto en alguna conexion con el Nodo y debo desactivarlo
	if ((nodoNombre != NULL) && (llegueConectarNodo == true)) {
		Servidor_loguear(LOG_LEVEL_ERROR, __func__ , "Hubo un Problema con el Nodo '%s' y se lo Desactivo" , nodoNombre);

		//El NODO esta caido, hay que marcarlo como No Disponible en la Base de Datos
		iNumeroError = FS_DesactivarNodo(nodoNombre);
		if (iNumeroError < 0) {
			Servidor_loguear(LOG_LEVEL_ERROR, __func__ , "No se pudo Desactivar al Nodo %s",nodoNombre );
		}
	}


	//Ahora Vemos si Hay que Borrar el Archivo y Todos sus Bloques porque dio Error
	//Si es 0 Significa que no Se Creo el Archivo
	if( tIdArch != 0){
		Servidor_loguear(LOG_LEVEL_ERROR, __func__ , "Hubo un Problema al Cargar el Archivo:'%s' y Debi Eliminarlo", rutaYnombreArchivoFinal);

		iNumeroError = FS_EliminarArchivo(tIdArch);
		//Chequeo que se elimine correctamente el Archivo y sus Bloques
		if( iNumeroError < 0 ){
			Servidor_loguear(LOG_LEVEL_ERROR, "Servidor_realizarOrden_pedirSubirArchivoFinal", "(Linea %d) Hubo un Error al Borrar el archivo Final y todos sus bloques asociados", __FILE__ );
		}
	}

	//Libero Toda la Memoria Dinamica Utilizada
	Comun_LiberarMemoria((void**)&ordenSerializadaPayload);
	Comun_LiberarMemoria((void**)&archivoFinalRutaFS);
	Comun_LiberarMemoria((void**)&ordenSerializadaConHeader);
	Comun_LiberarMemoria((void**)&nodoNombre);

	//Libero la memoria usada por el bloque (20mb)
	Comun_LiberarMemoria((void**)&punteroBloque);

	Comun_LiberarMemoria((void**)&rutaYnombreArchivoFinal);
	Comun_LiberarMemoria((void**)&archivoFinalNombre);
	Comun_LiberarMemoria((void**)&nodoIP);
	Comun_LiberarMemoria((void**)&nodoPuerto);

	Comun_LiberarMemoria((void**)&cBloquesActivos);


	if (tBloquesActivos != NULL) {
		bitarray_destroy(tBloquesActivos);
	}


	pthread_exit((void*)EXIT_FAILURE);

	Servidor_loguear(LOG_LEVEL_INFO, __func__, "Se Finaliza Con Errores la orden \n" );
}





char* Servidor_realizarOrden_avisarNodoConectado(tipo_socket* socketConectadoAlNodo, t_header headerRecibido){
	Servidor_loguear(LOG_LEVEL_INFO, "Servidor_realizarOrden_avisarNodoConectado", "Se Inicia la orden" );

	//Variable para Controlar Errores, por defecto es 0 que significa que no hay error.
	int iNumeroError = 0;

	//Voy a RETIRAR del socket el Payload de la ORDEN (me avisa quien es) que me envio el NODO
	char* ordenSerializadaPayload = NULL;
	iNumeroError = Sockets_recibir_Datos(socketConectadoAlNodo, &ordenSerializadaPayload, headerRecibido);

	//Chequeo Errores
	if (iNumeroError <= 0) {
		Servidor_loguear(LOG_LEVEL_ERROR, "Servidor_realizarOrden_avisarNodoConectado", "(Linea %d) Hubo un problema al Intentar Obtener el Payload de la orden del NODO", __LINE__ );
		return NULL;
	}


	//Ahora Des-Serializo la orden, la cual debe incluir: nombreNodo, IP, Puerto, Numero de Bloques de su Archivo BIN
	t_datos_nodo nodoRecibidoDatos = deserializar_datos_nodo(ordenSerializadaPayload);
	char nodoNombre[NODO_LONGITUD_NOMBRE];	//Esta es una Variable Especial, la razon de su existencia es para retornar el Nombre del Nodo, asi lo puede usar en el Main_File_System, ya van a ver que le hago un duplicate
	strcpy(nodoNombre ,nodoRecibidoDatos.nombre_nodo);
	char nodoIP[LONGITUD_CHAR_IP];
	strcpy(nodoIP , nodoRecibidoDatos.ip);
	char nodoPuerto[LONGITUD_CHAR_PUERTOS];
	strcpy(nodoPuerto , string_itoa(nodoRecibidoDatos.puerto) );
	uint32_t nodoNumeroBloques = nodoRecibidoDatos.numero_bloques;

	Macro_ImprimirParaDebugConDatos("nodoNombre: %s   nodoIP: %s   nodoPuerto: %s   nodoNumeroBloques: %d", nodoNombre, nodoIP, nodoPuerto, nodoNumeroBloques);

	Servidor_loguear(LOG_LEVEL_TRACE, "Servidor_realizarOrden_avisarNodoConectado", "nodoNombre: %s    nodoIP: %s    nodoPuerto: %s    nodoNumeroBloques: %d", nodoNombre, nodoIP, nodoPuerto, nodoNumeroBloques );

	//Como ya "des-serialice", libero la memoria
	liberar_memoria_payload(ordenSerializadaPayload);

	//Ahora verifico si el Nodo ya existia previamente (es un nodo reconectado) o no (es nuevo)
	tipo_id nodoID;
	int cantidadNodosConMismoNombre;
	bool nodoExistia = false; //Por defecto creo que no Existe (es lo comun)
	iNumeroError = NODO_BuscarNombre(nodoNombre, &nodoID, DB_SET , &cantidadNodosConMismoNombre);

	//Chequeo la Existencia en base al Numero de Error
	if(iNumeroError<0){
		//El nodo NO existe
		nodoExistia = false;
		Servidor_loguear(LOG_LEVEL_TRACE, "Servidor_realizarOrden_avisarNodoConectado", "Se detecto que este Nodo No Existia Previamente dentro del FS (es Nuevo)");
	}else{
		//El Nodo SI existia
		nodoExistia = true;
		Servidor_loguear(LOG_LEVEL_TRACE, "Servidor_realizarOrden_avisarNodoConectado", "Se detecto que este Nodo Ya Existia Previamente dentro del FS (es Reconectado)");
	}

	tipo_datos_nodo nodoDatosViejos;

	if (nodoExistia == true) {
		iNumeroError = NODO_Consultar(nodoID, &nodoDatosViejos);
		if (iNumeroError < 0) {
			Sockets_cerrar_desconectar(socketConectadoAlNodo);
			Macro_ImprimirParaDebug("Error Consultar Datos Nodo para ver su Ip y Puerto Viejo");
			return NULL;
		}

		//Aca controlamos si un NODO es valido para reconectar o no basandonos en la IP y Puerot Viejo que tenia antes
		if ( (!string_equals_ignore_case(nodoIP, nodoDatosViejos.nodoIP)) || (!string_equals_ignore_case(nodoPuerto, nodoDatosViejos.nodoPuerto)) ) {
			Servidor_loguear(LOG_LEVEL_ERROR, __func__, "El Nodo %s Ip %s Puerto %s quizo Reconectarse Pero Tenia una IP o Puerto Distinto al de la Ultima Vez (eran IP:%s Puerto:%s)", nodoNombre, nodoIP, nodoPuerto, nodoDatosViejos.nodoIP, nodoDatosViejos.nodoPuerto);
			Sockets_cerrar_desconectar(socketConectadoAlNodo);
			return NULL;
		}


		//Actualizo el Tamanio del Archivo BIN y sus Bloques Libres
		int BloquesTotalesViejos = nodoDatosViejos.bloques_totales;
		int cantidadDeBloquesAgregados = nodoNumeroBloques - BloquesTotalesViejos;
		int BloquesLibresViejos = nodoDatosViejos.bloques_libres;

		nodoDatosViejos.bloques_totales 	= nodoNumeroBloques;
		nodoDatosViejos.bloques_libres 		= BloquesLibresViejos + cantidadDeBloquesAgregados;
		nodoDatosViejos.bloques_reservados 	= 0;		//Por las dudas se lo pongo aca.

		//(Considerando al Nodo como Valido) Marco todos los bloques asociados al Nodo como validos y listos para usar
		iNumeroError = FS_ActivarNodo(nodoNombre);
		//Chequeo Errores
		if (iNumeroError < 0) {
			Servidor_loguear(LOG_LEVEL_ERROR, "Servidor_realizarOrden_avisarNodoConectado" , "(Linea %d) Hubo un problema al Intentar Reconectar a la Base de Datos al Nodo %s Ip %s Puerto %s" , __LINE__, nodoNombre, nodoIP, nodoPuerto );
			Sockets_cerrar_desconectar(socketConectadoAlNodo);
			return NULL;
		}

		//NOTA: Lo puse aca abajo para que no haga cagada con lo de recorrer bloques en Activar Nodo. Va a funcionar igual
		//Actualizo los Cambios en la Base de Datos
		iNumeroError = NODO_Modificar(nodoID, nodoDatosViejos);
		//Chequeo Errores
		if (iNumeroError < 0) {
			Servidor_loguear(LOG_LEVEL_ERROR, __func__ , "Hubo un problema al Actualizar los Datos del Nodo %s Ip %s Puerto %s" , nodoNombre, nodoIP, nodoPuerto );
			Sockets_cerrar_desconectar(socketConectadoAlNodo);
			return NULL;
		}

	}else{
		//El nodo es nuevo, entonces hay que agregarlo como vacio a la Base de Datos con sus datos
		iNumeroError = NODO_Nuevo(nodoNombre, nodoIP, nodoPuerto, nodoNumeroBloques );
		//Chequeo Errores
		if (iNumeroError < 0) {
			Servidor_loguear(LOG_LEVEL_ERROR, "Servidor_realizarOrden_avisarNodoConectado" , "(Linea %d) Hubo un problema al Intentar agregar a la Base de Datos al Nodo %s Ip %s Puerto %s" , __LINE__, nodoNombre, nodoIP, nodoPuerto );
			Sockets_cerrar_desconectar(socketConectadoAlNodo);
			return NULL;
		}
	}

	Servidor_loguear(LOG_LEVEL_TRACE, "Servidor_realizarOrden_avisarNodoConectado", "Se Agrego/Reconecto el Nodo en la Base de Datos. Procedo a Enviarle Confirmacion al Nodo");

	//Ahora le aviso al NODO un "OK, ya estas conectado" que ya esta conectado al File System (sea Nuevo o Reconectado, no interesa)
	char* ordenSerializadaConHeader = package_create("OK, ya estas conectado",strlen("OK, ya estas conectado")+1, "nodoAceptado", FILESYSTEM);
	iNumeroError = Sockets_enviar_datos(socketConectadoAlNodo, ordenSerializadaConHeader);
	if (iNumeroError <= 0) {

		//Libero la memoria de la orden
		package_destroy(ordenSerializadaConHeader);

		Servidor_loguear(LOG_LEVEL_ERROR, "Servidor_realizarOrden_avisarNodoConectado", "(Linea %d) Se cayo el Nodo: %s en IP: %s Puerto: %s Apenas me aviso que queria conectarse , se lo considero como Caido . Hay mas informacion en Consola (por funciones de Sockets)", __LINE__, nodoNombre, nodoIP, nodoPuerto );

		///El NODO esta caido, hay que marcarlo como No Disponible en la Base de Datos
		iNumeroError = FS_DesactivarNodo(nodoNombre);
		if (iNumeroError < 0) {
			Servidor_loguear(LOG_LEVEL_ERROR, "Servidor_realizarOrden_pedirSubirArchivoFinal" , "(Linea %d) No se Desactivar al Nodo que se Cayo" , __LINE__ );
		}

		return NULL;
	}

	//Libero la memoria de la orden
	package_destroy(ordenSerializadaConHeader);

	//Fin Orden
	Servidor_loguear(LOG_LEVEL_INFO, "Servidor_realizarOrden_avisarNodoConectado", "Se Finalizo Correctamente la orden. Nodo Conectado:'%s' \n", nodoNombre );

	//Como no uso mas el socket, lo cierro
	Sockets_cerrar_desconectar(socketConectadoAlNodo);

	//Por Ultimo, retorno el Nombre del Nodo, asi lo puede usar en el Main_File_System
	return string_duplicate(nodoNombre);
}
