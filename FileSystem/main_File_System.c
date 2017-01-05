#include <stdlib.h>
#include <string.h>
#include  <signal.h>
#include  <stdio.h>
#include <setjmp.h>
#include <semaphore.h>

#include <commons/config.h>
#include <commons/string.h>


#include "headers/console.h"
#include "headers/Servidor_FileSystem.h"
#include "../Serializador/serialized_package.h"


#include "../Biblioteca_Comun/Biblioteca_Comun.h"		//Para las Constantes y Macros
#include "../Biblioteca_Comun/2048.h"
//#include "main_File_System.h"

#define MAXIMA_CANTIDAD_CARACTERES_POR_LEER 	RUTA_LONGITUD_APROX		//Uso la Longitud Aproximada y no la Maxima porque Consumiria Demasiada Memoria y no es probable que la necesitemos.
												//FIXME Si se cortan los argumentos, cambiar esto por RUTA_LONGITUD_MAXIMA

jmp_buf jump;
sem_t _memcheck_mutex;

int isvarset (void *x);

void segv(int signum, siginfo_t *info, void *ptr);

void segv(int signum, siginfo_t *info, void *ptr) {
	longjmp(jump,1);
}

int isvarset (void *x) {
	int definido = 1;
	volatile int resjump;
	struct sigaction _sigact;
	sigemptyset(&_sigact.sa_mask);
	_sigact.sa_sigaction = segv;
	_sigact.sa_flags = SA_SIGINFO | SA_NODEFER;
	sigaction(SIGSEGV, &_sigact, NULL);

	sem_wait(&_memcheck_mutex);
	resjump = setjmp(jump);
	if (resjump == 0) {
		if ((char*)x == NULL) {
			definido = 0;
		} else {
			definido = 1;
		}
	} else {
		definido = 0;
	}
	sem_post(&_memcheck_mutex);

	sigemptyset(&_sigact.sa_mask);
	_sigact.sa_flags = SA_RESETHAND;
	if (sigaction(SIGSEGV, &_sigact, NULL) == -1){
		return -1;
	}

	return (definido);
}

//Funcion principal para leer la consola, Basicamente toma lo que escribe el usuario, controla Los Arguemtnos Y selecciona a que Comando(Funcion) Llamar
int leerConsola();

//Funcion para Salir del FS, tanto por Consola como por Señal
void salir_Cerrar_FS();

/*
 * Funcion Deprecada
void  INThandler(int sig)
{
	char c, *param;
	memcpy(&param , "" , 1);
	signal(sig , SIG_IGN);
	printf("\n-Fea la actitud de tratar de interrumpir el proceso!\n-Tal vez estes aburrido. ¿Queres jugar al Tetris? [s/n] ");
	c = getchar();

	if (c == 's' || c == 'S') {
		printf("Tetris no tengo, pero te doy el 2048 que esta de Moda..\n");
		sleep(2);
		jugar(0 , &param);
		return;
	} else if (c == 'k' || c == 'K') {
		//Cierro el FS de Manera Correcta
		//sleep();
		salir_Cerrar_FS();
		exit(0);

	} else {
		signal(SIGINT , INThandler);     //Hace Falta Redefinirla Porque usando Signal Solo la Atrapa 1 Vez.
		return;
		//getchar(); // Get new line character
	}
}

void sig_handler_ctrlc(int signum, siginfo_t *info, void *ptr) {
	char c, *param;
	memcpy(&param , "" , 1);
	printf("\n-Fea la actitud de tratar de interrumpir el proceso!\n-Tal vez estes aburrido. ¿Queres jugar al Tetris? [s/n] ");
	c = getchar();

	if (c == 's' || c == 'S') {
		printf("Tetris no tengo, pero te doy el 2048 que esta de Moda..\n");
		sleep(2);
		jugar(0 , &param);
		return;
	} else if (c == 'k' || c == 'K') {
		//Cierro el FS de Manera Correcta
		//sleep();
		salir_Cerrar_FS();
		exit(0);
	}
}
*/
void Ctrl_C_Hanlder(int signum, siginfo_t *info, void *ptr) {
	char c, *param;
	static bool bJugando;

	Servidor_loguear(LOG_LEVEL_INFO, __func__, "###Se ha llamado al Handler###");		//Para llamar La Atencion, por BUGs

	if (signum == SIGTERM) {
		salir_Cerrar_FS();
		exit(-1);
	} else if (signum == SIGINT) {
		if (bJugando == true) {
			printf("\n Cuantas veces queres jugar?? Te voy a sacar el teclado!!");
			return;
		}
		memcpy(&param , "" , 1);
		printf("\n-Fea la actitud de tratar de interrumpir el proceso!\n-Tal vez estes aburrido. ¿Queres jugar al Tetris? [s/n] ");
		c = getchar();

		if (c == 's' || c == 'S') {
			printf("Tetris no tengo, pero te doy el 2048 que esta de Moda..\n");
			sleep(2);
			bJugando = true;
			jugar(0 , &param);
			Macro_LimpiarPantalla();
			Macro_ImprimirEstadoInicio("Retomando la consola\n");
			bJugando = false;
			return;
		} else if (c == 'k' || c == 'K') {
			//Cierro el FS de Manera Correcta
			//sleep();
			salir_Cerrar_FS();
			exit(0);
		}
	}

	Servidor_loguear(LOG_LEVEL_INFO, __func__, "Fin Handler");
}

int main(){
	// Inicializo Semaforo para memcheck
	sem_init(&_memcheck_mutex,0,1);

	//Variable para Controlar Errores, por defecto es 0 que significa que no hay error
	int iNumeroError = 0;

	Macro_LimpiarPantalla();

	Servidor_loguear(LOG_LEVEL_INFO, __func__, "----------------------------------------------------------");
	Servidor_loguear(LOG_LEVEL_INFO, __func__, "----------------------------------------------------------");

	Servidor_loguear(LOG_LEVEL_INFO, __func__, ">>Iniciando File System");
	Comun_Pantalla_Separador_Destacar("Iniciando File System");


	//signal(SIGINT, INThandler);
	static struct sigaction _sigact;
	iNumeroError = sigemptyset(&_sigact.sa_mask);
	if(iNumeroError == -1){
		Servidor_loguear(LOG_LEVEL_WARNING, __func__, "Funciono Mal el SigEmptySet de SigKILL");
	}
	_sigact.sa_sigaction = Ctrl_C_Hanlder;
	_sigact.sa_flags = SA_SIGINFO;
	iNumeroError = sigaction(SIGTERM, &_sigact, NULL);
	if(iNumeroError == -1){
		Servidor_loguear(LOG_LEVEL_WARNING, __func__, "Funciono Mal el SigAction de SigKILL");
	}

	static struct sigaction _sigact2;
	iNumeroError = sigemptyset(&_sigact2.sa_mask);
	if(iNumeroError == -1){
		Servidor_loguear(LOG_LEVEL_WARNING, __func__, "Funciono Mal el SigEmptySet de CTRL-C (SigINT)");
	}

	_sigact2.sa_sigaction = Ctrl_C_Hanlder;
	_sigact2.sa_flags = SA_SIGINFO;
	iNumeroError = sigaction(SIGINT, &_sigact2, NULL);
	if(iNumeroError == -1){
		Servidor_loguear(LOG_LEVEL_WARNING, __func__, "Funciono Mal el SigAction de CTRL-C (SigINT)");
	}

	Macro_ImprimirEstadoInicio("Controlando los Permisos Necesarios");

	//Controlo que hayan los Permisos para poder Ejecutar el File System
	iNumeroError = Comun_controlarPermisos();
	//Chequeo Errores
	if (iNumeroError < 0){
		Macro_ImprimirEstadoFinal(EJECUCION_FALLIDA);
		printf("No se pudo Iniciar el File System\n");
		return 0;
	}
	Macro_ImprimirEstadoFinal(EJECUCION_CORRECTA);

	Macro_ImprimirEstadoInicio("Viendo Si existe El archivo de Configuracion");

	//Verifico si existe el archivo de Configuracion
	iNumeroError = Comun_existeArchivo(RUTA_ARCHIVO_CONFIG_FS);
	if (iNumeroError < 0) {
		Macro_ImprimirEstadoFinal(EJECUCION_FALLIDA);
		printf("El archivo o No existe o No se tienen permisos de Lectura/Escritura/Ejecucion.\n");
		return 0;
	}
	Macro_ImprimirEstadoFinal(EJECUCION_CORRECTA);

	// FIXME: memory leak
	//Abro Archivo de Configuracion
	t_config* archivoConfig = config_create(RUTA_ARCHIVO_CONFIG_FS);

	Macro_ImprimirEstadoInicio("Verificando Archivo de Configuracion");

	//Verifico que tenga los Campos/Claves que necesitamos
	if (config_has_property(archivoConfig , "PUERTO_LISTEN") == false) {
		Macro_ImprimirEstadoFinal(EJECUCION_FALLIDA);
		printf("El archivo Config No tiene el Campo PUERTO_LISTEN\n");
		return 0;
	}
	if (config_has_property(archivoConfig , "CANTIDAD_NODOS_MINIMOS") == false) {
		Macro_ImprimirEstadoFinal(EJECUCION_FALLIDA);
		printf("El archivo Config No tiene el Campo CANTIDAD_NODOS_MINIMOS\n");
		return 0;
	}
	Macro_ImprimirEstadoFinal(EJECUCION_CORRECTA);

	//Cargo en las Variables los Datos del Archivo de Configuracion
	char* puertoEscucha = config_get_string_value(archivoConfig , "PUERTO_LISTEN");
	int cantidadNodosMinimos = config_get_int_value(archivoConfig , "CANTIDAD_NODOS_MINIMOS");


	Comun_Pantalla_Separador_Destacar("Abriendo Indices");

	//Se encargar de Abrir y Cargar en Memoria los Indices que se utilizan en las Tablas de la Base de Datos
	//Si no los abrieramos no podriamos usar la Base de Datos
	iNumeroError = FS_abrir_indices();
	//Chequeo Errores
	if (iNumeroError < 0){
		printf("No se pudo Iniciar el File System\n");
		return 0;
	}

	Comun_Pantalla_Separador_Destacar("Cargando Nodos - Viendo de Reconectar Nodos");
	//Valida que los Nodos Guardados en la Base estan Activos o No. Y tambien Des/Activa los Archivos correspondientes
	iNumeroError = FS_CargarNodos();
	//Chequeo Errores
	if (iNumeroError < 0) {
		printf(ANSI_COLOR_RED "No se pudo Iniciar el File System" ANSI_COLOR_RESET "\n");
		return 0;
	}


	Comun_Pantalla_Separador_Destacar("Esperando Nodos Minimos");


	//Me pongo a Escuchar para Esperar Nodos Minimos
	tipo_socket* socketEscuchando = Sockets_ponerme_escuchar(puertoEscucha);
	if (socketEscuchando==NULL) {
		printf("No se pudo el FS a poner a Escuchar Nodos Minimos\n");
		return 0;
	}

	//Espero que se conecten la cantidad de Nodos Minimos
	int cantidadNodosConectados = 0;
	iNumeroError = FS_ContarNodosActivos(&cantidadNodosConectados);
	if (iNumeroError < 0) {
		printf("Exploto al Tratar de Ver la primera Vez Cuantos Nodos hay Activos\n");
		return 0;
	}


	//PARA DEBUG, saltea la parte de Esperar Nodos, sirve para poder Probar la Consola sin Nodos
	//cantidadNodosConectados = 5;
	//cantidadNodosMinimos = 2;

	if( cantidadNodosConectados >= cantidadNodosMinimos ){
		printf("- Todos los Nodos Minimos Estan Conectado \n");
	}

	while (cantidadNodosConectados < cantidadNodosMinimos) {
		printf("- Nodos Minimos %d/%d\n" , cantidadNodosConectados , cantidadNodosMinimos);

		//Acepto a Alguien, Sea Nodo o NO
		tipo_socket* socketConectadoAlCliente =Sockets_aceptar_cliente(socketEscuchando);
		if (socketEscuchando==NULL) {
			printf("El FS No pudo Aceptar a Alguien, No se quien Es asi que no puedo hacer nada. Se Ignora y Espera proximo Nodo..\n");
			continue;
		}

		//Recibo el Header del Mensaje
		t_header headerRecibido;
		iNumeroError = Sockets_recibir_Header(socketConectadoAlCliente , &headerRecibido);

		//Chequeo Errores
		if (iNumeroError == 0) {
			printf("Se corto la conexion apenas recibimos al cliente, No tenemos manera de saber si era un Nodo o el Marta, asi que no podemos hacer nada\n");
			Sockets_cerrar_desconectar(socketConectadoAlCliente);
			continue;

		} else if (iNumeroError < 0) {
			printf("(Linea %d) Hubo un problema con la conexion apenas recibimos al cliente, se imprimio por pantalla que paso (la funcion de Socket lo hizo)\n" ,__LINE__);
			Sockets_cerrar_desconectar(socketConectadoAlCliente);
			continue;
		}

		//Verifico que sea un Nodo que me este avisando que se quiera conectar, sino lo ignoro
		//FEATURE Podria mandarle un mensaje "Todavia No esta Listo el FILESYSTEM, esta esperando Nodos Minimos".
		if( headerRecibido.sender_id!= NODO || !header_esOrden(headerRecibido,"avisarNodoConectado") ){
			Macro_ImprimirParaDebug("Como Aun no esta Preparado el FS se Ignoro una Orden de: %d, la orden era: %s\n", (int)headerRecibido.sender_id, headerRecibido.order );
			Sockets_cerrar_desconectar(socketConectadoAlCliente);
			continue;
		}

		//Mando a Ejecutar la Orden para que lo de De Alta
		char* nodoNombre = Servidor_realizarOrden_avisarNodoConectado(socketConectadoAlCliente, headerRecibido);
		if(nodoNombre==NULL){
			Macro_ImprimirParaDebug("No se pudo Realizar la Orden de Avisar Nodo conectado\n");
			continue;
		}

		Macro_ImprimirParaDebug("Procedo a Activar al Nodo %s\n",nodoNombre);

		//Establesco como Disponible al Nodo
		console_agregarNodo(nodoNombre);

		//Dentro de la Orden se encarga de Cerra la Conexion

		iNumeroError = FS_ContarNodosActivos(&cantidadNodosConectados);
		if(iNumeroError<0){
			printf("ERROR: No se pudo calcular Cuantos Nodos hay Activos\n");
			return 0;
		}
	}

	///Dejo de Escuchar para Esperar Nodos Minimos
	Sockets_cerrar_desconectar(socketEscuchando);

	Servidor_loguear(LOG_LEVEL_INFO, __func__, ">>Nodos Minimos Listos");

	Comun_Pantalla_Separador_Destacar("Todos los Nodos Minimos Estan Conectados");

	Macro_ImprimirEstadoInicio("Iniciando Servidor");

	//Levanto el Servidor en un Thread
	// FIXME: memory leak
	pthread_t idThread;
	iNumeroError = pthread_create(&idThread, NULL, (void*) &threadBaseServidor,(void*) puertoEscucha);
	//Chequeo Errores al crear el thread
	if (iNumeroError != 0) {
		Macro_ImprimirEstadoFinal(EJECUCION_FALLIDA);
		perror("Ocurrio un Error al crear el Thread Base del Servidor. El Error es");
		return 0;
	}
	Macro_ImprimirEstadoFinal(EJECUCION_CORRECTA);

	//Le digo al Thread que cuando se termine libere los recursos que uso, lo hace el solo y nos olvidamos nosotros
	iNumeroError = pthread_detach(idThread);
	//Chequeo Errores en el detach
	if (iNumeroError != 0) {
		perror("Ocurrio un Error al hacer Detach. El Error es");
		return 0;
	}

	Servidor_loguear(LOG_LEVEL_INFO, __func__, ">>Servidor Listo");

	//Espero 1 Segundo a que se Inicie Correctamente El Servidor
	sleep(1);

	Servidor_loguear(LOG_LEVEL_INFO, __func__, ">>El FS esta Listo para Trabajar, Iniciando Consola");

	printf("Iniciando Consola...\n\n");

	//Abro la Consola, solo sale del While cuando se ejecuta el comando "Salir"
	while (leerConsola()) {
		//No hace Falta Hacer nada
	}


	//Solo llega aca abajo cuando se Cierra el File System

	return 1;
}






int leerConsola() {

	//Coloreado Para que sirva de Separador Entre comandos, asi es mas facil de distinguir Visualmente
	printf(ANSI_COLOR_YELLOW ">Ingrese un comando (ingrese \"Ayuda\" para una lista de comandos): " ANSI_COLOR_RESET);

	int estaReconocidoComando = -1; //Los valores que va a tener son 1/0/-1 Siendo 1 y 0 Que el comando SI esta reconocido y -1 Que no lo esta. Por defecto se toma que no esta reconocido el comando

	char lineaLeida[MAXIMA_CANTIDAD_CARACTERES_POR_LEER];
	char *input[10];

	lineaLeida[0] = '\0'; /* se asegura que si hacen input vacio no lea cualquier cosa */
	lineaLeida[sizeof(lineaLeida) - 1] = ~'\0'; /* se asegura que el ultimo caracter marque que es un string (le pongo caracter de Fin de Cadena) */
	//que es ese caracter "~" que aparece Ahi?

#pragma GCC diagnostic ignored "-Wunused-result"
	//Lee una Linea de la Consola (Terminal de Linux)
	fgets(lineaLeida, sizeof(lineaLeida), stdin);
#pragma GCC diagnostic pop

	int caracterActual = 0; /*read counter*/
	int cantidadArgumentos = 0; /*args counter*/
	int k = 0; /*write counter*/
	bool estoyDentroArgumento = false; //

	//Cada Argumento debe tener un tamaño igual a toda la cantidad de caracteres por leer, ya que puede ser que haya 1 solo argumento gigante
	input[cantidadArgumentos] = malloc(sizeof(char) * MAXIMA_CANTIDAD_CARACTERES_POR_LEER); /*Pido memoria para guardar la instruccion por separado de los argumentos*/
	char* currentPointer = input[cantidadArgumentos];

	for (caracterActual = 0; caracterActual < MAXIMA_CANTIDAD_CARACTERES_POR_LEER; caracterActual++) {
		if (lineaLeida[caracterActual] == '\n') {
			/*cierro el currentPointer porque el usuario ya ingreso enter, despues del \n es dato basura, por eso hago break*/
			currentPointer[k] = '\0';
			break;
		}

		/*Busco comillas, si tenia un argumento abierto, cierro el argumento, si estaba cerrado, cambio j,
		 abriendo un nuevo argumento, y reseteo k para buffear el nuevo argumento*/
		if (lineaLeida[caracterActual] == '\"') {
			if (!estoyDentroArgumento) {
				currentPointer[k] = '\0';
				cantidadArgumentos++;
				input[cantidadArgumentos] = malloc(sizeof(char) * MAXIMA_CANTIDAD_CARACTERES_POR_LEER);
				currentPointer = input[cantidadArgumentos];
				estoyDentroArgumento = true;
				k = 0;
			} else {
				estoyDentroArgumento = false;
				currentPointer[k] = '\0';
				k++;
			}
		} else {
			/*Cuando el char es un espacio, si estoy adentro de un argumento, no lo ignoro, si estoy fuera, lo ignoro
			 * (ejemplo espacio entre argumentos, ignoro, espacio adentro de una ruta, no lo ignoro)*/
			if (lineaLeida[caracterActual] == ' ') {
				if (estoyDentroArgumento) {
					currentPointer[k] = lineaLeida[caracterActual];
					k++;
				}
			} else {
				currentPointer[k] = lineaLeida[caracterActual];
				k++;
			}
		}
	};

	if (string_equals_ignore_case(input[0], "Ayuda")) {
		if( cantidadArgumentos >= 1 ){
			mostarAyuda( input[1] );		//Tweak Piola

		}else{
			mostarAyuda( "MostrarTodos" );
		}
		estaReconocidoComando = 1;
	}

	if (string_equals_ignore_case(input[0], "NuevaCarpeta")  || string_equals_ignore_case(input[0], "NC")) {
		estaReconocidoComando = 1;
		if (cantidadArgumentos < 2) {
			printf(ANSI_COLOR_RED "Por favor, ingrese un nombre de directorio y una ruta para crear el directorio\n");
			printf(ANSI_COLOR_GREEN "Ingrese \"Ayuda\" para una lista de comandos\n" ANSI_COLOR_RESET);
		} else {
			console_crearCarpeta(input[1], input[2]);
		}
	}

	if (string_equals_ignore_case(input[0], "MoverCarpeta")  || string_equals_ignore_case(input[0], "MC")) {
		estaReconocidoComando = 1;
		if (cantidadArgumentos < 3) {
			printf(
					ANSI_COLOR_RED "Por favor, ingrese el nombre del directorio, la carpeta de origen y la carpeta de destino\n");
			printf(ANSI_COLOR_GREEN "Ingrese \"Ayuda\" para una lista de comandos\n" ANSI_COLOR_RESET);
		} else {
			console_moverCarpeta(input[1], input[2], input[3]);
		}
	}

	if (string_equals_ignore_case(input[0], "BorrarCarpeta")  || string_equals_ignore_case(input[0], "BC")) {
		estaReconocidoComando = 1;
		if (cantidadArgumentos < 2) {
			printf(
			ANSI_COLOR_RED "Por favor, ingrese el nombre del directorio,y la ruta de origen\n");
			printf(ANSI_COLOR_GREEN "Ingrese \"Ayuda\" para una lista de comandos\n" ANSI_COLOR_RESET);
		} else {
			console_borrarCarpeta(input[1], input[2]);
		}
	}

	if (string_equals_ignore_case(input[0], "RenombrarCarpeta")  || string_equals_ignore_case(input[0], "RC")) {
		//Pongo que el Comando esta siendo Reconocido
		estaReconocidoComando = 1;

		//Chequeo que tenga 3 argumentos: la ruta en el interior de filesystem, el nombre actual y el nuevo nombre.
		if (cantidadArgumentos < 3) {
			printf(
					ANSI_COLOR_RED "Por favor, ingrese la ruta interna del File System, el nombre actual y el nuevo nombre.\n" ANSI_COLOR_RESET);
			printf(ANSI_COLOR_GREEN "Ingrese \"Ayuda\" para una lista de comandos\n" ANSI_COLOR_RESET);
		} else {
			//Si esta bien llamo a esta funcion.
			console_renombrarCarpeta(input[1], input[2], input[3]);
		}
	}

	if (string_equals_ignore_case(input[0], "FormatearFS")) {
		//Pongo que el Comando esta siendo Reconocido
		estaReconocidoComando = 1;

		//Llamo a la funcion para Formatear el File System
		console_formatearFS();
	}

	if (string_equals_ignore_case(input[0], "MostrarMD5")  || string_equals_ignore_case(input[0], "MM")) {
		estaReconocidoComando = 1;
		if (cantidadArgumentos < 2) {
			printf(
			ANSI_COLOR_RED "Por favor, ingrese el nombre del archivo,y la carpeta de origen\n");
			printf(ANSI_COLOR_GREEN "Ingrese \"Ayuda\" para una lista de comandos\n" ANSI_COLOR_RESET);
		} else {
			console_requestMd5(input[1], input[2]);
		}
	}

	if (string_equals_ignore_case(input[0], "MostrarBloquesDeArchivo")  || string_equals_ignore_case(input[0], "MBDA")) {
		estaReconocidoComando = 1;
		if (cantidadArgumentos < 2) {
			printf(
			ANSI_COLOR_RED "Por favor, ingrese el nombre del archivo, y la ruta donde se encuentra\n");
			printf(ANSI_COLOR_GREEN "Ingrese \"Ayuda\" para una lista de comandos\n" ANSI_COLOR_RESET);
		} else {
			console_mostrarBloquesDeArchivo(input[1], input[2]);
		}
	}

	if (string_equals_ignore_case(input[0], "BorrarBloqueDeArchivo")  || string_equals_ignore_case(input[0], "BBDA")) {
		estaReconocidoComando = 1;
		if (cantidadArgumentos < 3) {
			printf(
					ANSI_COLOR_RED "Por favor, ingrese el nombre del archivo, la ruta del archivo y el numero de archivo, este ultimo entre \"s\n");
			printf(ANSI_COLOR_GREEN "Ingrese \"Ayuda\" para una lista de comandos\n" ANSI_COLOR_RESET);
		} else {
			console_borrarBloqueDeArchivo(input[1], input[2], input[3]);
		}
	}

	if (string_equals_ignore_case(input[0], "MoverArchivo")  || string_equals_ignore_case(input[0], "MA")) {
		estaReconocidoComando = 1;
		if (cantidadArgumentos < 3) {
			printf(
			ANSI_COLOR_RED "Por favor, ingrese el nombre del archivo, la ruta de origen y la ruta de destino\n");
			printf(ANSI_COLOR_GREEN "Ingrese \"Ayuda\" para una lista de comandos\n" ANSI_COLOR_RESET);
		} else {
			console_moverArchivo(input[1], input[2], input[3]);
		}
	}

	if (string_equals_ignore_case(input[0], "BorrarArchivo")  || string_equals_ignore_case(input[0], "BA")) {
		estaReconocidoComando = 1;
		if (cantidadArgumentos < 2) {
			printf(ANSI_COLOR_RED "Por favor, ingrese el nombre del archivo y la ruta de origen\n");
			printf(ANSI_COLOR_GREEN "Ingrese \"Ayuda\" para una lista de comandos\n" ANSI_COLOR_RESET);
		} else {
			console_borrarArchivo(input[1], input[2]);
		}
	}

	//Comando para Cargar un Archivo al File System
	if (string_equals_ignore_case(input[0], "CargarArchivo")  || string_equals_ignore_case(input[0], "CA")) {
		//Pongo que el Comando esta siendo Reconocido
		estaReconocidoComando = 1;

		//Chequeo que tenga 2 argumentos: el comando (la ruta y nombre al archivo dentro de linux) y la ruta en el interior de filesystem
		if (cantidadArgumentos < 2) {
			printf(
			ANSI_COLOR_RED "Por favor, ingrese la ruta del archivo externa, y la ruta interna.\n" ANSI_COLOR_RESET);
			printf(ANSI_COLOR_GREEN "Ingrese \"Ayuda\" para una lista de comandos\n" ANSI_COLOR_RESET);
		} else {
			//Si esta bien llamo a esta funcion.
			console_cargarArchivo(input[1], input[2]);
		}
	}

	//Comando para Descargar desde el FileSystem un Archivo hacia Linux
	if (string_equals_ignore_case(input[0], "DescargarArchivo")  || string_equals_ignore_case(input[0], "DA")) {
		//Pongo que el Comando esta siendo Reconocido
		estaReconocidoComando = 1;

		//Chequeo que tenga 2 argumentos: la ruta en el interior de filesystem y la ruta externa (la ruta y nombre al archivo dentro de linux).
		if (cantidadArgumentos < 2) {
			printf(
					ANSI_COLOR_RED "Por favor, ingrese la ruta externa del archivo , y la ruta interna del File System.\n" ANSI_COLOR_RESET);
			printf(ANSI_COLOR_GREEN "Ingrese \"Ayuda\" para una lista de comandos\n" ANSI_COLOR_RESET);
		} else {
			//Si esta bien llamo a esta funcion.
			console_descargarArchivo(input[1], input[2]);
		}
	}

	//Comando para Renombrar un Archivo dentro del FileSystem.
	if (string_equals_ignore_case(input[0], "RenombrarArchivo")  || string_equals_ignore_case(input[0], "RA")) {
		//Pongo que el Comando esta siendo Reconocido
		estaReconocidoComando = 1;

		//Chequeo que tenga 3 argumentos: la ruta en el interior de filesystem, el nombre actual y el nuevo nombre.
		if (cantidadArgumentos < 3) {
			printf(
					ANSI_COLOR_RED "Por favor, ingrese la ruta interna del File System, el nombre actual y el nuevo nombre.\n" ANSI_COLOR_RESET);
			printf(ANSI_COLOR_GREEN "Ingrese \"Ayuda\" para una lista de comandos\n" ANSI_COLOR_RESET);
		} else {
			//Si esta bien llamo a esta funcion.
			console_renombrarArchivo(input[1], input[2], input[3]);
		}
	}

	if (string_equals_ignore_case(input[0], "AgregarNodo")  || string_equals_ignore_case(input[0], "AN")) {
		estaReconocidoComando = 1;
		if (cantidadArgumentos < 1) {
			printf( ANSI_COLOR_RED "Por favor, ingrese el nombre del nodo a agregar\n");
			printf(ANSI_COLOR_GREEN "Ingrese \"Ayuda\" para una lista de comandos\n" ANSI_COLOR_RESET);
		} else {
			console_agregarNodo(input[1]);
		}
	}

	if (string_equals_ignore_case(input[0], "ListarCarpeta")  || string_equals_ignore_case(input[0], "LS")  || string_equals_ignore_case(input[0], "LC")) {
		estaReconocidoComando = 1;
		if (cantidadArgumentos < 1) {
			console_listarContenido("/");	//Vemos a Root
		} else {
			console_listarContenido(input[1]);
		}
	}
	
	if (string_equals_ignore_case(input[0], "ListarNodos")  || string_equals_ignore_case(input[0], "LN")) {
		estaReconocidoComando = 1;
		console_listarNodos();	//Vemos los Nodos
	}

	if (string_equals_ignore_case(input[0], "CrearRuta")  || string_equals_ignore_case(input[0], "CR")) {
		estaReconocidoComando = 1;
		if (cantidadArgumentos < 1) {
			printf(ANSI_COLOR_RED "Por favor, ingrese la ruta a crear\n");
			printf(ANSI_COLOR_GREEN "Ingrese \"Ayuda\" para una lista de comandos\n" ANSI_COLOR_RESET);
		} else {
			console_crearRuta(input[1]);
		}
	}

	if (string_equals_ignore_case(input[0], "EliminarNodo")  || string_equals_ignore_case(input[0], "EN")) {
		estaReconocidoComando = 1;
		if (cantidadArgumentos < 1) {
			printf( ANSI_COLOR_RED "Por favor, ingrese el nombre del nodo a eliminar\n");
			printf(ANSI_COLOR_GREEN "Ingrese \"Ayuda\" para una lista de comandos\n" ANSI_COLOR_RESET);
		} else {
			console_eliminarNodo(input[1]);
		}
	}

	if (string_equals_ignore_case(input[0], "VerBloque")  || string_equals_ignore_case(input[0], "VB")) {
		estaReconocidoComando = 1;
		if (cantidadArgumentos < 3) {
			printf(
			ANSI_COLOR_RED "Por favor, ingrese el nombre del Nodo, el Numero de Bloque y el Nombre del Archivo\n");
			printf(ANSI_COLOR_GREEN "Ingrese \"Ayuda\" para una lista de comandos\n" ANSI_COLOR_RESET);
		} else {
			console_verBloqueNodo(input[1], input[2], input[3]);
		}
	}

	if (string_equals_ignore_case(input[0], "CopiarBloque")  || string_equals_ignore_case(input[0], "CB")) {
		estaReconocidoComando = 1;
		if (cantidadArgumentos < 4) {
			printf(
					ANSI_COLOR_RED "Por favor, ingrese el nombre del Nodo Origen, el Numero de Bloque, el Nodo Destino donde copiar el bloque y en cual Numero Copiarlo\n");
			printf(ANSI_COLOR_GREEN "Ingrese \"Ayuda\" para una lista de comandos\n" ANSI_COLOR_RESET);
		} else {
			console_copiarBloqueANodo(input[1], input[2], input[3], input[4]);
		}
	}

	if (string_equals_ignore_case(input[0], "BorrarBloque")  || string_equals_ignore_case(input[0], "BB")) {
		estaReconocidoComando = 1;
		if (cantidadArgumentos < 2) {
			printf( ANSI_COLOR_RED "Por favor, ingrese el nombre del Nodo y el Numero de Bloque\n");
			printf(ANSI_COLOR_GREEN "Ingrese \"Ayuda\" para una lista de comandos\n" ANSI_COLOR_RESET);
		} else {
			console_borrarBloqueNodo(input[1], input[2]);
		}
	}

	if (string_equals_ignore_case(input[0], "Salir")) {
		//Llamo a Salir
		Macro_ImprimirParaDebug("Saliendo del FS por Comando 'Salir' \n");
		salir_Cerrar_FS();
		estaReconocidoComando = 0;
	}

	//Pequeña Mejora para que los "Enter" No los Tome como Error, sino que haga varios \n para Separar. Muy Util, es como un "Clear" o "Ctrl + L"
	if (string_is_empty(input[0])) {
		estaReconocidoComando = 1;
		printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
	}

	/*Libero la memoria del comando y los argumentos*/
	for (caracterActual = 0; caracterActual <= cantidadArgumentos; caracterActual++) {
		free(input[caracterActual]);
	};

	/*Si no se reconocio el comando, se envia error*/
	if (estaReconocidoComando == -1) {
		printf(ANSI_COLOR_RED"El comando ingresado no es conocido\n" ANSI_COLOR_RESET);
		Macro_ImprimirParaDebug("Comando Ingresado: %s", lineaLeida);
		printf(ANSI_COLOR_GREEN "Ingrese \"Ayuda\" para una lista de comandos\n" ANSI_COLOR_RESET);
	}

	return estaReconocidoComando;
}

void salir_Cerrar_FS(){
	//Por Seguridad, asi no se hace garcha la consola de Linux
	printf(ANSI_COLOR_RESET "\n");

	Macro_ImprimirEstadoInicio("- Cerrando la Base de Datos");
	//Cierro la Base de Datos y Despues se Cierra Todo lo Demas al Terminarse el Main.

	//Cierro todas las tablas y los indices
	DIR_CerrarIndices();
	FILE_CerrarIndices();
	NODO_CerrarIndices();
	BLOQUES_CerrarIndices();
	//Sincronizo y Cierro todas las Tablas
	DB_synccloseTablas("", 1);
	DB_CerrarEntorno();

	Macro_ImprimirEstadoFinal(EJECUCION_CORRECTA);

	//Como al Volver de Esta Rutina se Cierra la Consola, Hago el printf de cerrando aca
	Macro_ImprimirEstadoInicio("- Cerrando la Consola");
	Macro_ImprimirEstadoFinal(EJECUCION_CORRECTA);

	//NOTA: El Servidor Se Cierra Ahora, por eso pongo el cerrando aca
	Macro_ImprimirEstadoInicio("- Cerrando el Servidor del FS");
	Macro_ImprimirEstadoFinal(EJECUCION_CORRECTA);

	Comun_Pantalla_Separador_Destacar("CERRADO EL FILE SYSTEM");
	return;
}

void FS_LiberarMemoriaDobleArray(void **punteroMemoria, int sizeArray) {
	int posActual = 0;
	for (posActual = 0; posActual < sizeArray; posActual++) {
			if (isvarset(punteroMemoria[posActual]) == 1) {
				free(punteroMemoria[posActual]);
				punteroMemoria[posActual] = NULL;
			}
	}
	free(punteroMemoria);
	punteroMemoria = NULL;
	return;
}
