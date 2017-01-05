/*
Contiene todas las funciones relacionadas con el uso de Sockets. Para conectar, desconectar, enviar y recibir datos. 
*/
 
#include "../tests/Biblioteca_Sockets.h"
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <netdb.h>

//Si hay problemas de "Conexiones Rechazadas" en los Clientes, hay que aumentar este Numero
#define CONEXIONES_MAXIMAS_DE_ESCUCHA					20


const char* Sockets_obtener_ip_cliente( tipo_socket* socketServidorConClienteAceptado ){
	//String donde guardo la IP del Cliente Remoto, defino de IPv6 para que seguro alcanze el tamaño
	//Es una pelotudes pero necesesito SI o SI una variable porque asi trabaja inet_ntop.
	char ipCliente[INET6_ADDRSTRLEN];
	
	//Obtengo la IP del cliente remoto
	return inet_ntop(AF_INET, &(socketServidorConClienteAceptado->tDireccionClienteRemoto.sin_addr), ipCliente, sizeof(ipCliente));
}

int Sockets_obtener_puerto_cliente( tipo_socket* socketServidorConClienteAceptado ){
	return ntohs(socketServidorConClienteAceptado->tDireccionClienteRemoto.sin_port);
}

//Un Cliente (yo) me conecto a un servidor Remoto, especificando su IP y Puerto. Me devuelve (en caso correcto) un puntero a un socket que Ya Esta Conectado al servidor (ya puedo recibir y enviar datos). Ante un error, imprime por pantalla que paso y devuelve NULL.
	//Dentro la funcion crea todas las estructuras necesarias y el tipo_socket
tipo_socket* Sockets_conectar_servidor(const char* serverIP, const char* serverPort) {
	//Estructuras para la busqueda de IPs disponibles donde escuchar (Todo esto es porque por alguna razon pueden haber duplicados[Ej: duplicadas las direcciones por IPv4 e IPv6] y algunos de esos duplicados pueden no funcionar).
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
		fprintf(stderr, "Hubo un error al usar Getaddrinfo para cargar las estructuras de datos del Nuevo Socket, el Error Detectado es: %s\n", gai_strerror(numeroError));
		return NULL;
	}
	
	//TODO meter todo lo que sigue en un Ciclo FOR como hize mas abajo, recorriendo los "resultadoBusqueda" hasta encontrar uno valido y haciendo printf "reintentando.."
	//NOTA: recordar sacar los Return NULL y poner Continue, el Return NULL lo hago al final si no  hay mas resultadoBusqueda
	
	
	//Ciclamos entre todos los Resultados obtenidos y me Enlazo (bind) al primero que puedo
		//Puntero para ciclar por los Resultados
		struct addrinfo *punteroResultado;
	for(punteroResultado = resultadoBusqueda; punteroResultado != NULL; punteroResultado = punteroResultado->ai_next) {
		
		
		
		//Creo un nuevo socket con los valores obtenidos por "getaddrinfo".
		socketConectadoAlServidor->descriptorSocket = socket(resultadoBusqueda->ai_family, resultadoBusqueda->ai_socktype, resultadoBusqueda->ai_protocol);
		if( socketConectadoAlServidor->descriptorSocket==-1 ){
			perror("Error al crear el Socket. \n Error Detectado:");
			printf("Reintentando...\n");
			//Como hubo un error con este resultado, salto al proximo ciclo con el proximo resultado.
			continue;
		}
			
			
			
			
		//Establesco los Flags Necesarios en el socket recien creado.
			//SO_REUSEADDR hace que se pueda reutilizar inmediatamente el socket despues de hacerle un Close (asi no da error de "socket ocupado")
			
			//Variable para Establecer los Flag en True.
			int verdadero=1;
		if (setsockopt(socketConectadoAlServidor->descriptorSocket, SOL_SOCKET, SO_REUSEADDR, &verdadero,  sizeof(int)) == -1) {
			perror("No se pudieron establecer las Opciones/Flags Necesarias en el socket\n Error Detectado:");
			printf("Reintentando...\n");
			//Como hubo un error con este resultado, salto al proximo ciclo con el proximo resultado.
			continue;
		}
			
			
		
		//Borrar, para DEBUG
		//printf("Paso la parte de Crear el Socket\n");
		
		
		
		/*Comentario Viejo, pero puede llegar a ser util:
		No se si aca hace falta un BIND de mi socket "socketConectadoAlServidor" con una IP y Puerto para el Cliente. Y si es por esto que me da que el server esta ocupado
		NO, era que el server no esta haciendo ningun accept. Hay que usar justamente el SELECT, como No Bloqueante o sino tener un Hilo que Solo Acepte Peticiones y cree otros hilos para ejecutar estas peticiones (es la idea de cliente servidor).
		*/
		
		//No hace falta realizar ningun Bind porque nosotros seriamos el Cliente que se quiere conectar al servidor. El Bind es obligatorio para hacer Listen
		//Al no definir un Puerto con el bind, al hacer connect va a buscar un Puerto que No este siendo Usado y lo va a utilizar.
		
		
		
		
		//Me conecto al Servidor remoto con los valores obtenidos por "getaddrinfo".
		numeroError = connect(socketConectadoAlServidor->descriptorSocket, resultadoBusqueda->ai_addr, resultadoBusqueda->ai_addrlen);
		if( numeroError!=0 ){
			perror("Error al conectarse Al Servidor. \n Error Detectado:");
			printf("Reintentando...\n");
			continue;
		}
		
		
		
		//Si llego hasta aca es que no hubo errores y se puedo conectar, entonces salgo del Ciclo.
		break;
	}
	
	
	//Chequeo que no se hallan recorridos todos los resultados, porque entonces significa que no nos pudimos conectar de ninguna manera al servidor.
	if (punteroResultado == NULL) {
		// FIXME: memory leak
		fprintf(stderr, "No hubo ninguna manera de Conectarse al Servidor.\n");
		return NULL;
	}
	
	

	//Por los memory Leaks libero aca al struct "resultadoBusqueda" que use con "getaddrinfo".
	freeaddrinfo(resultadoBusqueda);
	
	//Como llegados aca me pude Conectar al Servidor, Devuelvo el Socket al servidor Ya conectado
	return socketConectadoAlServidor;
}



void Sockets_cerrar_desconectar(tipo_socket* socket) {
	//Primero Reviso que no sea un puntero NULL
	if( socket==NULL ){
		printf("Error, No se puede Cerrar un Socket que es NULL (fijate que el puntero que pasaste para cerrar apunta a NULL)\n");
		//Corto la ejecucion, sino da error en el "close" o en el "free" de mas abajo.
		return;
	}
	
	//Si no es NULL, entonces yo lo libero
	int numeroError;
	numeroError = close(socket->descriptorSocket);
	if( numeroError==-1 ){
		perror("Hubo un problema y no se pudo Cerrar el Socket. \n Esto es muy raro, no deberia pasar nunca. \nError Detectado: ");
	}
	free(socket);
	
	return;
}



tipo_socket* Sockets_ponerme_escuchar(const char* puertoDeEscucha) {
	//Primero Chequeo que no usen sockets Reservados
	if( atol(puertoDeEscucha)<1024 ){
		printf("Estas usando un puerto Reservado (Menor a 1024), Me pediste el puerto: %s \n", puertoDeEscucha);
		return NULL;
	}
	
	
	//Estructuras para la busqueda de IPs disponibles donde escuchar (Todo esto es porque por alguna razon pueden haber duplicados[Ej: duplicadas las direcciones por IPv4 e IPv6] y algunos de esos duplicados pueden no funcionar).
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
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(numeroError));
		return NULL;
	}
	
	
	//Ciclamos entre todos los Resultados obtenidos y me Enlazo (bind) al primero que puedo
		//Puntero para ciclar por los Resultados
		struct addrinfo *punteroResultado;
	for(punteroResultado = resultadoBusqueda; punteroResultado != NULL; punteroResultado = punteroResultado->ai_next) {
		
		//Intento crear un socket con los resultados obtenidos (Familia IPv4, Socket Tipo TCP, Solo para el Protocolo TCP IP)
		if ((socketParaEscuchar->descriptorSocket = socket(punteroResultado->ai_family, punteroResultado->ai_socktype,
							 punteroResultado->ai_protocol)) == -1) {
			perror("No se pudo crear un Socket con esta Direccion, Reintentando... \n Error Detectado:");
			//Como hubo un error con este resultado, salto al proximo ciclo con el proximo resultado.
			continue;
		}
		
		//Establesco los Flags Necesarios en el socket recien creado.
			//SO_REUSEADDR hace que se pueda reutilizar inmediatamente el socket despues de hacerle un Close (asi no da error de "socket ocupado")
			
			//Variable para Establecer los Flag en True.
			int verdadero=1;
		if (setsockopt(socketParaEscuchar->descriptorSocket, SOL_SOCKET, SO_REUSEADDR, &verdadero,
						sizeof(int)) == -1) {
			perror("No se pudieron establecer las Opciones/Flags Necesarias en el socket, Reintentando...\n Error Detectado:");
			//Como hubo un error con este resultado, salto al proximo ciclo con el proximo resultado.
			continue;
		}

		//Debemos hacer Bind porque luego vamos a hacer Listen, asi sabe que puerto va a escuchar.
		if (bind(socketParaEscuchar->descriptorSocket, punteroResultado->ai_addr, punteroResultado->ai_addrlen) == -1) {
			close(socketParaEscuchar->descriptorSocket);
			perror("No se pudo enlazar en esta Direccion, Reintentando...\n  Error Detectado:");
			//Como hubo un error con este resultado, salto al proximo ciclo con el proximo resultado.
			continue;
		}
		
		//Si llego hasta aca es que no hubo errores y se puedo enlazar, entonces salgo del Ciclo.
		break;
	}
	
	//Chequeo que no se hallan recorridos todos los resultados, porque entonces significa que no nos pudimos enlazar a ninguno.
	if (punteroResultado == NULL) {
		fprintf(stderr, "No se pudo Enlazar el Socket.\n");
		return NULL;
	}
	
	//Libero de la memoria la estructura usada
	freeaddrinfo(resultadoBusqueda);
	
	//Hacemos el Listen
	if (listen(socketParaEscuchar->descriptorSocket, CONEXIONES_MAXIMAS_DE_ESCUCHA) == -1) {
		perror("No se pudo poner en Escucha al Socket.\n   Error Detectado:");
		return NULL;
	}
	
	//Como ya se pudo enlazar y poner en escucha el socket, lo devuelvo.
	return socketParaEscuchar;
}



tipo_socket* Sockets_aceptar_cliente(tipo_socket* socketEnEscucha) {
	//Primero chequeo que no sea NULL el puntero al socketEnEscucha.
	if( socketEnEscucha==NULL ){
		printf("Error, No puedo Aceptar Clientes por un Socket que es NULL (fijate que el puntero que pasaste apunta a NULL)\n");
		//Corto la ejecucion porque daria error al recibir datos sino (mas abajo).
		return NULL;
	}
	
	//Estructuras necesarias para pasarle al Accept
	struct sockaddr_in tDireccionClienteRemoto;
	socklen_t tamanioDireccionCliente = sizeof(tDireccionClienteRemoto);
	
	//Creo un socket para guardar al cliente que reciba
	tipo_socket* socketCliente = (tipo_socket*)malloc(sizeof(tipo_socket));

	//Acepto al Cliente
	socketCliente->descriptorSocket = accept(socketEnEscucha->descriptorSocket, (struct sockaddr*)&tDireccionClienteRemoto, &tamanioDireccionCliente);
	//Chequeo Errores
	if (socketCliente->descriptorSocket == -1) {
		// FIXME: memory leak
		perror("No se pudo aceptar al Cliente. El Error Detectado es: ");
		return NULL;
	}
	//Guardo dentro del tipo_socket la direccion (IP y Puerto) del cliente remoto
	socketCliente->tDireccionClienteRemoto = tDireccionClienteRemoto;
	
	//BORRAR
	//Obtengo la IP del cliente remoto
	//inet_ntop(AF_INET, &(tDireccionClienteRemoto.sin_addr), ipCliente, sizeof(ipCliente));

	//TODO imprimir el puerto que se conecto.
	printf("Se acepto una Conexion por Socket desde la IP: %s  Puerto: %d \n",  Sockets_obtener_ip_cliente(socketCliente),   Sockets_obtener_puerto_cliente(socketCliente) );
	return socketCliente;
}



// Funcion interna para recibir los Datos en un Socket y Controlar todos los errores posibles
int Sockets_funcion_interna_recibir_datos(tipo_socket* socket, char* buffer, unsigned int tamanioBuffer) {
	//Hago el Recv
		//Para que no llegan paquetes a la mitad podria usar el Flag MSG_WAITALL, que SIEMPRE espera a que llegen todos los datos completos especificados en "tamanioBuffer", pero se queda tildado si esperamos un paquete de 400 y solo envian 50. (Lo probe y sigue detectando bien los cortes de conexion, asi que parece que NO afecta a eso)
	int bytesRecibidos = recv(socket->descriptorSocket, buffer, tamanioBuffer, 0);
	//Controlo Errores
	if(bytesRecibidos <= 0) {
		
		if(bytesRecibidos == 0)
			printf("Se desconectaron del otro lado del Socket.\n");
		
		//Ahora uso el "errno" que es el Numero de Error Estandar para atrapar el caso que se Resetio la Conexion
		else if( errno==ECONNRESET ) {
			perror("Se Resetio la Conexion del otro lado a medio ejecutar. \nPuede haberse terminado el proceso o el thread (del otro lado del socket) por alguna causa. \nError Detectado: ");
		
			//Devuelvo 0 si Resetearon la Conexion del otro lado
			return 0;
		}
		else
			perror("Error al recibir Datos, el Error detectado es: ");
	}

	return bytesRecibidos;
}



// Funcion interna para enviar Datos por un Socket y Controlar todos los errores posibles
int Sockets_funcion_interna_enviar_datos(tipo_socket* socket, char* buffer, unsigned int bytesPorEnviar) {
	//Variables para Control de Errores
	int retorno;
	int bytesEnviados = 0;
	
	//Todos los Datos puede que no se envien por una unica invocacion de "send" (culpa del Maximo de 64KB por paquete de TCPIP), asi que hago un While hasta que se envien todos los Datos
	while (bytesEnviados < (int)bytesPorEnviar) {
		//Voy a ir moviendome mas adelante de lo que tengo por enviar si no lo envia de una, por eso sumo al puntero char*. Tambien voy restando los bytes que ya envie, porque me queda menos por enviar. Asi hasta que se envie todo.
			//NOTA: Si hay error porque del otro lado del socket dejaron de hacer "recv" y me esta generando el signal "SIGPIPE", cambiar el 0 por el flag MSG_NOSIGNAL asi almenos no se genera el Signal, luego el error se va a atrapar por "retorno".
		retorno = send(socket->descriptorSocket, (char*)(buffer+bytesEnviados), bytesPorEnviar-bytesEnviados, 0);
		
		//Controlo Errores
		if( retorno <= 0 ) {
			//Si no envie ningun byte, es que hubo un problema, corto el envio y devuelvo el numero de error.
			//NOTA: Lo que se envio al destino, ya se envio y no tengo manera de cancelarlo, OJO con eso.
			printf("Error al enviar Datos (se corto el Paquete Enviado), solo se enviaron %d bytes de los %d bytes totales por enviar\n", bytesEnviados, (int)bytesPorEnviar);
			perror("El Error Detectado es: ");
			//Aca siempre deberia enviar -1, pero por las dudas lo dejo asi que es mas compatible.
			bytesEnviados = retorno;
			break;
		}
		//Si no hay problemas, sigo acumulando bytesEnviados
		bytesEnviados += retorno;
	}
	//Ahora controlo que la cantidad de bytesEnviados coincida con los bytesPorEnviar
	if( bytesEnviados!=(int)bytesPorEnviar ){
		printf("No se enviaron todos los bytes pedidos. Esto es Raro, No deberia pasar.\n Se enviaron %d bytes y vos pediste enviar %d bytes\n", bytesEnviados, bytesPorEnviar);
		return 0;
	}

	return bytesEnviados;
}



int Sockets_recibir_Datos(tipo_socket* socket, char* buffer, unsigned int tamanioBuffer) {
	//Primero chequeo que no sea NULL el puntero al Socket.
	if( socket==NULL ){
		printf("Error, No se puede Recibir Datos por un Socket que es NULL (fijate que el puntero que pasaste apunta a NULL)\n");
		//Corto la ejecucion porque daria error al recibir datos sino (mas abajo).
		return -1;
	}
	
	int bytesRecibidos;
	//Llamo a mi funcion interna para recibir los datos, para que haga el "trabajo sucio"
	bytesRecibidos = Sockets_funcion_interna_recibir_datos(socket, (char*)buffer, tamanioBuffer);
	
	//Controlo Errores
	if (bytesRecibidos <= 0) {
		//Si los Bytes recibidos son 0 o menos, hubo un error, asi que yo cierro el Socket, asi evitamos problemas.
		Sockets_cerrar_desconectar(socket);
	}
	return bytesRecibidos;
}



int Sockets_enviar_datos(tipo_socket* socket, char* buffer) {
	//Primero chequeo que no sea NULL el puntero al Socket.
	if( socket==NULL ){
		printf("Error, No se puede Enviar Datos por un Socket que es NULL (fijate que el puntero que pasaste apunta a NULL)\n");
		//Corto la ejecucion porque daria error al enviar datos sino (mas abajo).
		return -1;
	}
	
	int bytesEnviados = 0;
	int bytesPorEnviar = strlen(buffer) + 1;
	//Llamo a mi funcion interna para enviar los datos, para que haga el "trabajo sucio"
	bytesEnviados = Sockets_funcion_interna_enviar_datos(socket, (char*)buffer, bytesPorEnviar);
	
	//Controlo Errores
	if (bytesEnviados <= 0) {
		//Si los Bytes enviados son 0 o menos, hubo un error, asi que yo cierro el Socket, asi evitamos problemas.
		Sockets_cerrar_desconectar(socket);
	}
	return bytesEnviados;
}

/*
	NOTAS DE LEO, No lean esto o se van a Pegar un Tiro:
	•Si hay problemas porque enviamos muy pocos bytes y parecen que no se envian, podemos desactivar el Algoritmo de Nagle con el flag TCP_NODELAY en los 2 "setsockopt", pero provoca un rendimiento Pobre de la Red.
	•Podria poner Flag SO_LINGER en los 2 "setsockopt" por si hay problemas de que cerramos Conexiones que tienen pendientes  mensajes por leer (estan en su cola interna de TCP IP), actuaria sobre el "close".
	•Puedo armar una funcion "Sockets_estaAbierto_Conectado" que devuelva un Bool, nomas debo agregar un campo al Struct. Se pone en Conectado inmediatamente antes de hacer los "return" en las funciones "Sockets_conectar_servidor", "Sockets_ponerme_escuchar" y "Sockets_aceptar_cliente" y se pone en Desconectado en "Sockets_cerrar_desconectar".
*/


void Sockets_Select_preparar( tipo_select* unSelect ){
	//Inicializa el "unSelect", debe usarse CADA VEZ que queramos ejecutar un Select
	FD_ZERO(unSelect);
	return;
}


void Sockets_Select_agregar( tipo_socket* socket, tipo_select* unSelect ){
	if( socket==NULL ){
		printf("Error, No puedo Agregar un Socket que es NULL (fijate que el puntero que me pasaste apunta a NULL)\n");
		return;
	}
	//Agrega el socket a la lista de Descriptores de Sockets, para un Select
	FD_SET(socket->descriptorSocket, unSelect);
	return;
}

int Sockets_Select_esperarEnvios( tipo_socket* ultimoSocketCreado, tipo_select* unSelect, int segundosDeEspera, int microSegundosDeEspera ){
	if( ultimoSocketCreado==NULL ){
		printf("Error, No puedo Esperar Envios en un Socket que es NULL (fijate que el puntero que me pasaste apunta a NULL)\n");
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
			perror("Error al Esperar Envios, el Error Detectado es :");
			//Si hubo un error, inicializo la lista de Descriptores del Select, asi me evito problemas futuros;
			Sockets_Select_preparar( unSelect );
			return numeroError;
			
	}else if ( numeroError==0 ){
		//Da 0 cuando se paso el tiempo y no hubo cambios (no llegaron envios), por ahora no hago nada porque No quiero que lo usen para ver si hay envios, quiero que usen la funcion "Sockets_Select_enviaronAlgo" asi hay un mejor encapsulamiento.
		return numeroError;
	}
		
	return numeroError;
}

int Sockets_Select_enviaronAlgo( tipo_socket* socket, tipo_select* unSelect ){
	if( socket==NULL ){
		printf("Error, No puedo fijarme si Enviaron Algo a un Socket que es NULL (fijate que el puntero que me pasaste apunta a NULL)\n");
		return 0;
	}
	
	//Se fija si para ese Socket tiene algun mensaje/peticion pendiente en la cola. Solo valido para usar despues de haber ejecutado un Select.
	return FD_ISSET( socket->descriptorSocket, unSelect );
}
