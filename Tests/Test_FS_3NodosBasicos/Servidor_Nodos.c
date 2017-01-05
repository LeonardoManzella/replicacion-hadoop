#include "Servidor_Nodos.h"

#include "../../Serializador/Protocolo_Nodos_FS.h"

#include <commons/string.h>

//Inicializo un Mutex Global para los Logs, asi se puede loguear desde donde sea sin problemas.
pthread_mutex_t GLOBAL_mutex_interno_paraLogs = PTHREAD_MUTEX_INITIALIZER;

void threadNodo(char* sPuertoEscucha) {
	//Variable para Manejo de Errores
	int iNumeroError = 0;


	//Indico que Cada vez que me llegue la señal SIGCHLD (se termino un proceso hijo) revise si esta en estado Zombie y lo mata correctamente
	//Voy a definir la accion para la señal SIGCHLD
	/*	Creo un struct necesario para la funcion "sigaction"
	 NOTA: Es necesario poner "struct" porque hay una funcion que se llama igual al tipo de dato (sino, entiende cualquier cosa el compilador)
	 */
	struct sigaction accionSenial_SIGCHLD;

	//Le especifico que la accion sera llamar a la funcion "Handler_Eliminar_Zombies"
	accionSenial_SIGCHLD.sa_handler = &Handler_Eliminar_Zombies;

	//Ahora si establesco la Accion para la Señal, a partir de ahora es automatico, no hace falta tocar nada mas
	sigaction( SIGCHLD , &accionSenial_SIGCHLD , NULL);


	//Ahora me Conecto al FILE SYSTEM
	tipo_socket* socketConectadoFS = Sockets_conectar_servidor(IP_FILE_SYSTEM , PUERTO_FILE_SYSTEM);
	if (socketConectadoFS == NULL) {
		Servidor_loguear("threadNodo" , "NO me pude Conectar al FS" , LOG_LEVEL_ERROR);
		return;
	}

	//Variables HardCodeadas para Armar la Orden “avisarNodoConectado”
	char* nodoNombre = sPuertoEscucha;
	char* nodoIP = "127.0.0.1";
	int nodoNumeroBloquesArchivoBIN = 100;
	t_datos_nodo datosEsteNodo;
	strcpy(datosEsteNodo.nombre_nodo, nodoNombre);
	strcpy(datosEsteNodo.ip, nodoIP);
	datosEsteNodo.puerto = atol(sPuertoEscucha);
	datosEsteNodo.numero_bloques = nodoNumeroBloquesArchivoBIN;


	//Serializo el Payload de la orden
	char* payload = serializar_datos_nodo(datosEsteNodo);

	//Armo la Orden
	char* ordenSerializada = package_create(payload , sizeof(t_datos_nodo) , "avisarNodoConectado" , NODO);

	//Se la envio al FS
	iNumeroError = Sockets_enviar_datos(socketConectadoFS , ordenSerializada);
	if (iNumeroError <= 0) {
		Servidor_loguear("threadNodo" , "NO pude enviar orden 'avisarNodoConectado' al FS" , LOG_LEVEL_ERROR);
		return;
	}

	//Espero contestacion
	t_header headerRecibido;
	iNumeroError = Sockets_recibir_Header(socketConectadoFS , &headerRecibido);
	if (iNumeroError <= 0) {
		Servidor_loguear("threadNodo" , "NO pude recibir header del FS" , LOG_LEVEL_ERROR);
		return;
	}

	char* confirmacion;
	iNumeroError = Sockets_recibir_Datos(socketConectadoFS , &confirmacion , headerRecibido);
	if (iNumeroError <= 0) {
		Servidor_loguear("threadNodo" , "NO pude recibir payload del FS" , LOG_LEVEL_ERROR);
		return;
	}

	//Verifico la Contestacion Recibida o sino imprimo que me llego
	if (!string_equals_ignore_case(confirmacion , "Ok, ya estas conectado")) {
		char* stringParaLoguear = string_from_format("No llego una Confirmacion Correcta del FS. Llego %s" , confirmacion);
		Servidor_loguear("threadNodo" , stringParaLoguear , LOG_LEVEL_ERROR);
		return;
	}

	//Si llegue hasta aca es que El FS me acepto correctamente, ahora me pongo a esperar cosas

	Sockets_cerrar_desconectar(socketConectadoFS);


	Servidor_loguear("threadNodo" , "Pude Conectarme al FS" , LOG_LEVEL_TRACE);


	//Creo el Socket de Escucha del Servidor
	tipo_socket* socketEnEscucha = Sockets_ponerme_escuchar(sPuertoEscucha);
	//Chequeo errores
	if (socketEnEscucha == NULL) {
		char* stringParaLoguear = string_from_format("Hubo un problema con la conexion apenas nos pusimos en escucha, se imprimio por pantalla que paso (la funcion de Socket lo hizo)");
		Servidor_loguear("threadNodo" , stringParaLoguear , LOG_LEVEL_ERROR);
		return;
	}

	//Acepto nuevos clientes infinitamente, bloqueandose hasta que se conecte un nuevo cliente
	while ( INFINITO) {

		//Para DEBUG
		printf("NODO %s Iniciado, Esperando Nuevo Cliente...\n" , sPuertoEscucha);

		//Acepto un nuevo cliente
		tipo_socket* socketConectadoAlCliente = Sockets_aceptar_cliente(socketEnEscucha);
		//Chequeo errores
		if (socketConectadoAlCliente == NULL) {
			char* stringParaLoguear = string_from_format("Nodo %s . Hubo un problema con la conexion cuando intentamos aceptar un cliente, se imprimio por pantalla que paso (la funcion de Socket lo hizo)", sPuertoEscucha);
			Servidor_loguear("threadCreadoPorCliente" , stringParaLoguear , LOG_LEVEL_ERROR);
			return;
		}

		//Armo el string para loguear que se conecto un cliente
		char* stringParaLoguear = string_from_format("Se conecto alguien al Nodo %s, su IP es: %s y su Puerto es: %d" , sPuertoEscucha, Sockets_obtener_ip_cliente(socketConectadoAlCliente) , Sockets_obtener_puerto_cliente(socketConectadoAlCliente));
		Servidor_loguear("Servidor de Escucha" , stringParaLoguear , LOG_LEVEL_TRACE);

		//Creo una variable para el ID del Thread, no nos importa realmente que se pierda el ID de los Threads Hijos
		pthread_t idThread;

		//Creo un nuevo Thread para atender al cliente y le paso el socketConectadoAlCliente
		iNumeroError = pthread_create(&idThread , NULL , (void*) &threadCreadoPorCliente , (void*) socketConectadoAlCliente);

		//Chequeo Errores al crear el thread
		if (iNumeroError != 0) {
			perror("Ocurrio un Error al crear el Thread. El Error es");
			return;
		}

		//Le digo al Thread que cuando se termine libere los recursos que uso, lo hace el solo y nos olvidamos nosotros
		iNumeroError = pthread_detach(idThread);

		//Chequeo Errores en el detach
		if (iNumeroError != 0) {
			perror("Ocurrio un Error al hacer Detach. El Error es");
			return;
		}

	}

	//En teoria nunca ejecutaria aca abajo ni en necesario

	return;
}

//Rutina para Eliminar Procesos Zombies (se encarga automaticamente)
void Handler_Eliminar_Zombies(int valorQueNoUso) {
	//Para Debug
	//printf("Llego al Handler de Eliminar Zombie\n");

	//Elimino todos los Zombies Procesos Hijos (creados con fork) que encuentra
	while (waitpid(-1 , 0 , WNOHANG))
		;

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
	int iNumeroError = 0;
	//Variable para el header
	t_header headerRecibido;

	//Aca vamos a esperar a que me llegue una peticion
	//Recibimos el header
	iNumeroError = Sockets_recibir_Header(socketConectadoAlCliente , &headerRecibido);

	//Chequeo Errores
	if (iNumeroError == 0) {

		char* stringParaLoguear = string_from_format("Se corto la conexion apenas recibimos al cliente, No tenemos manera de saber si era un Nodo o el Marta, asi que no podemos hacer nada");
		Servidor_loguear("threadCreadoPorCliente" , stringParaLoguear , LOG_LEVEL_WARNING);
		return;

	} else if (iNumeroError < 0) {
		char* stringParaLoguear = string_from_format("Hubo un problema con la conexion apenas recibimos al cliente, se imprimio por pantalla que paso (la funcion de Socket lo hizo)");
		Servidor_loguear("threadCreadoPorCliente" , stringParaLoguear , LOG_LEVEL_ERROR);
		return;
	}

	//Veo quien se me conecto y en base a eso voy a ver que orden/peticion me mandaron para tenderla
	switch (headerRecibido.sender_id) {
		//Veo si se me conecto el Marta (por ejemplo)
		case FILESYSTEM:
			if (header_esOrden(headerRecibido , "envioUbicacionBloqueParaGuardar")) {
				Servidor_realizarOrden_envioUbicacionBloqueParaGuardar(socketConectadoAlCliente , headerRecibido);
			} else if (header_esOrden(headerRecibido , "pedirSubirArchivoFinal")) {
				Servidor_realizarOrden_pedirSubirArchivoFinal(socketConectadoAlCliente , headerRecibido);
			}else if (header_esOrden(headerRecibido , "pedirBloque")) {
				Servidor_realizarOrden_pedirBloque(socketConectadoAlCliente , headerRecibido);
			}else if (header_esOrden(headerRecibido , "copiarBloqueANodo")) {
				Servidor_realizarOrden_copiarBloqueANodo(socketConectadoAlCliente , headerRecibido);
			} else {
				char* stringParaLoguear = string_from_format(" Llego una orden del FILE SYSTEM que no se la reconoce, la orden decia: %s" , headerRecibido.order);
				Servidor_loguear("threadCreadoPorCliente" , stringParaLoguear , LOG_LEVEL_WARNING);
			}
			break;

		default:
			//Ese Punto y Coma suelto ES NECESARIO porque no se pueden hacer declaraciones de variables apenas empieza un "case o default" (una restriccion que viene desde ANSI C)
			;
			char* stringParaLoguear = string_from_format("Llego una Orden de alguien que no reconosco, quien mando: %d" , headerRecibido.sender_id);
			Servidor_loguear("threadCreadoPorCliente" , stringParaLoguear , LOG_LEVEL_WARNING);
			break;
	}

	//Como no uso mas el socket, lo cierro
	Sockets_cerrar_desconectar(socketConectadoAlCliente);

	//Salgo de la Funcion
	pthread_exit( EXIT_SUCCESS);
}

void Servidor_loguear(const char* nombreTareaFuncion , const char* textoPorLoguear , t_log_level nivelDeLogueo) {
	//Inicio Mutex
	pthread_mutex_lock(&GLOBAL_mutex_interno_paraLogs);

	//Creo mi Log, defino que NO se muestre por pantalla y defino el Nivel de Log.
	//NOTA: Si no existe el Log, lo Crea. Si ya existia agrega cosas al final
	t_log *miLog = log_create( ARCHIVO_DE_LOG , (char*) nombreTareaFuncion , false , LOG_LEVEL_TRACE);

	//Veo con que nivel debo Imprimir
	switch (nivelDeLogueo) {
		case LOG_LEVEL_TRACE:
			log_trace(miLog , textoPorLoguear);
			break;
		case LOG_LEVEL_DEBUG:
			log_debug(miLog , textoPorLoguear);
			break;
		case LOG_LEVEL_INFO:
			log_info(miLog , textoPorLoguear);
			break;
		case LOG_LEVEL_WARNING:
			log_warning(miLog , textoPorLoguear);
			break;
		case LOG_LEVEL_ERROR:
			log_error(miLog , textoPorLoguear);
			break;
		default:
			printf("Me mandaste a imprimir un nivel de Logueo que no existe, revisa los Argumentos");
			break;
	}

	//Libero la Memoria y dejo de escribir en el Log
	log_destroy(miLog);

	//Fin Mutex
	pthread_mutex_unlock(&GLOBAL_mutex_interno_paraLogs);
}

void Servidor_realizarOrden_envioUbicacionBloqueParaGuardar(tipo_socket* socketConectadoAlCliente , t_header headerRecibido) {
	//Variable para Manejo de Errores
	int iNumeroError = 0;

	//Recibo Numero Bloque para Archivo BIN y lo imprimo
	char* numeroBloque;
	iNumeroError = Sockets_recibir_Datos(socketConectadoAlCliente , &numeroBloque , headerRecibido);
	if (iNumeroError <= 0) {
		Servidor_loguear("Servidor_realizarOrden_envioUbicacionBloqueParaGuardar" , "NO pude recibir el Numero de Bloque a Guardar en archivo BIN" , LOG_LEVEL_ERROR);
		return;
	}

	char* stringParaLoguear = string_from_format("Me llego que tengo que guardar un Bloque en la Posicion %d de mi Archivo BIN" , atol(numeroBloque) );
	Servidor_loguear("Servidor_realizarOrden_envioUbicacionBloqueParaGuardar" , stringParaLoguear , LOG_LEVEL_TRACE);

	//Ahora me enviaria el Bloque el FS
	t_header headerRecibidoFS;
	iNumeroError = Sockets_recibir_Header(socketConectadoAlCliente , &headerRecibidoFS);
	if (iNumeroError <= 0) {
		Servidor_loguear("threadNodo" , "NO pude recibir header del FS" , LOG_LEVEL_ERROR);
		return;
	}

	char* bloqueSerializadoPayload;
	iNumeroError = Sockets_recibir_Datos(socketConectadoAlCliente , &bloqueSerializadoPayload , headerRecibidoFS);
	if (iNumeroError <= 0) {
		Servidor_loguear("threadNodo" , "NO pude recibir payload del FS" , LOG_LEVEL_ERROR);
		return;
	}

	tipo_bloque* punteroBloque = Bloques_des_serializar(bloqueSerializadoPayload);

	//Verifico que el bloque tenga un tamaño mayor a 0, imprimo su tamaño
	if (punteroBloque->tamanioBloque <= 0) {
		stringParaLoguear = string_from_format("Hay un Problema, el Bloque tiene un tamaño <=0 y no deberia. el tamaño detectado es %d" , (int) punteroBloque->tamanioBloque);
		Servidor_loguear("Servidor_realizarOrden_envioUbicacionBloqueParaGuardar" , stringParaLoguear , LOG_LEVEL_ERROR);
		return;
	}

	Comun_LiberarMemoria((void**) &bloqueSerializadoPayload);
	Bloques_destruir(punteroBloque);

	//Le envio al FS una Confirmacion de que guarde el Bloque
	char* confirmacion = package_create("Bloque Guardado Correctamente" , string_length("Bloque Guardado Correctamente") + 1 , "respuestaBloqueGuardado" , NODO);

	iNumeroError = Sockets_enviar_datos(socketConectadoAlCliente , confirmacion);
	if (iNumeroError <= 0) {
		Servidor_loguear("threadNodo" , "NO pude enviar Confirmacion del Bloque al FS" , LOG_LEVEL_ERROR);
		return;
	}


	//Si llego aca es que se realizo correctamente el Comando
	stringParaLoguear = string_from_format("Guarde un Bloque Dentro de mi Archivo BIN \n");
	Servidor_loguear("Servidor_realizarOrden_envioUbicacionBloqueParaGuardar" , stringParaLoguear , LOG_LEVEL_TRACE);

	return;
}

void Servidor_realizarOrden_pedirSubirArchivoFinal(tipo_socket* socketConectadoAlCliente , t_header headerRecibido) {
	//Variable para Manejo de Errores
	int iNumeroError = 0;
	char* stringParaLoguear;

	//Recibo el Nombre del Archivo que me pidieron
	char* nombreArchivo;
	iNumeroError = Sockets_recibir_Datos(socketConectadoAlCliente , &nombreArchivo , headerRecibido);
	if (iNumeroError <= 0) {
		Servidor_loguear("Servidor_realizarOrden_pedirSubirArchivoFinal" , "No pude recibir el nombre de que archivo quiere el FS" , LOG_LEVEL_ERROR);
		return;
	}

	stringParaLoguear = string_from_format("Inicio de Orden.  Me Pidio el FS el archivo Final %s", nombreArchivo );
	Servidor_loguear("Servidor_realizarOrden_pedirSubirArchivoFinal" , stringParaLoguear , LOG_LEVEL_TRACE);

	//Preparo y serializo los datos a Enviar
	t_datos_archivo_final datosArchivoEnviar;
	datosArchivoEnviar.tamanio = ARCHIVOFINAL_TAMANIO;
	datosArchivoEnviar.cantidad_bloques = ARCHIVOFINAL_CANTIDAD_BLOQUES;
	strcpy(datosArchivoEnviar.md5, ARCHIVOFINAL_MD5);

	uint32_t tamanioSerializacion = -1;
	char* archivoFinalDatosSerializados = serializar_datos_archivo_final(datosArchivoEnviar, &tamanioSerializacion);

	//Armo la Orden
	char* package_archivoFinalDatos = package_create(archivoFinalDatosSerializados, tamanioSerializacion,"enviarDatosArchivoFinal",NODO);


	//Envio los Datos del Archivo Final
	iNumeroError = Sockets_enviar_datos(socketConectadoAlCliente,package_archivoFinalDatos);
	if (iNumeroError <= 0) {
		Servidor_loguear("Servidor_realizarOrden_pedirSubirArchivoFinal" , "No pude enviar los Datos del Archivo Final" , LOG_LEVEL_ERROR);
		return;
	}

	//Preparo el Bloque
	tipo_bloque* bloquePorEnviar = Bloques_crear_nuevo();
	strcpy(bloquePorEnviar->contenidoBloque, ARCHIVOFINAL_CONTENIDO);
	bloquePorEnviar->tamanioBloque = string_length(ARCHIVOFINAL_CONTENIDO)+1;

	//Serializo el Bloque
	char* bloqueSerializado = Bloques_serializar(bloquePorEnviar,&tamanioSerializacion);

	//Armo la Orden
	char* package_archivoFinalBloque = package_create(bloqueSerializado,tamanioSerializacion,"enviarBloqueArchivoFinal",NODO);


	//Envio el Bloque del Archivo Final
	iNumeroError = Sockets_enviar_datos(socketConectadoAlCliente , package_archivoFinalBloque);
	if (iNumeroError <= 0) {
		Servidor_loguear("Servidor_realizarOrden_pedirSubirArchivoFinal" , "No pude enviar el Bloque del Archivo Final" , LOG_LEVEL_ERROR);
		return;
	}


	//Si llego aca es que se realizo correctamente el Comando
	stringParaLoguear = string_from_format("Pude Enviar el Archivo Final al FILE System. Aconsejo usar el Comando:  MostrarBloquesDeArchivo \"ArchivoFinal.txt\" \"/\"  para Ver si llego todo bien \n");
	Servidor_loguear("Servidor_realizarOrden_pedirSubirArchivoFinal" , stringParaLoguear , LOG_LEVEL_TRACE);

	return;
}

void Servidor_realizarOrden_pedirBloque(tipo_socket* socketConectadoAlCliente, t_header headerRecibido){
	//Variable para Manejo de Errores
	int iNumeroError = 0;
	char* stringParaLoguear;

	//Recibir el Payload de la Orden, aunque no se usa en el Dummie
	char* numeroBloqueArchivoBIN;
	iNumeroError = Sockets_recibir_Datos(socketConectadoAlCliente , &numeroBloqueArchivoBIN , headerRecibido);
	if (iNumeroError <= 0) {
		Servidor_loguear("Servidor_realizarOrden_pedirBloque" , "No pude recibir el numeroBloqueArchivoBIN quiere el FS" , LOG_LEVEL_ERROR);
		return;
	}

	stringParaLoguear = string_from_format("Inicio de Orden.  Me Pidio el FS el Bloque %d", atol(numeroBloqueArchivoBIN) );
	Servidor_loguear("Servidor_realizarOrden_pedirBloque" , stringParaLoguear , LOG_LEVEL_TRACE);

	//Aca se supone que buscaria el Bloque, nosotros usamos valores hardcodeados pra el Dummie

	//Preparo el Bloque para Enviar
	tipo_bloque* bloquePorEnviar = Bloques_crear_nuevo();
	strcpy(bloquePorEnviar->contenidoBloque, BLOQUE_PEDIDO_CONTENIDO);
	bloquePorEnviar->tamanioBloque = string_length(BLOQUE_PEDIDO_CONTENIDO)+1;

	//Serializo el Bloque
	uint32_t tamanioSerializacion = -1;
	char* bloqueSerializado = Bloques_serializar(bloquePorEnviar,&tamanioSerializacion);

	//Preparo la Orden
	char* package_archivoFinalBloque = package_create(bloqueSerializado,tamanioSerializacion,"enviarBloquePedido",NODO);

	//Envio el Bloque
	iNumeroError = Sockets_enviar_datos(socketConectadoAlCliente , package_archivoFinalBloque);
	if (iNumeroError <= 0) {
		Servidor_loguear("Servidor_realizarOrden_pedirBloque" , "No pude enviar el Bloque Pedido" , LOG_LEVEL_ERROR);
		return;
	}

	//Si llego aca es que se realizo correctamente el Comando
	stringParaLoguear = string_from_format("Pude Enviar el Bloque Pedido al FILE System. Aconsejo ver el Archivo Generado por el FS para ver si llego bien \n");
	Servidor_loguear("Servidor_realizarOrden_pedirBloque" , stringParaLoguear , LOG_LEVEL_TRACE);

	return;
}

void Servidor_realizarOrden_copiarBloqueANodo(tipo_socket* socketConectadoAlCliente, t_header headerRecibido){
	//Variable para Manejo de Errores
	int iNumeroError = 0;
	char* stringParaLoguear;

	//Recibir el Payload de la Orden, aunque no se usa en el Dummie
	char* payload;
	iNumeroError = Sockets_recibir_Datos(socketConectadoAlCliente , &payload , headerRecibido);
	if (iNumeroError <= 0) {
		Servidor_loguear("Servidor_realizarOrden_copiarBloqueANodo" , "No pude recibir el numeroBloqueArchivoBIN quiere el FS" , LOG_LEVEL_ERROR);
		return;
	}

	stringParaLoguear = string_from_format("Inicio de Orden.  Me Pidio el FS que copie un Bloque (Nimporta Ahora cual es)" );
	Servidor_loguear("Servidor_realizarOrden_copiarBloqueANodo" , stringParaLoguear , LOG_LEVEL_TRACE);

	//Aca yo Deberia des-Serializar la Orden para hacer cosas. BaH! No me importa y le mando un OK al FS

	//Preparo la Orden
	char* package_archivoFinalBloque = package_create("Confirmacion", strlen("Confirmacion")+1, "respuestaCopiarBloqueANodo",NODO);

	//Envio el Bloque
	iNumeroError = Sockets_enviar_datos(socketConectadoAlCliente , package_archivoFinalBloque);
	if (iNumeroError <= 0) {
		Servidor_loguear("Servidor_realizarOrden_copiarBloqueANodo" , "No pude enviar Confirmacion al FS" , LOG_LEVEL_ERROR);
		return;
	}

	//Si llego aca es que se realizo correctamente el Comando
	stringParaLoguear = string_from_format("Pude Copiar el Bloque Pedido por FILE System. \n");
	Servidor_loguear("Servidor_realizarOrden_copiarBloqueANodo" , stringParaLoguear , LOG_LEVEL_TRACE);
	return;
}
