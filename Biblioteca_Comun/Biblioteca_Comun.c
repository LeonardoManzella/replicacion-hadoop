#include "Biblioteca_Comun.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h> 	//Para Mutex
#include <time.h> 	//Para Mutex

#include <commons/string.h>

#include "../FileSystem/headers/Biblioteca_Bloques.h"			//Para Calcular Tamanio

	#include <sys/types.h> //Para Portabilidad
	#include <fcntl.h> // Para open
	#include "../lib/include/md5/md5.h" // para crear md5
	#include <sys/ioctl.h> // Para tamanio de consola

//Define Interno, no les debe Importar a ustedes
#define NOMBRE_ARCHIVO_PRUEBA "PruebaTemporalParaPermisos.txt"


//Inicializo un Mutex para el Access. Le tuve que poner Mutex porque nadie me asegura que Access sea Thread Safe
static pthread_mutex_t Mutex_Interno_Access  = PTHREAD_MUTEX_INITIALIZER;


int Comun_existeArchivo(const char *srutaLinuxArchivo) {
	//Veo si el archivo existe el archivo y si se tienen permisos de Lectura, Escritura y Ejecucion.
	int numeroError = 0;
	int numeroErrorAccess = 0;

	//Le tuve que poner Mutex porque nadie me asegura que Access sea Thread Safe
	numeroError = pthread_mutex_lock(&Mutex_Interno_Access);
	if (numeroError != 0) {
		Macro_Imprimir_Error("Problema al Pedir Lock Mutex. Numero Error:%d" , numeroError);
		return -1;
	}

	numeroErrorAccess = access(srutaLinuxArchivo , F_OK);

	numeroError = pthread_mutex_unlock(&Mutex_Interno_Access);
	if (numeroError != 0) {
		Macro_Imprimir_Error("Problema al Hacer UnLock Mutex. Numero Error:%d" , numeroError);
		return -1;
	}


	if ( numeroErrorAccess == 0) {
		//Se Abrio Correctamente, devuelvo 0.
		return 0;
	} else {
		//Hubo algun problema al ver si existe el archivo, devuelvo -1 y aviso por consola.
		Macro_ImprimirParaDebug("El archivo o No existe o No se tienen permisos de Lectura/Escritura/Ejecucion.\n");
		Macro_ImprimirParaDebug("La Ruta del archivo es: %s.\n", srutaLinuxArchivo);
		Macro_ImprimirParaDebug("Error Detectado:" ANSI_COLOR_RED "%s" ANSI_COLOR_RESET "\n", Macro_Obtener_Errno());
		return -1;
	}
}

void Comun_ImprimirProgreso(char* Mensaje, size_t TamanioTotal, size_t estado) {
	struct winsize ws;
	static uint32_t progresoActual;
	int tamMensaje = strlen(Mensaje) + 2;
	// Para obtener el ancho de la consola
	static int cantBarras;

	ioctl(0 , TIOCGWINSZ , &ws);

	//ws.ws_col

	int tamanioBloqueTexto = 100 / (ws.ws_col - tamMensaje) + (((100 % (ws.ws_col - tamMensaje)) != 0) ? 1 : 0);
	int tamanioBloqueArch;
	if (TamanioTotal > (ws.ws_col - tamMensaje)) {
		tamanioBloqueArch = TamanioTotal / (ws.ws_col - tamMensaje);
	}

	if (estado == 0) {
		printf("%s [", Mensaje);
		printf(PANTALLA_POSICIONARSE_TABULADO("%d")"]\r" , (100 / tamanioBloqueTexto) - 1);
		//printf(PANTALLA_POSICIONARSE_TABULADO("%d")"]\r" , (100 / EscalaPorcentual) - 1);

		printf(PANTALLA_POSICIONARSE_TABULADO("%d") , tamMensaje);
		progresoActual = 0;
		cantBarras = 0;
	}

	if ((estado > 0) && (cantBarras < 100/tamanioBloqueTexto)) {
		if (TamanioTotal <= (ws.ws_col - tamMensaje)) {
			for (cantBarras = 1; cantBarras < (100/tamanioBloqueTexto); cantBarras++){
				printf("\x1b[44m "ANSI_COLOR_RESET);
			}
			fflush(stdout);
			printf("\n");
			// Imprimo toda la barra
		} else {
			if (estado > tamanioBloqueArch) {
				// Imprimo varias barras
				int parteArchivo = 0;
				while (cantBarras < (100/tamanioBloqueTexto)) {
					parteArchivo += tamanioBloqueArch;
					if (parteArchivo >= estado) {
						progresoActual += (parteArchivo - estado);
						break;
					}
					cantBarras++;
					if (cantBarras >= 100/tamanioBloqueTexto)
						printf(ANSI_COLOR_RESET"\n");
					else
						printf("\x1b[44m "ANSI_COLOR_RESET);
					fflush(stdout);
					// verifico si llegue a completar el tamanioBloqueArch
				}
			} else if ((progresoActual+estado) >= tamanioBloqueArch){
				// Imprimo una barra
				cantBarras++;
				// si es la ultima barra imprimo un \n
				if (cantBarras >= 100/tamanioBloqueTexto) {
					printf(ANSI_COLOR_RESET"\n");
				} else {
					printf("\x1b[44m "ANSI_COLOR_RESET);
				}
				fflush(stdout);
				progresoActual -= tamanioBloqueArch;
				progresoActual += estado;
			} else {
				progresoActual += estado;
			}
		}
	}

	//uint32_t tamanioBarraProg = 0;
//	uint32_t tamanioBarraProg = (EscalaPorcentual * TamanioTotal) / 100;
/*	if (TamanioTotal > TamanioProgreso) {
		tamanioBarraProg = (TamanioTotal / 100) * EscalaPorcentual;
	} else {
		// tengo un solo bloque de ese tamanio
		tamanioBarraProg = 100 / EscalaPorcentual;
	}
	*/
/*	if (estado == 0) {
		printf("%s [", Mensaje);
		printf(PANTALLA_POSICIONARSE_TABULADO("%d")"]\r" , (100 / EscalaPorcentual) - 1);

		printf(PANTALLA_POSICIONARSE_TABULADO("%d") , tamMensaje);
		progresoActual = 0;
		cantBarras = 0;
	}

	if (estado > 0) {
		progresoActual += estado;
		if (progresoActual == TamanioTotal) {
			char *sBarra;
			sBarra = malloc(sizeof(char) * ((100 / EscalaPorcentual) - 1));
			memset(sBarra , ' ' , (100 / EscalaPorcentual) - 1);
			printf("\x1b[44m%s"ANSI_COLOR_RESET , sBarra);
			free(sBarra);
			printf("\n");
		}

//		if (tamanioBarraProg == (100 / EscalaPorcentual)) {
		} else if (progresoActual >= tamanioBarraProg) {
			if (tamanioBarraProg >= cantBarras) {
				printf("\x1b[44m "ANSI_COLOR_RESET);
				if (tamanioBarraProg == cantBarras)
					printf("\n");
				fflush(stdout);
				progresoActual = 0;
				cantBarras++;
			}
		}
	}
*/
}

void Comun_ImprimirMensajeConBarras(char* Mensaje) {
	struct winsize ws;
	int tamMensaje = strlen(Mensaje) + 2;
	// Para obtener el ancho de la consola
	ioctl(0 , TIOCGWINSZ , &ws);
	//ws.ws_col
	int CentroPantalla = (ws.ws_col) / 2;
	if (tamMensaje > ws.ws_col) {
		printf("%s", Mensaje);
		return;
	}
	int InicioMensaje = CentroPantalla - (tamMensaje / 2);
	int PosActual, PosFinal;

	printf("\r");
	PosFinal = InicioMensaje;
	for (PosActual = 0; PosActual <= PosFinal; PosActual++) {
		printf("\x1b[46m "ANSI_COLOR_RESET);
	}
	printf(" %s ", Mensaje);
	PosFinal = ws.ws_col;
	for (PosActual += tamMensaje; PosActual < PosFinal; PosActual++) {
		printf("\x1b[46m "ANSI_COLOR_RESET);
	}
	printf("\n");
}

//Funcion para Obtener el MD5 dado un archivo
char* Comun_obtener_MD5(const char* rutaLinux, bool showProgress) {
	int 		inFile = -1;
	uint32_t 	numeroError = 0;
	unsigned char md5calculado[MD5_DIGEST_LENGTH];
	uint32_t PageSize = sysconf(_SC_PAGESIZE);		//Obtengo el tamaño (en bytes) de las Paginas del Sistema
	unsigned char data[PageSize];

	//Para evitar Problemas Seteo el Errno en 0 al empezar
	errno = 0;

	//Creo un string para guardar el MD5 (32 caracteres + 1 de fin de string), inicializo al caracter de fin de linea en la primera posicion
	static char md5[33];
	md5[0] = '\0';



	inFile = open(rutaLinux , O_RDONLY , O_NONBLOCK);
	Macro_Check_And_Handle_Error(inFile == -1 , "Hubo un Problema al Abrir el  Archivo:'%s'", rutaLinux);


	MD5_CTX mdContext;
	int bytes;


	uint32_t tamanioArchivo = Bloques_obtener_tamanio_archivo(rutaLinux);

	if (showProgress == true) {
		Comun_ImprimirProgreso("Calculando MD5 Archivo", tamanioArchivo, 0);
	} else {
		printf("Calculando MD5");
	}
	numeroError = MD5_Init(&mdContext);
	Macro_Check_And_Handle_Error((numeroError == 0) || (errno!=0) , "Problema al Hacer MD5_Init");		//La documentacion dice que con 0 significa error y setea el errno. No me fio de que devuelvan bien el 0, asi que incluyo que si el errno cambia pare todo el MD5.
	//Quedense tranquilos que el Errno es Thread Safe y Fork Safe.


	while ((bytes = read(inFile , data , PageSize)) != 0) {
		Macro_Check_And_Handle_Error(bytes == -1 , "Problema al Hacer Read");

		numeroError = MD5_Update(&mdContext , data , bytes);
		Macro_Check_And_Handle_Error((numeroError == 0) || (errno!=0) , "Problema al Hacer MD5_Update");
		if (showProgress == true)
			Comun_ImprimirProgreso("Calculando MD5", tamanioArchivo, bytes);
	}

	numeroError = MD5_Final(md5calculado , &mdContext);
	Macro_Check_And_Handle_Error((numeroError == 0) || (errno!=0) , "Problema al Hacer MD5_Final");


	int iPosStringMD5;
	for (iPosStringMD5 = 0; iPosStringMD5 < MD5_DIGEST_LENGTH; iPosStringMD5++) {
		sprintf(&md5[iPosStringMD5 * 2] , "%02x" , md5calculado[iPosStringMD5]);
	}

	if (inFile != -1) {
		numeroError = close(inFile);
		inFile = -1;
		Macro_Check_And_Handle_Error(numeroError ==-1 , "Problema al Hacer Close del Archivo:'%s'", rutaLinux);
	}


	// Comentado por Julian 2015-07-01 y reemplazado por rutina que lo hace internamente
/*	//Tengo que concatenar la ruta del archivo con el comando para poder llamar a "popen".
	char* comandoPorEjecutar = string_new();
	string_append(&comandoPorEjecutar, "md5sum ");
	string_append(&comandoPorEjecutar, (char*) rutaLinux);

	//Ejecuto el Comando de linux para obtener MD5, uso un Pipe Stream a un Nuevo Proceso (que ejecuta el comando y redirecciona el resultado) en modo Lectura, luego lo cierro.
	FILE *pipeStream = popen((const char*) comandoPorEjecutar, "r");

	//Chequeo que se halla ejecutado bien el comando
	if (pipeStream == NULL) {
		Macro_ImprimirParaDebug("No se pudo crear el Pipe Stream que ejecuta el comando \"md5sum\"para obtener el MD5.\n");
		return NULL;
	} else {
		//Obtengo del pipeStream el MD5 generado (primeros 32 caracteres).

		//Leo el MD5 (primeros 32 caracteres)
#pragma GCC diagnostic ignored "-Wunused-result"
		fgets(md5, 33, pipeStream);
#pragma GCC diagnostic pop
		//Cierro el pipeStream y chequeo errores
		iNumeroError = pclose(pipeStream);
		if (iNumeroError == -1) {
			Macro_ImprimirParaDebug("No se pudo cerrar el Pipe Stream que ejecuta el comando \"md5sum\"para obtener el MD5.\n");
			return NULL;
		}
	}
*/
	//Chequeo que se halla obtenido un MD5 valido.Que no sea el valor inicializado,  ni este vacio, ni tenga un numero distinto de 32 caracteres.
	if (strcmp(md5, "\0") == 0 || strcmp(md5, "") == 0
			|| string_length(md5) != 32) {
		Macro_ImprimirParaDebug("Hubo un problema al obtener el MD5.\n");
		Macro_ImprimirParaDebug("El MD5 Leido es: %s \n", md5);
		return NULL;
	}

	//Si llego hasta aqui, es que se ejecuto bien, entonces devuelvo el MD5.
	return md5;

Error_Handler:
	Macro_Imprimir_Error("Problema al Calcular el MD5");

	if (inFile != -1) {
		numeroError = close(inFile);
		if(numeroError ==-1){
			Macro_Imprimir_Error("Problema al Hacer Close del Archivo:'%s'", rutaLinux);
		}
	}

	return NULL;
}

//Funcion para Obtener el MD5 dado un archivo
char* Comun_obtener_MD5_Bloque(const tipo_bloque* punteroBloque, bool showProgress) {
	uint32_t 	numeroError = 0;
	unsigned char md5calculado[MD5_DIGEST_LENGTH];
	//uint32_t PageSize = sysconf(_SC_PAGESIZE);		//Obtengo el tamaño (en bytes) de las Paginas del Sistema
	//unsigned char data[PageSize];

	//Para evitar Problemas Seteo el Errno en 0 al empezar
	errno = 0;

	//Creo un string para guardar el MD5 (32 caracteres + 1 de fin de string), inicializo al caracter de fin de linea en la primera posicion
	static char md5[33];
	md5[0] = '\0';

	// El menos 1 es por el caracter final que se agrega

	MD5_CTX mdContext;

	if (showProgress == true) {
		Comun_ImprimirProgreso("Calculando MD5 Bloque", punteroBloque->tamanioBloque, 0);
	} else {
		printf("Calculando MD5");
	}

	numeroError = MD5_Init(&mdContext);
	Macro_Check_And_Handle_Error((numeroError == 0) || (errno!=0) , "Problema al Hacer MD5_Init");		//La documentacion dice que con 0 significa error y setea el errno. No me fio de que devuelvan bien el 0, asi que incluyo que si el errno cambia pare todo el MD5.
	//Quedense tranquilos que el Errno es Thread Safe y Fork Safe.

	numeroError = MD5_Update(&mdContext , punteroBloque->contenidoBloque , punteroBloque->tamanioBloque);
	Macro_Check_And_Handle_Error((numeroError == 0) || (errno!=0) , "Problema al Hacer MD5_Update");
	if (showProgress == true) {
		Comun_ImprimirProgreso("Calculando MD5", punteroBloque->tamanioBloque, punteroBloque->tamanioBloque);
	}

	numeroError = MD5_Final(md5calculado , &mdContext);
	//Macro_Check_And_Handle_Error((numeroError == 0) || (errno!=0) , "Problema al Hacer MD5_Final");


	int iPosStringMD5;
	for (iPosStringMD5 = 0; iPosStringMD5 < MD5_DIGEST_LENGTH; iPosStringMD5++) {
		sprintf(&md5[iPosStringMD5 * 2] , "%02x" , md5calculado[iPosStringMD5]);
	}

	//Chequeo que se halla obtenido un MD5 valido.Que no sea el valor inicializado,  ni este vacio, ni tenga un numero distinto de 32 caracteres.
	if (strcmp(md5, "\0") == 0 || strcmp(md5, "") == 0
			|| string_length(md5) != 32) {
		Macro_ImprimirParaDebug("Hubo un problema al obtener el MD5.\n");
		Macro_ImprimirParaDebug("El MD5 Leido es: %s \n", md5);
		return NULL;
	}

	//Si llego hasta aqui, es que se ejecuto bien, entonces devuelvo el MD5.
	return md5;

Error_Handler:
	Macro_Imprimir_Error("Problema al Calcular el MD5");

	return NULL;
}


int Comun_controlarPermisos() {
	//Variable para Controlar Errores, por defecto es 0 que significa que no hay error
	int iNumeroError = 0;

	//Controlo que pueda crear archivos creando un archivo temporal
	FILE* archivoTemporal = fopen(NOMBRE_ARCHIVO_PRUEBA, "w+");
	if (archivoTemporal == NULL) {
		Macro_ImprimirParaDebug("No hay Permisos para Crear Archivos \n");
		Macro_ImprimirParaDebug("Error Detectado:" ANSI_COLOR_RED "%s" ANSI_COLOR_RESET "\n", Macro_Obtener_Errno())
		return -1;
	}
	//Controlo que se pueda cerrar el archivo
	iNumeroError = fclose(archivoTemporal);
	if (iNumeroError != 0) {
		Macro_ImprimirParaDebug("No hay Permisos para Cerrar Archivos, \n");
		Macro_ImprimirParaDebug("Error Detectado:" ANSI_COLOR_RED "%s" ANSI_COLOR_RESET "\n", Macro_Obtener_Errno())
		return -1;
	}
	//Veo por permisos de Lectura, Escritura y Ejecucion.
	iNumeroError = Comun_existeArchivo(NOMBRE_ARCHIVO_PRUEBA);
	if (iNumeroError != 0) {
		//No hace falta imprimir Nada, se encarga la funcion "Comun_existeArchivo"
		return -1;
	}
	//Veo por permisos para borrar archivos
	iNumeroError = remove(NOMBRE_ARCHIVO_PRUEBA);
	if (iNumeroError != 0) {
		Macro_ImprimirParaDebug("No hay Permisos para Borrar Archivos \n");
		Macro_ImprimirParaDebug("Error Detectado:" ANSI_COLOR_RED "%s" ANSI_COLOR_RESET "\n", Macro_Obtener_Errno())
		return -1;
	}

	//Si llegamos hasta aca, es que tenemos los permisos
	return 0;
}


char* Comun_obtenerRutaDirectorio(const char* rutaCompleta) {

	//Veo si me Pasaron un Char* NULL
	if (rutaCompleta == NULL) {
		Macro_ImprimirParaDebug("ERROR en 'Comun_obtenerRutaDirectorio', le pasastes un char* NULL. Te devuelvo NULL, asi te explota en algun otro lado y para todo \n");
		return NULL;
	}

	//Veo si es Vacio
	if (string_equals_ignore_case((char*) rutaCompleta , "")) {
		return string_duplicate("");
	}

	//Veo si es "/" Solito
	if (string_equals_ignore_case((char*) rutaCompleta , "/")) {
		return string_duplicate("/");
	}

	//Veo si no tiene ningun "/", entonces devuelvo Vacio, ""
	if (strrchr(rutaCompleta , '/') == NULL) {
		return string_duplicate("");
	}

	//Obtengo la longitud desde el inicio hasta el ultimo "/"
	int longitud = strrchr(rutaCompleta , '/') - rutaCompleta;

	//Veo Si el Unico "/" que hay es al principio
	if(longitud==0){
		return string_duplicate("/");
	}

	//Tomo toda la Ruta hasta el ultimo "/" que haya, lo que le sigue al "/" final "se descarta" y no lo devuelve
	char* stringPorDevolver = string_substring_until((char*) rutaCompleta , longitud );
	return stringPorDevolver;
}

void Comun_LiberarMemoria(void** punteroMemoria) {

	//Si no ha sido Utilizada la Memoria, no la Libero
	if (*punteroMemoria != NULL) {
		//Libero la Memoria
		free(*punteroMemoria);
		//Pongo el Puntero a NULL, para que Cualquier Referencia Futura a la Memoria Genere Error. Asi no puede referenciarse a Memoria Basura
		//Esto tambien evita que se libere 2 veces la memoria
		*punteroMemoria = NULL;
	}

	return;
}

void Comun_LiberarMemoriaDobleArray(void ***punteroMemoria, int sizeArray) {

	int posActual = 0;
	for (posActual = 0; posActual < sizeArray; posActual++) {
			free(*punteroMemoria[posActual]);
			*punteroMemoria[posActual] = NULL;
	}
	free(*punteroMemoria);
	*punteroMemoria = NULL;
	return;
}


void Comun_Pantalla_Separador_Destacar(const char* texto, ... ) {
	//Necesito juntar en un Unico String todo los char que se le pasen
	char* textoJuntado;

	//Utilizo una Lista de Argumentos Variables y Una funcion de las Commons para juntar los Strings
	va_list listaArgumentosVariables;
	va_start(listaArgumentosVariables, texto);

	textoJuntado = string_from_vformat(texto, listaArgumentosVariables);

	printf( "\n\n" ANSI_COLOR_BLUE "[========== %s ==========]" ANSI_COLOR_RESET "\n\n", textoJuntado );
	free(textoJuntado);
}
