/*
Contiene todas las funciones relacionadas con el uso de Sockets. Para conectar, desconectar, enviar y recibir datos. 
	TODAS las funciones ya manejan errores y los imprimen por pantalla. Asi que ni hace falta usar un if para atajar el error, a menos que quieras hacer algo especial ante un error determinado.
	
ORDEN DE USO:
-->Siendo un Servidor:
	-Uso Sockets_ponerme_escuchar
	-Uso Sockets_aceptar_cliente
		-Ya puedo usar tanto Sockets_recibir_Datos como Sockets_enviar_datos las veces que quiera
		-Cuando dejo de atender a un cliente uso Sockets_cerrar_desconectar con el Socket Conectado al Cliente
	-Antes de Salir del programa uso Sockets_cerrar_desconectar con el Socket en Escucha
	
-->Siendo un Cliente:
	-Uso Sockets_conectar_servidor
		-Ya puedo usar tanto Sockets_recibir_Datos como Sockets_enviar_datos las veces que quiera
	-Antes de Salir del programa uso Sockets_cerrar_desconectar con el Socket conectado al Servidor
	
--NOTA: Antes de realizar un "Sockets_recibir_Datos" siempre deberian realizar un "Sockets_recibir_Header", para con el Header obtener el tamaño del payload del mensaje que te enviaron para saber que tamaño de buffer reservar y, recien ahí poder llamar a "Sockets_recibir_Datos".

Ahora bien, si queremos utilizar un SELECT (para tener un Tiempo Maximo de espera al recibir mensajes o para atender a Varios clientes en un mismo Thread/Proceso)
	-Usar Sockets_Select_preparar
	-Usar Sockets_Select_agregar (tantas veces como sockets queramos escuchar)
	-Usar Sockets_Select_esperarEnvios
	-Usar Sockets_Select_enviaronAlgo para ver si llego algo (tantas veces como sockets hayamos agregado)
		-Por cada socket que le enviaron algo uso Sockets_recibir_Datos
	NOTA: Estas funciones Deben ser ejecutadas en este orden CADA VEZ que queramos realizar un SELECT.
*/

#ifndef BIBLIOTECA_SOCKETS_H
#define BIBLIOTECA_SOCKETS_H

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "../Serializador/serialized_package.h"




//Un Struct que contiene los datos necesarios para establecer una conexion por socket (el File Descriptor del Socket y la Direccion Remota del Cliente).
typedef struct {
	int descriptorSocket;
	//Solo lo usa el servidor, para guardar la Direccion del Cliente cuando el se conecte (y yo lo acepte).
	struct sockaddr_in tDireccionClienteRemoto;
} tipo_socket;


//Tipo de Dato para el Select, lo renombro asi es mas facil de entender, a ustedes no deberia importarles que es un "fd_set".
typedef fd_set tipo_select;





const char* Sockets_obtener_ip_cliente( const tipo_socket* socketServidorConClienteAceptado );
	//Dado un socketServidorConClienteAceptado devuelve la IP del Cliente. SI yo lo aplico a un Socket que no tenga un Cliente Aceptado, devuelve Basura.
	//Osea que NO puede aplicarse a Sockets donde YO soy el Cliente.


int Sockets_obtener_puerto_cliente( const tipo_socket* socketServidorConClienteAceptado );
	//Dado un socketServidorConClienteAceptado devuelve el Puerto del Cliente. SI yo lo aplico a un Socket que no tenga un Cliente Aceptado, devuelve Basura.
	//Osea que NO puede aplicarse a Sockets donde YO soy el Cliente.


void Sockets_cerrar_desconectar(  tipo_socket* socket  );
	//Cierro el Socket y libero los Recursos que usaba.


bool Sockets_estaDisponibleServidor(  const char* serverIP,   const char* serverPort  );
	//Verifica si el Servidor Esta Disponible (No Murio) en ESTE Momento. Devuelve True Si esta Disponible

	
tipo_socket* Sockets_ponerme_escuchar(  const char* puertoDeEscucha  );
	/*	Establecer un Socket Como un Servidor De Escucha de Nuevas Conexiones por Socket.
		Devuelve un NUEVO SOCKET puesto en escucha en el puerto especificado.
		Ante un error, imprime por pantalla que paso y devuelve NULL
	*/
	//NOTA: Esta funcion va a reintentar varias veces antes de dar error, asi que es normal que aparescan mensajes de "reintentando.." (si no llegan a aparecer, Mejor!)

	
tipo_socket* Sockets_aceptar_cliente(  const tipo_socket* socketEnEscucha  );
	/* 	Acepta un Nuevo Cliente en el socket puesto en Escucha.
		Devuelve un NUEVO SOCKET conectado al cliente bidireccionalmente, con el cual se puede enviar y recibir de SOLAMENTE ese cliente aceptado.
		Ante un error, imprime por pantalla que paso y devuelve NULL
	*/
	//NOTA: Esta funcion es Bloqueante (se queda esperando hasta que se conecta realmente un nuevo Cliente), por eso es mejor usarla en conjunto con un SELECT, sino no hay manera de evitar que se quede tildado el programa.
	
	
tipo_socket* Sockets_conectar_servidor(  const char* serverIP,   const char* serverPort  );
	/*	Un Cliente (Yo) me conecto a un Servidor Remoto, especificando su IP y Puerto. 
		Me devuelve (en caso correcto) un puntero a un socket que Ya Esta Conectado al servidor (ya puedo recibir y enviar datos SOLAMENTE con ese servidor). 
		Ante un error, imprime por pantalla que paso y devuelve NULL.
	*/
	//NOTA: Esta funcion va a intentar conectar varias veces antes de dar error, asi que es normal que aparescan mensajes de "reintentando.." (si no llegan a aparecer, Mejor!)

	
	
	
//Ahora vienen 2 Funciones para enviar y recibir Datos YA serializados con header incluido (convertidos en tipo)
	uint32_t Sockets_recibir_Datos(  tipo_socket* socket,   char** buffer,   const t_header headerRecibido  );
		/*	Recibo (por el socket especificado) Datos y los guarda en "buffer" (deben pasarle un &<un char*>). Debe especificarse el header recibido anteriormente (la idea es que primero usen "Sockets_recibir_Header" antes de esta funcion).
			DEBEN ASEGURARSE de realizar un "liberar_memoria_payload" al char* "buffer" luego de que no lo utilicen mas (NO VALE reutilizarlo 2 veces y hacer un solo liberar, debe liberarse cada vez antes de reusarlo).
			Me devuelve (en caso correcto) la cantidad de bytes que recibi.
			Ante un error, imprime por pantalla que paso y devuelve:
																	0 si se cerro/cayo la conexion desde el otro lado
																	-1 si hubo otro error
			Tambien ante un error Cierra y Libera El Socket, asi  ustedes no se tienen que acordar de Cerrar y liberar el Socket para no generar Memory Leaks ni problemas con los Puertos.
			
		NOTA: Esta Funcion Retira el mensaje (payload) de la cola de Mensajes del Socket, si ustedes solo quieren retirar el Header sin retirar el payload del mensaje usen la funcion "Sockets_recibir_Header".
		NOTA: Esta funcion es Bloqueante (se queda esperando hasta que recibe datos O hasta que se corto la conexion) asi que podes hacer 2 cosas: 
			1-Usar SELECT con un Tiempo Maximo para esperar que se reciba algo. 
			2-Usar la funcion asi como viene y tener en cuenta que puede quedarse tildado el programa (pero no por siempre, ya que detecta que se corto la conexion).   
		En ambos casos vos tendrias que hacer algo (para el programa? avisar a alguien? reintentar?)  si se pasa del tiempo maximo / se corta la conexion.
		*/
		

	uint32_t Sockets_recibir_Header(   tipo_socket* socket,   t_header* headerRecibido  );
	/*	Recibo (por el socket especificado) SOLO el header. Para invocarse debe pasarsele "&<un t_header>".
		Me devuelve (en caso correcto) la misma cantidad que HEADER_SIZE (siempre es mayor a 0).
		Ante un error, imprime por pantalla que paso y devuelve:
																0 si se cerro/cayo la conexion desde el otro lado
																-1 si hubo otro error
		Tambien ante un error Cierra y Libera El Socket, asi  ustedes no se tienen que acordar de Cerrar y liberar el Socket para no generar Memory Leaks ni problemas con los Puertos.
		NOTA: Esta Funcion Retira el header de la cola de Mensajes del Socket, no retira el payload del mensaje.
		NOTA: Esta funcion es Bloqueante (se queda esperando hasta que recibe datos O hasta que se corto la conexion) asi que podes hacer 2 cosas:
			1-Usar SELECT con un Tiempo Maximo para esperar que se reciba algo.
			2-Usar la funcion asi como viene y tener en cuenta que puede quedarse tildado el programa (pero no por siempre, ya que detecta que se corto la conexion).
		En ambos casos vos tendrias que hacer algo (para el programa? avisar a alguien? reintentar?)  si se pasa del tiempo maximo / se corta la conexion.

	 */
		
	uint32_t Sockets_enviar_datos (  tipo_socket* socket,   const char* datosSerializadosConHeader  );
		/*	Envia (por el socket especificado) los Datos guardados en "datosSerializados". Calcula cuando tiene que enviar en base al Header del mensaje serializado en "datosSerializadosConHeader".
			Me devuelve (en caso correcto) la cantidad de bytes que envie (siempre mayor a 0).
			Ante un error, imprime por pantalla que paso y devuelve:
																	0 si se enviaron menos Bytes de los que pediste (se corto el paquete al enviar)
																	-1 si hubo otro error
			Tambien ante un error Cierra y Libera El Socket, asi  ustedes no se tienen que acordar de Cerrar y liberar el Socket para no generar Memory Leaks ni problemas con los Puertos.
		*/
	
	
	
	
	
	
	void Sockets_Select_preparar(  tipo_select* unSelect  );
	//Dado un select lo va a preparar para poder agregar sockets. 
	
	void Sockets_Select_agregar(  tipo_socket* socket,   tipo_select* unSelect  );
	/*	Agrego un Socket al Select, le estoy indicando que cuando espere tenga en cuenta este socket (+ todos los que ya agregue antes).
		NOTA: Cada vez que se vuelva a querer ejecutar un select debemos volver a agregar todos los sockets que queremos esperar envios.
	*/
	
	int Sockets_Select_esperarEnvios(  tipo_socket* ultimoSocketCreado,   tipo_select* unSelect,   int segundosDeEspera,   int microSegundosDeEspera  );
	/*	Para un select espera que llegen envios tantos Segundos y Microsegundos. Espera hasta que llegue envios a, por lo menos, UNO de todos los sockets agregados.
		Es decir, dado un conjunto de sockets agregados al select, espera COMO MAXIMO la suma de Segundos y Microsegundos especificada. Si llegara algun envio Antes del timpo maximo, el Select termina y sigue de largo (luego chequeamos con Sockets_Select_enviaronAlgo).
		Durante el tiempo de espera el programa se encontrara Bloqueado.
		
		Ante Error avisa el error que hubo y devuelve -1.
		
		NOTA: Es re molesto, pero SIEMPRE tenes que pasarle de 1er argumento el Ultimo socket creado (el + nuevo) de todos los sockets que agregaste (si usas siempre 1 le pasas ese y ya).
		NOTA: Si pones ambos segundos en 0, sirve para que no se bloquee y ver en el momento si llegaron envios con la funcion "Sockets_Select_enviaronAlgo". Es dificil que llegues a necesitar esto, pero existe.
	*/
	
	int Sockets_Select_enviaronAlgo(  tipo_socket* socket,   tipo_select* unSelect  );
	/*	Sirve para ver si dado un select, hubo un envio en Uno de los sockets que agregaste.
		Devuelve 1 (verdadero) Si enviaron algo en el Socket que estas consultando y 0 si No enviaron algo.
	*/

	ssize_t Sockets_enviar_datos_progreso( tipo_socket* socket, const char* datosSerializadosConHeader, size_t bytesPorEnviar, size_t bytesEnviados );
	ssize_t Sockets_funcion_interna_enviar_datos_progreso( const tipo_socket* socket, const char* buffer, const size_t bytesPorEnviar, size_t bytesEnviados);
	uint32_t Sockets_funcion_enviar_datos_directos( const tipo_socket* socket, const char* buffer, uint32_t bytesPorEnviar);
#endif 
//BIBLIOTECA_SOCKETS_H
