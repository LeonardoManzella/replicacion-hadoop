/*
Contiene todas las funciones relacionadas con el uso de Sockets. Para conectar, desconectar, enviar y recibir datos. 
*/
 
#include "Biblioteca_Sockets.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>
#include <netdb.h>

#include <sys/wait.h>
#include <signal.h>
#include <assert.h>
#include <sys/time.h>

#include "../Biblioteca_Comun/Biblioteca_Comun.h" // Para Las Macros y Constantes


//Si hay problemas de "Conexiones Rechazadas" en los Clientes, hay que aumentar este Numero
//Lo subi porque No quiero que Explote con un Archivo de 101 Bloques, puede que ahora consuma Mas Memoria cada vez que se pone a Escuchar cualquier Servidor
#define CONEXIONES_MAXIMAS_DE_ESCUCHA	400

#define MAXIMOS_CARACTERES_PAYLOAD_POR_IMPRIMIR	200



static uint32_t Sockets_funcion_interna_recibir_datos(  const tipo_socket* socket,   char* buffer,   const uint32_t tamanioBuffer  );
	/*	Recibo (por el socket especificado) Datos y los guarda en "buffer". Debe especificarse que tamañio tiene "buffer".
		Me devuelve (en caso correcto) la cantidad de bytes que recibi. (lo puse asi porque puede ser util para saber si definiste un buffer de tamaño mas chico que los datos enviados)
		Ante un error, imprime por pantalla que paso y devuelve:
																			0 si se cerro/cayo la conexion desde el otro lado
																			-1 si hubo otro error
	*/
	/*	NOTA: La funcion retira del buzon/cola del socket la cantidad de bytes especificada en "tamanioBuffer".
		NOTA: Esta funcion es Bloqueante (se queda esperando hasta que recibe datos O hasta que se corto la conexion [Si, Enserio detecta que se corto la conexion]) asi que podes hacer 2 cosas:
			1-Usar SELECT con un Tiempo Maximo para esperar que se reciba algo.
			2-Usar la funcion asi como viene y tener en cuenta que puede quedarse tildado el programa (pero no por siempre, ya que detecta que se corto la conexion).
	En ambos casos vos tendrias que hacer algo (para el programa? avisar a alguien? reintentar?)  si se pasa del tiempo maximo / se corta la conexion.
	*/
static uint32_t Sockets_funcion_interna_enviar_datos(  const tipo_socket* socket,   const char* buffer,   const uint32_t bytesPorEnviar  );
	/*	Envia (por el socket especificado) los Datos guardados en "buffer". Debe especificarse que tamañio tiene "buffer", asi sabe cuantos bytes debe enviar.
		Me devuelve (en caso correcto) la cantidad de bytes que envie.
		Ante un error, imprime por pantalla que paso y devuelve:
																		0 si se enviaron menos Bytes de los que pediste (se corto el paquete al enviar)
																		-1 si hubo otro error
	*/
static void Sockets_imprimir_header(const char* datosSerializadosConHeader);
//Imprime caracter por caracter el header de los "datosSerializadosConHeader"

static void Sockets_imprimir_payload(const char* payloadSerializado, const uint32_t tamanioPayload);
//Imprime caracter por caracter el payload indicado.




const char* Sockets_obtener_ip_cliente( const tipo_socket* socketServidorConClienteAceptado ){
	//Chequeo Argumentos
	if(socketServidorConClienteAceptado==NULL){
		Macro_ImprimirParaDebug("No puedo obtener la IP de un Socket NULL, fijate que me pasaste un puntero NULL.\n");
		return "ERROR";
	}

	//String donde guardo la IP del Cliente Remoto, defino de IPv6 para que seguro alcanze el tamaño

	//Es una pelotudes pero necesito SI o SI una variable porque asi trabaja inet_ntop.
	static char ipCliente[INET6_ADDRSTRLEN];
	
	//Inicializo la variable con Ceros, para evitar problemas al ser static
	memset(ipCliente, '0', INET6_ADDRSTRLEN);

	//Obtengo la IP del cliente remoto
	inet_ntop(AF_INET, &(socketServidorConClienteAceptado->tDireccionClienteRemoto.sin_addr), ipCliente, sizeof(ipCliente));

	//NOTA Especial: Tuve que definir el Char* a devolver como static porque sino devuelve basura (tiene que ver conque si no uso static se guarda en la pila y al terminarse el programa se libera la pila, con static guarda la variable en el heap y cuando se termina el programa entonces puede devolver el valor)
	return ipCliente;
}

int Sockets_obtener_puerto_cliente( const tipo_socket* socketServidorConClienteAceptado ){
	//Chequeo Argumentos
	if(socketServidorConClienteAceptado==NULL){
		Macro_ImprimirParaDebug("No puedo obtener el Puerto de un Socket NULL, fijate que me pasaste un puntero NULL.\n");
		return -1;
	}

	return ntohs(socketServidorConClienteAceptado->tDireccionClienteRemoto.sin_port);
}

//Un Cliente (yo) me conecto a un servidor Remoto, especificando su IP y Puerto. Me devuelve (en caso correcto) un puntero a un socket que Ya Esta Conectado al servidor (ya puedo recibir y enviar datos). Ante un error, imprime por pantalla que paso y devuelve NULL.
	//Dentro la funcion crea todas las estructuras necesarias y el tipo_socket
tipo_socket* Sockets_conectar_servidor(const char* serverIP, const char* serverPort) {
	Macro_ImprimirParaDebug("Intentando Conectar al Servidor en IP: %s y Puerto: %s...\n", serverIP, serverPort );

	//Primero Hago Chequeos de IP
	if ((strlen(serverIP) < 9) || (strlen(serverIP) > 15)) {
		Macro_ImprimirParaDebug("Me mandaste cualquier cosa como IP, Fijate que me pediste la IP: %s \n", serverIP);
		return NULL;
	}
	//Chequeo que no manden cualquier cosa de Puerto
	if ((strlen(serverPort) < 4) || (atol(serverPort) > 65532)) {
		Macro_ImprimirParaDebug("Me mandaste cualquier cosa como Puerto, Fijate que me pediste el puerto: %s \n", serverPort);
		return NULL;
	}
	//Chequeo que no usen sockets Reservados
	if (atol(serverPort) < 1024) {
		Macro_ImprimirParaDebug("Estas usando un puerto Reservado (Menor a 1024), Me pediste el puerto: %s \n", serverPort);
		return NULL;
	}



	//Estructuras para la busqueda de IPs disponibles donde escuchar (Esto es porque por alguna razon pueden haber duplicados[Ej: duplicadas las direcciones por IPv4 e IPv6] y algunos de esos duplicados pueden no funcionar).
	struct addrinfo busqueda;
	struct addrinfo *resultadoBusqueda = NULL;
	
	//Me aseguro que la structura "busqueda" este vacio, asi que la lleno con Ceros.
	memset(&busqueda, 0, sizeof(busqueda));
	//Establesco que el protocolo que quiero es IPv4
	busqueda.ai_family = AF_INET;
	//Establesco que el protocolo que quiero es TCP
	busqueda.ai_socktype = SOCK_STREAM;
	
	//Establesco mi IP, me llena mi IP por mi asi no hay que usar un parametro ni hardcodear..
	busqueda.ai_flags = AI_PASSIVE;
	
	//Creo un tipo_socket de forma dinamica (si fuera estatico no lo puedo devolver por ser variable local)
	tipo_socket* socketConectadoAlServidor = (tipo_socket*)malloc(sizeof(tipo_socket));
	
	//Variable para Manejar Errores
	int numeroError;
	
	//Funcion que sirve para cargarme todas los "struct de struct de struct" necesarios, asi no debo hacerlo a mano.
	//Supuestamente tambien hace un chequeo por DNS y revisa que exista y pueda conectarse al servidor en su IP. 
	numeroError = getaddrinfo(serverIP, serverPort, &busqueda, &resultadoBusqueda);
	if (numeroError != 0) {
		Macro_ImprimirParaDebug("Hubo un error al usar Getaddrinfo para cargar las estructuras de datos del Nuevo Socket, el Error Detectado es: %s\n", gai_strerror(numeroError));
		free(socketConectadoAlServidor);
		return NULL;
	}


	//Ciclamos entre todos los Resultados obtenidos y me Enlazo (bind) al primero que puedo
		//Puntero para ciclar por los Resultados
		struct addrinfo *punteroResultado;
	for(punteroResultado = resultadoBusqueda; punteroResultado != NULL; punteroResultado = punteroResultado->ai_next) {
		
		
		//Creo un nuevo socket con los valores obtenidos por "getaddrinfo".
		socketConectadoAlServidor->descriptorSocket = socket(resultadoBusqueda->ai_family, resultadoBusqueda->ai_socktype, resultadoBusqueda->ai_protocol);
		if( socketConectadoAlServidor->descriptorSocket==-1 ){
			Macro_ImprimirParaDebug("Error al crear el Socket.\n");
			Macro_ImprimirParaDebug("Error Detectado:" ANSI_COLOR_RED "%s" ANSI_COLOR_RESET "\n", Macro_Obtener_Errno());
			Macro_ImprimirParaDebug("Reintentando...\n");
			//Como hubo un error con este resultado, salto al proximo ciclo con el proximo resultado.
			continue;
		}
			
			
			
			
		//Establesco los Flags Necesarios en el socket recien creado.
			//SO_REUSEADDR hace que se pueda reutilizar inmediatamente el socket despues de hacerle un Close (asi no da error de "socket ocupado")
			
			//Variable para Establecer los Flag en True.
			int verdadero=1;
		if (setsockopt(socketConectadoAlServidor->descriptorSocket, SOL_SOCKET, SO_REUSEADDR, &verdadero,  sizeof(int)) == -1) {
			Macro_ImprimirParaDebug("No se pudieron establecer las Opciones/Flags Necesarias en el socket\n");
			Macro_ImprimirParaDebug("Error Detectado:" ANSI_COLOR_RED "%s" ANSI_COLOR_RESET "\n", Macro_Obtener_Errno());
			Macro_ImprimirParaDebug("Reintentando...\n");
			//Como hubo un error con este resultado, salto al proximo ciclo con el proximo resultado.
			continue;
		}

		
		/*Comentario Viejo, pero puede llegar a ser util:
		No se si aca hace falta un BIND de mi socket "socketConectadoAlServidor" con una IP y Puerto para el Cliente. Y si es por esto que me da que el server esta ocupado
		NO, era que el server no esta haciendo ningun accept. Hay que usar justamente el SELECT, como No Bloqueante o sino tener un Hilo que Solo Acepte Peticiones y cree otros hilos para ejecutar estas peticiones (es la idea de cliente servidor).
		*/
		
		//No hace falta realizar ningun Bind porque nosotros seriamos el Cliente que se quiere conectar al servidor. El Bind es obligatorio para hacer Listen
		//Al no definir un Puerto con el bind, al hacer connect va a buscar un Puerto que No este siendo Usado y lo va a utilizar.
		
		
		
		
		//Me conecto al Servidor remoto con los valores obtenidos por "getaddrinfo".
		numeroError = connect(socketConectadoAlServidor->descriptorSocket, resultadoBusqueda->ai_addr, resultadoBusqueda->ai_addrlen);
		if( numeroError!=0 ){
			Macro_ImprimirParaDebug("Error al conectarse Al Servidor. \n");
			Macro_ImprimirParaDebug("Error Detectado:" ANSI_COLOR_RED "%s" ANSI_COLOR_RESET "\n", Macro_Obtener_Errno());
			Macro_ImprimirParaDebug("Reintentando...\n");
			continue;
		}
		
		
		
		//Si llego hasta aca es que no hubo errores y se puedo conectar, entonces salgo del Ciclo.
		break;
	}
	
	
	//Chequeo que no se hallan recorridos todos los resultados, porque entonces significa que no nos pudimos conectar de ninguna manera al servidor.
	if (punteroResultado == NULL) {
		Macro_ImprimirParaDebug( "No hubo ninguna manera de Conectarse al Servidor.\n");
		free(socketConectadoAlServidor);
		return NULL;
	}
	
	

	//Por los memory Leaks libero aca al struct "resultadoBusqueda" que use con "getaddrinfo".
	freeaddrinfo(resultadoBusqueda);
	
	//Como llegados aca me pude Conectar al Servidor, Imprimo y Devuelvo el Socket al servidor Ya conectado
	Macro_ImprimirParaDebug("Conectado al Servidor en IP: %s y Puerto: %s \n", serverIP, serverPort );

	return socketConectadoAlServidor;
}


bool Sockets_estaDisponibleServidor(  const char* serverIP,   const char* serverPort  ){
	//El truco es ver si me puedo conectar. Si me Puedo conectar es que esta Disponible y Activo el Servidor, sino no lo esta.
	tipo_socket* socketPrueba = Sockets_conectar_servidor(serverIP, serverPort);
	if (socketPrueba == NULL) {
		Macro_ImprimirParaDebug("No esta Disponible el servidor de IP: %s y Puerto: %s\n", serverIP, serverPort);

		return false;
	}
	//Como solo era para ver si estaba Vivo, ahora lo cerramos y evitamos problemas
	Sockets_cerrar_desconectar(socketPrueba);

	//Si llegamos hasta aca es que Se pudo conectar OK, entonces esta Disponible.
	return true;
}


void Sockets_cerrar_desconectar(tipo_socket* socket) {
	//Primero Reviso que no sea un puntero NULL
	if( socket==NULL ){
		Macro_ImprimirParaDebug("Error, No se puede Cerrar un Socket que es NULL (fijate que el puntero que pasaste para cerrar apunta a NULL)\n");
		//Corto la ejecucion, sino da error en el "close" o en el "free" de mas abajo.
		return;
	}
	
	//Si no es NULL, entonces yo lo libero
	int numeroError;
	numeroError = close(socket->descriptorSocket);
	if( numeroError==-1 ){
		Macro_ImprimirParaDebug("Hubo un problema y no se pudo Cerrar el Socket. \n Esto es muy raro, no deberia pasar nunca. \n");
		Macro_ImprimirParaDebug("Error Detectado:" ANSI_COLOR_RED "%s" ANSI_COLOR_RESET "\n", Macro_Obtener_Errno());
	}
	free(socket);
	
	return;
}



tipo_socket* Sockets_ponerme_escuchar(const char* puertoDeEscucha) {
	//Chequeo que no manden cualquier cosa de Puerto
	if ((strlen(puertoDeEscucha) < 4) || (atol(puertoDeEscucha) > 65532)) {
		Macro_ImprimirParaDebug("Me mandaste cualquier cosa como Puerto, Fijate que me pediste el puerto: %s \n", puertoDeEscucha);
		return NULL;
	}
	//Chequeo que no usen sockets Reservados
	if (atol(puertoDeEscucha) < 1024) {
		Macro_ImprimirParaDebug("Estas usando un puerto Reservado (Menor a 1024), Me pediste el puerto: %s \n", puertoDeEscucha);
		return NULL;
	}
	
	
	//Estructuras para la busqueda de IPs disponibles donde escuchar (Esto es porque por alguna razon pueden haber duplicados[Ej: duplicadas las direcciones por IPv4 e IPv6] y algunos de esos duplicados pueden no funcionar).
	struct addrinfo busqueda;
	struct addrinfo *resultadoBusqueda = NULL;
	//Creo un nuevo Socket para Escuchar en el "puertoDeEscucha"
	tipo_socket* socketParaEscuchar = (tipo_socket*)malloc(sizeof(tipo_socket));  // listen on sock_fd, new connection on new_fd

	//Me aseguro que la structura "busqueda" este vacio, asi que la lleno con Ceros.
	memset(&busqueda, 0, sizeof(busqueda));
	
	//Establesco que el protocolo que quiero es IPv4
	busqueda.ai_family = AF_INET;
	//Establesco que el protocolo que quiero es TCP
	busqueda.ai_socktype = SOCK_STREAM;
	//Establesco mi IP, me llena mi IP por mi asi no hay que usar un parametro ni hardcodear..
	busqueda.ai_flags = AI_PASSIVE;
	
	//Variable para Manejar Errores
	int numeroError;
	
	//Funcion que sirve para cargarme todas los "struct de struct de struct" necesarios, asi no debo hacerlo a mano.
	//Supuestamente tambien hace un chequeo para ver si puede escuchar el servidor en el Puerto que le indico. 
	numeroError = getaddrinfo(NULL, puertoDeEscucha, &busqueda, &resultadoBusqueda);
	if (numeroError != 0) {
		Macro_ImprimirParaDebug("getaddrinfo error: %s\n", gai_strerror(numeroError));
		return NULL;
	}
	
	
	//Ciclamos entre todos los Resultados obtenidos y me Enlazo (bind) al primero que puedo
		//Puntero para ciclar por los Resultados
		struct addrinfo *punteroResultado;
	for(punteroResultado = resultadoBusqueda; punteroResultado != NULL; punteroResultado = punteroResultado->ai_next) {
		
		//Intento crear un socket con los resultados obtenidos (Familia IPv4, Socket Tipo TCP, Solo para el Protocolo TCP IP)
		if ((socketParaEscuchar->descriptorSocket = socket(punteroResultado->ai_family, punteroResultado->ai_socktype,
							 punteroResultado->ai_protocol)) == -1) {
			Macro_ImprimirParaDebug("No se pudo crear un Socket con esta Direccion, Reintentando... \n");
			Macro_ImprimirParaDebug("Error Detectado:" ANSI_COLOR_RED "%s" ANSI_COLOR_RESET "\n", Macro_Obtener_Errno());
			//Como hubo un error con este resultado, salto al proximo ciclo con el proximo resultado.
			continue;
		}
		
		//Establesco los Flags Necesarios en el socket recien creado.
			//SO_REUSEADDR hace que se pueda reutilizar inmediatamente el socket despues de hacerle un Close (asi no da error de "socket ocupado")
			
			//Variable para Establecer los Flag en True.
			int verdadero=1;
		if (setsockopt(socketParaEscuchar->descriptorSocket, SOL_SOCKET, SO_REUSEADDR, &verdadero,
						sizeof(int)) == -1) {
			Macro_ImprimirParaDebug("No se pudieron establecer las Opciones/Flags Necesarias en el socket, Reintentando...\n");
			Macro_ImprimirParaDebug("Error Detectado:" ANSI_COLOR_RED "%s" ANSI_COLOR_RESET "\n", Macro_Obtener_Errno());
			//Como hubo un error con este resultado, salto al proximo ciclo con el proximo resultado.
			continue;
		}

		//Debemos hacer Bind porque luego vamos a hacer Listen, asi sabe que puerto va a escuchar.
		if (bind(socketParaEscuchar->descriptorSocket, punteroResultado->ai_addr, punteroResultado->ai_addrlen) == -1) {
			Macro_ImprimirParaDebug("No se pudo enlazar en esta Direccion, Reintentando...\n");
			Macro_ImprimirParaDebug("Error Detectado:" ANSI_COLOR_RED "%s" ANSI_COLOR_RESET "\n", Macro_Obtener_Errno());
			close(socketParaEscuchar->descriptorSocket);
			//Como hubo un error con este resultado, salto al proximo ciclo con el proximo resultado.
			continue;
		}
		
		//Si llego hasta aca es que no hubo errores y se puedo enlazar, entonces salgo del Ciclo.
		break;
	}
	
	//Chequeo que no se hallan recorridos todos los resultados, porque entonces significa que no nos pudimos enlazar a ninguno.
	if (punteroResultado == NULL) {
		Macro_ImprimirParaDebug("No se pudo Enlazar el Socket.\n");
		return NULL;
	}
	
	//Libero de la memoria la estructura usada
	freeaddrinfo(resultadoBusqueda);
	
	//Hacemos el Listen
	if (listen(socketParaEscuchar->descriptorSocket, CONEXIONES_MAXIMAS_DE_ESCUCHA) == -1) {
		Macro_ImprimirParaDebug("No se pudo poner en Escucha al Socket.\n");
		Macro_ImprimirParaDebug("Error Detectado:" ANSI_COLOR_RED "%s" ANSI_COLOR_RESET "\n", Macro_Obtener_Errno());
		return NULL;
	}
	
	//Como ya se pudo enlazar y poner en escucha el socket, imprimo y lo devuelvo.
	Macro_ImprimirParaDebug("Estoy escuchando en Puerto: %s\n", puertoDeEscucha);
	return socketParaEscuchar;
}



tipo_socket* Sockets_aceptar_cliente(const tipo_socket* socketEnEscucha) {
	//Primero chequeo que no sea NULL el puntero al socketEnEscucha.
	if( socketEnEscucha==NULL ){
		Macro_ImprimirParaDebug("Error, No puedo Aceptar Clientes por un Socket que es NULL (fijate que el puntero que pasaste apunta a NULL)\n");
		//Corto la ejecucion porque daria error al recibir datos sino (mas abajo).
		return NULL;
	}
	
	//Estructuras necesarias para pasarle al Accept
	struct sockaddr_in tDireccionClienteRemoto;
	socklen_t tamanioDireccionCliente = sizeof(tDireccionClienteRemoto);
	
	//Creo un socket para guardar al cliente que reciba
	tipo_socket* socketCliente = (tipo_socket*)malloc(sizeof(tipo_socket));
	if (socketCliente == NULL) {
		Macro_ImprimirParaDebug("Problema Grave: No se pudo hacer Malloc para Crear el Tipo Socket para Aceptar al Cliente\n");
	}

	//Acepto al Cliente
	socketCliente->descriptorSocket = accept(socketEnEscucha->descriptorSocket, (struct sockaddr*)&tDireccionClienteRemoto, &tamanioDireccionCliente);
	//Chequeo Errores
	if (socketCliente->descriptorSocket == -1) {
		Macro_ImprimirParaDebug("No se pudo aceptar al Cliente\n");
		Macro_ImprimirParaDebug("Error Detectado:" ANSI_COLOR_RED "%s" ANSI_COLOR_RESET "\n", Macro_Obtener_Errno());
		free(socketCliente);
		return NULL;
	}
	//Guardo dentro del tipo_socket la direccion (IP y Puerto) del cliente remoto
	socketCliente->tDireccionClienteRemoto = tDireccionClienteRemoto;

	Macro_ImprimirParaDebug("Se acepto una Conexion por Socket desde la IP: %s  Puerto: %d \n",  Sockets_obtener_ip_cliente(socketCliente),   Sockets_obtener_puerto_cliente(socketCliente) );
	return socketCliente;
}





uint32_t Sockets_recibir_Datos(tipo_socket* socket, char** buffer, t_header headerRecibido) {
	//Primero chequeo que no sea NULL el puntero al Socket. Luego hago otros Chequeos.
	if( socket==NULL ){
		Macro_ImprimirParaDebug("Error, No se puede Recibir Datos por un Socket que es NULL (fijate que el puntero que pasaste apunta a NULL)\n");
		//Corto la ejecucion porque daria error al recibir datos sino (mas abajo).
		return -1;
	}else if(headerRecibido.payload_size<=0){
		Macro_ImprimirParaDebug("Error en Sockets_recibir_Datos. En el 3er Parametro el 'headerRecibido.payload_size' es Menor o Igual a 0. Valor del 3er Parametro: %d", (int)headerRecibido.payload_size);
		//Corto la ejecucion porque daria error al recibir datos sino (mas abajo).
		return -1;
	}
	//Reservo la Memoria para el Payload
	*buffer = reservar_memoria_payload(headerRecibido.payload_size);
	
	uint32_t bytesRecibidos;
	//Llamo a mi funcion interna para recibir los datos, para que haga el "trabajo sucio"
	bytesRecibidos = Sockets_funcion_interna_recibir_datos(socket, *buffer, headerRecibido.payload_size);
	
	//Controlo Errores
	if (bytesRecibidos <= 0) {
		//Si los Bytes recibidos son 0 o menos, hubo un error, asi que yo cierro el Socket, asi evitamos problemas.
		Sockets_cerrar_desconectar(socket);
		return bytesRecibidos;
	}
	//Ahora voy a imprimir el payload recibido
	Sockets_imprimir_payload(*buffer, headerRecibido.payload_size);

	//Para evitar Problemas con Futuros Printf
	fflush(stdout);
	Macro_ImprimirParaDebug("\n");
	fflush(stdout);

	return bytesRecibidos;
}

uint32_t Sockets_recibir_Header(tipo_socket* socket, t_header* headerRecibido) {
	//Primero chequeo que no sea NULL el puntero al Socket.
	if (socket == NULL) {
		Macro_ImprimirParaDebug("Error, No se puede Recibir Datos por un Socket que es NULL (fijate que el puntero que pasaste apunta a NULL)\n");
		//Corto la ejecucion porque daria error al recibir datos sino (mas abajo).
		return -1;
	}

	//Variable para el header serializado
	char sHeaderSerializado[HEADER_SIZE];

	uint32_t bytesRecibidos;
	//Llamo a mi funcion interna para recibir los datos, para que haga el "trabajo sucio"
	bytesRecibidos = Sockets_funcion_interna_recibir_datos(socket, (char*) sHeaderSerializado, HEADER_SIZE);

	//Controlo Errores
	if (bytesRecibidos <= 0) {
		//Si los Bytes recibidos son 0 o menos, hubo un error, asi que yo cierro el Socket, asi evitamos problemas.
		Sockets_cerrar_desconectar(socket);
		return bytesRecibidos;
	}
	//Imprimo el header Recibido
	Sockets_imprimir_header(sHeaderSerializado);

	//Armo el tipo header
	*headerRecibido = header_create(sHeaderSerializado);

	return bytesRecibidos;
}



uint32_t Sockets_enviar_datos( tipo_socket* socket, const char* datosSerializadosConHeader ) {
	//Primero chequeo que no sea NULL el puntero al Socket.
	if( socket==NULL ){
		Macro_ImprimirParaDebug("Error, No se puede Enviar Datos por un Socket que es NULL (fijate que el puntero que pasaste apunta a NULL)\n");
		//Corto la ejecucion porque daria error al enviar datos sino (mas abajo).
		return -1;
	}
	uint32_t bytesPorEnviar = 0;
	//Obtengo del header de "datosSerializadosConHeader" los bytes a enviar
	memcpy(&bytesPorEnviar, datosSerializadosConHeader, sizeof(uint32_t));
	//Controlo que no enviaron cosas raras
	if (bytesPorEnviar <= 0) {
		Macro_ImprimirParaDebug("Error al Enviar, fijate que en el header de los datos serializdos (2do argumento) me pasaste un tamaño 0 o negativo.\n Tamanio detectado: %d", (int)bytesPorEnviar );
		return -1;
	}

	uint32_t bytesEnviados = 0;
	//Llamo a mi funcion interna para enviar los datos, para que haga el "trabajo sucio"
	bytesEnviados = Sockets_funcion_interna_enviar_datos(socket, datosSerializadosConHeader, bytesPorEnviar + HEADER_SIZE);
	
	//Controlo Errores
	if (bytesEnviados <= 0) {
		//Si los Bytes enviados son 0 o menos, hubo un error, asi que yo cierro el Socket, asi evitamos problemas.
		Sockets_cerrar_desconectar(socket);
	}
	//Imprimo el Header Enviado
	Sockets_imprimir_header(datosSerializadosConHeader);
	//Imprimo el Payload Enviado
	Sockets_imprimir_payload(datosSerializadosConHeader + HEADER_SIZE, bytesPorEnviar);

	return bytesEnviados;
}



void Sockets_Select_preparar( tipo_select* unSelect ){
	if( unSelect==NULL ){
		Macro_ImprimirParaDebug("Error, No puedo Preparar un Select que es NULL (fijate que el puntero que me pasaste apunta a NULL)\n");
		return;
	}

	//Inicializa el "unSelect", debe usarse CADA VEZ que queramos ejecutar un Select
	FD_ZERO(unSelect);
	return;
}


void Sockets_Select_agregar( tipo_socket* socket, tipo_select* unSelect ){
	if( socket==NULL ){
		Macro_ImprimirParaDebug("Error, No puedo Agregar un Socket que es NULL (fijate que el puntero que me pasaste apunta a NULL)\n");
		return;
	}else if( unSelect==NULL ){
		Macro_ImprimirParaDebug("Error, No puedo Agregar a un Select que es NULL (fijate que el puntero que me pasaste apunta a NULL)\n");
		return;
	}


	//Agrega el socket a la lista de Descriptores de Sockets, para un Select
	FD_SET(socket->descriptorSocket, unSelect);
	return;
}

int Sockets_Select_esperarEnvios( tipo_socket* ultimoSocketCreado, tipo_select* unSelect, int segundosDeEspera, int microSegundosDeEspera ){
	if( ultimoSocketCreado==NULL ){
		Macro_ImprimirParaDebug("Error, No puedo Esperar Envios en un Socket que es NULL (fijate que el puntero que me pasaste apunta a NULL)\n");
		return -1;
	}else if( unSelect==NULL ){
		Macro_ImprimirParaDebug("Error, No puedo Esperar Envios en un Select que es NULL (fijate que el puntero que me pasaste apunta a NULL)\n");
		return -1;
	}
	
	//Variable para Manejar Errores
	int numeroError;
	
	//Creo una variable para cargar los Segundos De Espera
		struct timeval tiempoEspera;
		//Segundos
		tiempoEspera.tv_sec = segundosDeEspera;
		//Microsegundos
		tiempoEspera.tv_usec = microSegundosDeEspera;
	
	
	//Ejecuto el Select
	numeroError = select(ultimoSocketCreado->descriptorSocket + 1, unSelect, NULL, NULL, &tiempoEspera );
	
	//Controlo Errores
	if (  numeroError==-1  ){
		Macro_ImprimirParaDebug("Error al Esperar Envios\n");
		Macro_ImprimirParaDebug("Error Detectado:" ANSI_COLOR_RED "%s" ANSI_COLOR_RESET "\n", Macro_Obtener_Errno());
		//Si hubo un error, inicializo la lista de Descriptores del Select, asi me evito problemas futuros;
		Sockets_Select_preparar(unSelect);
		return numeroError;

	}else if ( numeroError==0 ){
		//Da 0 cuando se paso el tiempo y no hubo cambios (no llegaron envios), por ahora no hago nada porque No quiero que lo usen para ver si hay envios, quiero que usen la funcion "Sockets_Select_enviaronAlgo" asi hay un mejor encapsulamiento.
		return numeroError;
	}
		
	return numeroError;
}

int Sockets_Select_enviaronAlgo( tipo_socket* socket, tipo_select* unSelect ){
	if( socket==NULL ){
		Macro_ImprimirParaDebug("Error, No puedo fijarme si Enviaron Algo a un Socket que es NULL (fijate que el puntero que me pasaste apunta a NULL)\n");
		return 0;
	}else if( unSelect==NULL ){
		Macro_ImprimirParaDebug("Error, No puedo fijarme si Enviaron Algo en un Select que es NULL (fijate que el puntero que me pasaste apunta a NULL)\n");
		return 0;
	}
	
	//Se fija si para ese Socket tiene algun mensaje/peticion pendiente en la cola. Solo valido para usar despues de haber ejecutado un Select.
	return FD_ISSET( socket->descriptorSocket, unSelect );
}


static void Sockets_imprimir_header(const char* datosSerializadosConHeader){
	Macro_ImprimirParaDebug("El header es: ");
	fflush(stdout);

	//Primero imprimo el tamaño del header
	int32_t bytesPorEnviar = 0;
	//Obtengo del header de "datosSerializadosConHeader" los bytes del tamaño
	memcpy(&bytesPorEnviar, datosSerializadosConHeader, sizeof(uint32_t));
	Macro_ImprimirParaDebug("<%d Bytes que representan al numero %d>", sizeof(uint32_t), bytesPorEnviar);

	//Voy a imprimir caracter por caracter el header del string que me pasaron, empiezo cuando termina el tamaño del header
	int iContadorCaracter;
	//NOTA ESPECIAL: no me gusta poner ese "+1", pero sino imprime caracteres de porqueria, no se porque (tiene que ver con el serializar de Lucas)
	for ( iContadorCaracter = sizeof(uint32_t)+1; iContadorCaracter < HEADER_SIZE; iContadorCaracter++) {

		//Como no puedo imprimir \0 porque Buguea el STDIN, imprimo el \0 como literal
		if(datosSerializadosConHeader[iContadorCaracter]=='\0'){
			Macro_ImprimirParaDebug("\\0");
			fflush(stdout);
		}else{
			Macro_ImprimirParaDebug("%c", (char) datosSerializadosConHeader[iContadorCaracter]);
			fflush(stdout);
		}
	}

	//Para evitar Problemas con Futuros Printf
	Macro_ImprimirParaDebug("\n");
	return;
}

static void Sockets_imprimir_payload(const char* payloadSerializado, const uint32_t tamanioPayload){
	uint32_t cantidadCaracteresPorImprimir = 0;
	Macro_ImprimirParaDebug("El Payload es: ");
	fflush(stdout);

	//DEBUG: Si el Payload es Mayor a MAXIMOS_CARACTERES_PAYLOAD_POR_IMPRIMIR lo bajo hasta ese numero (sino puede ser bestial imprimir los 20 megas de una)
	if (tamanioPayload > MAXIMOS_CARACTERES_PAYLOAD_POR_IMPRIMIR) {
		Macro_ImprimirParaDebug("<El Paylodad es Bastante Grande! (%d Caracteres) No te voy a Imprimir todo, solo los primeros %d Caracteres te Imprimo:\n>", tamanioPayload, MAXIMOS_CARACTERES_PAYLOAD_POR_IMPRIMIR);
		cantidadCaracteresPorImprimir = MAXIMOS_CARACTERES_PAYLOAD_POR_IMPRIMIR;
	} else {
		cantidadCaracteresPorImprimir = tamanioPayload;
	}

	//Voy a imprimir caracter por caracter el payload del string que me pasaron
	uint32_t iContadorCaracter;
	for (iContadorCaracter = 0; iContadorCaracter < cantidadCaracteresPorImprimir; iContadorCaracter++) {
		//Probar con <= a ver que pasa

		//Como no puedo imprimir \0 porque Buguea el STDIN, imprimo el \0 como literal
		if (payloadSerializado[iContadorCaracter] == '\0') {
			Macro_ImprimirParaDebug("\\0");
			fflush(stdout);
		} else if (payloadSerializado[iContadorCaracter] == '\n'){
			Macro_ImprimirParaDebug("\\n");
			fflush(stdout);
		}else {
			Macro_ImprimirParaDebug("%c" , (char) payloadSerializado[iContadorCaracter]);
			fflush(stdout);
		}
	}

	//Para evitar Problemas con Futuros Printf
	Macro_ImprimirParaDebug("\n");
	return;
}


// Funcion interna para recibir los Datos en un Socket y Controlar todos los errores posibles
static uint32_t Sockets_funcion_interna_recibir_datos( const tipo_socket* socket, char* buffer, const uint32_t tamanioBuffer) {
	Macro_ImprimirParaDebug("Recibiendo %d Bytes..\n", tamanioBuffer);
	//Hago el Recv
		//Para que no llegan paquetes a la mitad uso el Flag MSG_WAITALL, que SIEMPRE espera a que llegen todos los datos completos especificados en "tamanioBuffer", y NO se queda tildado si esperamos un paquete de 400 y solo envian 50. (Lo probe y sigue detectando bien los cortes de conexion)
	uint32_t bytesRecibidos = recv(socket->descriptorSocket, buffer, tamanioBuffer, MSG_WAITALL);
	//Controlo Errores
	if(bytesRecibidos <= 0) {

		if(bytesRecibidos == 0){
			Macro_ImprimirParaDebug("Se desconectaron del otro lado del Socket.\n");

		//Ahora uso el "errno" que es el Numero de Error Estandar para atrapar el caso que se Resetio la Conexion
		}else if( errno==ECONNRESET ){
			Macro_ImprimirParaDebug("Se Resetio la Conexion del otro lado a medio Recibir. \nPuede haberse terminado el proceso o el thread (del otro lado del socket) por alguna causa. \n");
			Macro_ImprimirParaDebug("Error Detectado:" ANSI_COLOR_RED "%s" ANSI_COLOR_RESET "\n", Macro_Obtener_Errno());

			//Devuelvo 0 si Resetearon la Conexion del otro lado
			return 0;
		}else{
			Macro_ImprimirParaDebug("Error al recibir Datos\n");
			Macro_ImprimirParaDebug("Error Detectado:" ANSI_COLOR_RED "%s" ANSI_COLOR_RESET "\n", Macro_Obtener_Errno());
		}
	}

	return bytesRecibidos;
}



// Funcion interna para enviar Datos por un Socket y Controlar todos los errores posibles
static uint32_t Sockets_funcion_interna_enviar_datos( const tipo_socket* socket, const char* buffer, const uint32_t bytesPorEnviar) {
	//Variables para Control de Errores
	uint32_t retorno = 0;
	uint32_t bytesEnviados = 0;

	//Todos los Datos puede que no se envien por una unica invocacion de "send" (culpa del Maximo de 64KB por paquete de TCPIP), asi que hago un While hasta que se envien todos los Datos
	while (bytesEnviados < bytesPorEnviar) {
		Macro_ImprimirParaDebug("Enviando %d Bytes..\n", bytesPorEnviar-bytesEnviados);
		//Voy a ir moviendome mas adelante de lo que tengo por enviar si no lo envia de una, por eso sumo al puntero char*. Tambien voy restando los bytes que ya envie, porque me queda menos por enviar. Asi hasta que se envie completo.
			//NOTA: Si hay error porque del otro lado del socket dejaron de hacer "recv" y me esta generando el signal "SIGPIPE", cambiar el 0 por el flag MSG_NOSIGNAL asi almenos no se genera el Signal, luego el error se va a atrapar por "retorno".
		retorno = send(socket->descriptorSocket, (char*)(buffer+bytesEnviados), bytesPorEnviar-bytesEnviados, 0);

		//Controlo Errores
		if( retorno <= 0 ) {
			//Si no envie ningun byte, es que hubo un problema, corto el envio y devuelvo el numero de error.
			//NOTA: Lo que se envio al destino, ya se envio y no tengo manera de cancelarlo, OJO con eso.
			Macro_ImprimirParaDebug("Error al enviar Datos (se corto el Paquete Enviado), solo se enviaron %d bytes de los %d bytes totales por enviar\n", bytesEnviados, (int)bytesPorEnviar);
			Macro_ImprimirParaDebug("Error Detectado:" ANSI_COLOR_RED "%s" ANSI_COLOR_RESET "\n", Macro_Obtener_Errno());
			//Aca siempre deberia enviar -1, pero por las dudas lo dejo asi que es mas compatible.
			bytesEnviados = retorno;
			break;
		}
		//Si no hay problemas, sigo acumulando bytesEnviados
		bytesEnviados += retorno;
	}
	//Ahora controlo que la cantidad de bytesEnviados coincida con los bytesPorEnviar
	if( bytesEnviados!=bytesPorEnviar ){
		Macro_ImprimirParaDebug("No se enviaron todos los bytes pedidos. Esto es Raro, No deberia pasar.\n Se enviaron %d bytes y vos pediste enviar %d bytes\n", bytesEnviados, bytesPorEnviar);
		return 0;
	}

	return bytesEnviados;
}



/*
	NOTAS DE LEO, No lean esto o se van a Pegar un Tiro:
	•Si hay problemas porque enviamos muy pocos bytes y parecen que no se envian, podemos desactivar el Algoritmo de Nagle con el flag TCP_NODELAY en los 2 "setsockopt", pero provoca un rendimiento Pobre de la Red.
	•Podria poner Flag SO_LINGER en los 2 "setsockopt" por si hay problemas de que cerramos Conexiones que tienen pendientes  mensajes por leer (estan en su cola interna de TCP IP), actuaria sobre el "close".
	•Puedo armar una funcion "Sockets_estaAbierto_Conectado" que devuelva un Bool, nomas debo agregar un campo al Struct. Se pone en Conectado inmediatamente antes de hacer los "return" en las funciones "Sockets_conectar_servidor", "Sockets_ponerme_escuchar" y "Sockets_aceptar_cliente" y se pone en Desconectado en "Sockets_cerrar_desconectar".
*/

ssize_t Sockets_enviar_datos_progreso( tipo_socket* socket, const char* datosSerializadosConHeader, size_t bytesPorEnviar, size_t bytesEnviados ) {
	//Primero chequeo que no sea NULL el puntero al Socket.
	if( socket==NULL ){
		Macro_ImprimirParaDebug("Error, No se puede Enviar Datos por un Socket que es NULL (fijate que el puntero que pasaste apunta a NULL)\n");
		//Corto la ejecucion porque daria error al enviar datos sino (mas abajo).
		return -1;
	}

	ssize_t bytesRetorno = 0;
	//Llamo a mi funcion interna para enviar los datos, para que haga el "trabajo sucio"
	bytesRetorno = Sockets_funcion_interna_enviar_datos_progreso(socket, datosSerializadosConHeader, bytesPorEnviar + HEADER_SIZE, bytesEnviados);

	//Imprimo el Header Enviado
	Sockets_imprimir_header(datosSerializadosConHeader);
	//Imprimo el Payload Enviado
	Sockets_imprimir_payload(datosSerializadosConHeader + HEADER_SIZE, bytesPorEnviar);

	return bytesRetorno;
}

ssize_t Sockets_funcion_interna_enviar_datos_progreso( const tipo_socket* socket, const char* buffer, const size_t bytesPorEnviar, size_t bytesEnviados) {
	//Variables para Control de Errores
	ssize_t retorno;
	//Todos los Datos puede que no se envien por una unica invocacion de "send" (culpa del Maximo de 64KB por paquete de TCPIP), asi que hago un While hasta que se envien todos los Datos

	Macro_ImprimirParaDebug("Enviando %d Bytes..\n", bytesPorEnviar-bytesEnviados);
	//Voy a ir moviendome mas adelante de lo que tengo por enviar si no lo envia de una, por eso sumo al puntero char*. Tambien voy restando los bytes que ya envie, porque me queda menos por enviar. Asi hasta que se envie completo.
		//NOTA: Si hay error porque del otro lado del socket dejaron de hacer "recv" y me esta generando el signal "SIGPIPE", cambiar el 0 por el flag MSG_NOSIGNAL asi almenos no se genera el Signal, luego el error se va a atrapar por "retorno".
	retorno = send(socket->descriptorSocket, (char*)(buffer+bytesEnviados), bytesPorEnviar-bytesEnviados, MSG_NOSIGNAL);
	if (errno == EPIPE) {
		Macro_ImprimirParaDebug("Error del Pipe en EnviarBloque\n");
		return 0;
	}

	//Controlo Errores
	if( retorno <= 0 ) {
		//Si no envie ningun byte, es que hubo un problema, corto el envio y devuelvo el numero de error.
		//NOTA: Lo que se envio al destino, ya se envio y no tengo manera de cancelarlo, OJO con eso.
		Macro_ImprimirParaDebug("Error al enviar Datos (se corto el Paquete Enviado), solo se enviaron %d bytes de los %d bytes totales por enviar\n", bytesEnviados, (int)bytesPorEnviar);
		Macro_ImprimirParaDebug("Error Detectado:" ANSI_COLOR_RED "%s" ANSI_COLOR_RESET "\n", Macro_Obtener_Errno());
		//Aca siempre deberia enviar -1, pero por las dudas lo dejo asi que es mas compatible.
		//bytesEnviados = retorno;
		retorno = 0;
	}

	return retorno;
}

uint32_t Sockets_funcion_enviar_datos_directos( const tipo_socket* socket, const char* buffer, uint32_t bytesPorEnviar) {
	//Primero chequeo que no sea NULL el puntero al Socket.
	if( socket==NULL ){
		Macro_ImprimirParaDebug("Error, No se puede Enviar Datos por un Socket que es NULL (fijate que el puntero que pasaste apunta a NULL)\n");
		//Corto la ejecucion porque daria error al enviar datos sino (mas abajo).
		return -1;
	}

	//Variables para Control de Errores
	uint32_t retorno = 0;
	uint32_t bytesEnviados = 0;

	//Todos los Datos puede que no se envien por una unica invocacion de "send" (culpa del Maximo de 64KB por paquete de TCPIP), asi que hago un While hasta que se envien todos los Datos
	while (bytesEnviados < bytesPorEnviar) {
		Macro_ImprimirParaDebug("Enviando %d Bytes..\n", bytesPorEnviar-bytesEnviados);
		//Voy a ir moviendome mas adelante de lo que tengo por enviar si no lo envia de una, por eso sumo al puntero char*. Tambien voy restando los bytes que ya envie, porque me queda menos por enviar. Asi hasta que se envie completo.
			//NOTA: Si hay error porque del otro lado del socket dejaron de hacer "recv" y me esta generando el signal "SIGPIPE", cambiar el 0 por el flag MSG_NOSIGNAL asi almenos no se genera el Signal, luego el error se va a atrapar por "retorno".
		retorno = send(socket->descriptorSocket, (char*)(buffer+bytesEnviados), bytesPorEnviar-bytesEnviados, 0);

		//Controlo Errores
		if( retorno <= 0 ) {
			//Si no envie ningun byte, es que hubo un problema, corto el envio y devuelvo el numero de error.
			//NOTA: Lo que se envio al destino, ya se envio y no tengo manera de cancelarlo, OJO con eso.
			Macro_ImprimirParaDebug("Error al enviar Datos (se corto el Paquete Enviado), solo se enviaron %d bytes de los %d bytes totales por enviar\n", bytesEnviados, (int)bytesPorEnviar);
			Macro_ImprimirParaDebug("Error Detectado:" ANSI_COLOR_RED "%s" ANSI_COLOR_RESET "\n", Macro_Obtener_Errno());
			//Aca siempre deberia enviar -1, pero por las dudas lo dejo asi que es mas compatible.
			bytesEnviados = retorno;
			break;
		}
		//Si no hay problemas, sigo acumulando bytesEnviados
		bytesEnviados += retorno;
	}
	//Ahora controlo que la cantidad de bytesEnviados coincida con los bytesPorEnviar
	if( bytesEnviados!=bytesPorEnviar ){
		Macro_ImprimirParaDebug("No se enviaron todos los bytes pedidos. Esto es Raro, No deberia pasar.\n Se enviaron %d bytes y vos pediste enviar %d bytes\n", bytesEnviados, bytesPorEnviar);
		return 0;
	}

	//Controlo Errores
	if (bytesEnviados <= 0) {
		//Si los Bytes enviados son 0 o menos, hubo un error, asi que yo cierro el Socket, asi evitamos problemas.
		Sockets_cerrar_desconectar(socket);
	}
	//Imprimo el Header Enviado
	Sockets_imprimir_header(buffer);
	//Imprimo el Payload Enviado
	Sockets_imprimir_payload(buffer + HEADER_SIZE, bytesPorEnviar);

	return bytesEnviados;

}
