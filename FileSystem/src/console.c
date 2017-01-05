#include "../headers/console.h"

//#include <db.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>

#include <commons/string.h>

#include "../../Biblioteca_Comun/Biblioteca_Comun.h"	//Para Constantes y Macros
#include "../../Sockets/Biblioteca_Sockets.h"			//Para Poder Enviar y Recibir
#include "../../Serializador/Protocolo_Nodos_FS.h"		//Para la Orden de Copiar Bloque

void mostarAyuda( char* queMostrar) {
	bool mostrarComandosBasicos = false;
	bool mostrarComandosCarpetas = false;
	bool mostrarComandosArchivos = false;
	bool mostrarComandosBloques = false;
	bool mostrarComandosNodos = false;
	bool mostrarComandosExtras = false;

	if( string_equals_ignore_case(queMostrar, "Basicos") ){
		mostrarComandosBasicos = true;

	}else if( string_equals_ignore_case(queMostrar, "Carpetas") || string_equals_ignore_case(queMostrar, "Directorios") ){
		mostrarComandosCarpetas = true;

	}else if( string_equals_ignore_case(queMostrar, "Archivos") ){
		mostrarComandosArchivos = true;

	}else if( string_equals_ignore_case(queMostrar, "Bloques") ){
		mostrarComandosBloques = true;

	}else if( string_equals_ignore_case(queMostrar, "Nodos") ){
		mostrarComandosNodos = true;

	}else if( string_equals_ignore_case(queMostrar, "Extras") ){
		mostrarComandosExtras = true;

	}else{
		//Muestro TODAS las Ayudas
		mostrarComandosBasicos = true;
		mostrarComandosCarpetas = true;
		mostrarComandosArchivos = true;
		mostrarComandosBloques = true;
		mostrarComandosNodos = true;
		mostrarComandosExtras = true;
	}


	//Nota: \t es tabulacion Horizontal (son varios espacios seguidos).

	if( mostrarComandosBasicos==true ){
		printf(ANSI_COLOR_CYAN "Todos los argumentos tienen que estar adentro un par de Comillas.\n"ANSI_COLOR_RESET);
		printf(ANSI_COLOR_CYAN "La carpeta raiz es \"/\", sin nada mas dentro de las Comillas. El caracter para separar las carpetas es \"/\".\n"ANSI_COLOR_RESET);
		printf(ANSI_COLOR_CYAN "Se Distinguen Mayusculas de Minusculas.\n"ANSI_COLOR_RESET);

		//Ayuda para el comando "salir"
		printf(ANSI_COLOR_GREEN "\t-\"salir\"\n");
		printf(ANSI_COLOR_RESET"\t\t El proceso FileSystem detiene su ejecucion permanentemente\n");

		//Ayuda para el comando "formatearFS"
		printf(ANSI_COLOR_GREEN "\t-\"formatearFS\"\n" ANSI_COLOR_RESET);
		printf("\t\t Elimina Todas las Carpetas y Archivos del File System.\n");

		printf("\n\n\n");
	}

	if( mostrarComandosCarpetas == true ){
		//Ayuda para el comando "nuevaCarpeta"
		printf(ANSI_COLOR_GREEN "\t-nuevaCarpeta \"<nombre>\" \"<ruta>\"\n");
		printf(ANSI_COLOR_RESET"\t\t Crea un nuevo directorio en la ruta especificada, si esta no existe o el nombre es nulo, devuelve un error. \n");
		printf("Atajo: " ANSI_COLOR_MAGENTA "NC" ANSI_COLOR_RESET "\n");

		//Ayuda para el comando "moverCarpeta"
		printf(ANSI_COLOR_GREEN "\t-moverCarpeta \"<nombre>\" \"<ruta de origen>\" \"<ruta de destino>\"\n");
		printf(	ANSI_COLOR_RESET"\t\t Mueve la carpeta con el nombre especificado desde la ruta de origen a la ruta de destino. \n");
		printf("Atajo: " ANSI_COLOR_MAGENTA "MC" ANSI_COLOR_RESET "\n");

		//Ayuda para el comando "renombrarCarpeta"
		printf(ANSI_COLOR_GREEN "\t-renombrarCarpeta \"<ruta dentro de filesystem>\" \"<nombre actual>\" \"<nuevo nombre>\"\n" ANSI_COLOR_RESET);
		printf("\t\t Busca la Carpeta dentro del File System en la ruta indicada y le cambia el nombre.\n");
		printf("\t\t Ejemplo de Uso: renombrarCarpeta \"/Carpeta1/Carpeta2\" \"nombreActual\" \"nombreNuevo\"\n");
		printf("Atajo: " ANSI_COLOR_MAGENTA "RC" ANSI_COLOR_RESET "\n");

		//Ayuda para el comando "borrarCarpeta"
		printf(ANSI_COLOR_GREEN "\t-borrarCarpeta \"<nombre>\" \"<ruta de origen>\"\n");
		printf( ANSI_COLOR_RESET "\t\t Borra la carpeta con el nombre especificado en la ruta de origen. \n");
		printf("Atajo: " ANSI_COLOR_MAGENTA "BC" ANSI_COLOR_RESET "\n");

		printf("\n\n\n");
	}

	if( mostrarComandosArchivos == true ){
		//Ayuda para el comando "cargarArchivo"
		printf(ANSI_COLOR_GREEN "\t-cargarArchivo \"<ruta dentro de linux>\" \"<ruta dentro de filesystem>\"\n" ANSI_COLOR_RESET);
		printf("\t\t Carga un Archivo a la Base de Datos, lee los bloques y se los envia al File System para que elija a que nodos enviarselos.\n");
		printf("\t\t Ejemplo de ruta dentro de linux: \"/home/utnso/Documentos/archivo.txt\"\n");
		printf("Atajo: " ANSI_COLOR_MAGENTA "CA" ANSI_COLOR_RESET "\n");

		//Ayuda para el comando "mostrarMd5"
		printf(ANSI_COLOR_GREEN "\t-mostrarMd5 \"<nombre>\" \"<ruta>\"\n");
		printf( ANSI_COLOR_RESET"\t\t Muestra el hash md5 del archivo guardado en la ruta especificada, si este no existe en la ruta o el nombre es invalido, devuelve un error. \n");
		printf("Atajo: " ANSI_COLOR_MAGENTA "MM" ANSI_COLOR_RESET "\n");

		//Ayuda para el comando "moverArchivo"
		printf(ANSI_COLOR_GREEN "\t-moverArchivo \"<nombre>\" \"<ruta de origen>\" \"<ruta de destino>\"\n");
		printf(ANSI_COLOR_RESET"\t\t Mueve el archivo con el nombre especificado desde la ruta de origen a la ruta de destino.\n");
		printf("Atajo: " ANSI_COLOR_MAGENTA "MA" ANSI_COLOR_RESET "\n");

		//Ayuda para el comando "renombrarArchivo"
		printf(ANSI_COLOR_GREEN "\t-renombrarArchivo \"<ruta dentro de filesystem>\" \"<nombre actual>\" \"<nuevo nombre>\"\n" ANSI_COLOR_RESET);
		printf("\t\t Busca el Archivo dentro del File System en la ruta indicada y le cambia el nombre.\n");
		printf("\t\t Ejemplo de Uso: renombrarArchivo \"/Carpeta1/Carpeta2\" \"nombreActual.extension\" \"nombreNuevo.extension\"\n");
		printf("Atajo: " ANSI_COLOR_MAGENTA "RA" ANSI_COLOR_RESET "\n");

		//Ayuda para el comando "descargarArchivo"
		printf(ANSI_COLOR_GREEN "\t-descargarArchivo \"<ruta dentro de linux>\" \"<ruta dentro de filesystem>\"\n" ANSI_COLOR_RESET);
		printf("\t\t Descarga un Archivo del File System y lo guarda en la ruta indicada.\n");
		printf("\t\t Ejemplo de ruta dentro de linux: \"/home/utnso/Documentos/archivo.txt\"\n");
		printf("\t\t Ejemplo de ruta dentro de File System: \"/Carpeta/archivo.txt\"\n");
		printf("Atajo: " ANSI_COLOR_MAGENTA "DA" ANSI_COLOR_RESET "\n");

		//Ayuda para el comando "borrarArchivo"
		printf(ANSI_COLOR_GREEN "\t-borrarArchivo \"<nombre>\" \"<ruta de origen>\"\n");
		printf( ANSI_COLOR_RESET "\t\t Borra el archivo con el nombre especificado en la ruta de origen. \n");
		printf("Atajo: " ANSI_COLOR_MAGENTA "BA" ANSI_COLOR_RESET "\n");

		printf("\n\n\n");
	}

	if( mostrarComandosBloques == true ){
		//Ayuda para el comando "verBloque"
		printf(ANSI_COLOR_GREEN "\t-verBloque \"<Nodo>\" \"<NumeroBloque>\" \"<RutaArchivoPorGenerar>\"\n");
		printf( ANSI_COLOR_RESET"\t\t Guarda en un Archivo Por Crear el Contenido del Bloque del Nodo especifico, si este no existe devuelve un error. \n");
		printf("Atajo: " ANSI_COLOR_MAGENTA "VB" ANSI_COLOR_RESET "\n");

		//Ayuda para el comando "copiarBloque"
		printf( ANSI_COLOR_GREEN "\t-copiarBloque \"<NodoOrigen>\" \"<NumeroBloqueOrigen>\" \"<NodoDestino>\" \"<NumeroBloqueDestino>\"\n");
		printf( ANSI_COLOR_RESET"\t\t Copia un bloque correspondiente de un Nodo especifico a otro nodo marcado, si este no existe, o el Nodo esta caido, devuelve un error. \n");
		printf("Atajo: " ANSI_COLOR_MAGENTA "CB" ANSI_COLOR_RESET "\n");

		//Ayuda para el comando "borrarBloque"
		printf(ANSI_COLOR_GREEN "\t-borrarBloque \"<Nodo>\" \"<NumeroBloque>\" \n");
		printf( ANSI_COLOR_RESET"\t\t Borra el bloque correspondiente de un Nodo especifico, si este no existe devuelve un error. \n");
		printf("Atajo: " ANSI_COLOR_MAGENTA "BB" ANSI_COLOR_RESET "\n");


		printf("\n\n\n");
	}

	if( mostrarComandosNodos == true ){
		//Ayuda para el comando "agregarNodo"
		printf(ANSI_COLOR_GREEN "\t-agregarNodo \"<nodo>\"\n");
		printf( ANSI_COLOR_RESET "\t\t Agrega un nodo a la lista de nodos disponibles.\n");
		printf("Atajo: " ANSI_COLOR_MAGENTA "AN" ANSI_COLOR_RESET "\n");

		//Ayuda para el comando "eliminarNodo"
		printf(ANSI_COLOR_GREEN "\t-eliminarNodo \"<nodo>\"\n");
		printf( ANSI_COLOR_RESET "\t\t Elimina un nodo de la lista de nodos disponibles.\n");
		printf("Atajo: " ANSI_COLOR_MAGENTA "EN" ANSI_COLOR_RESET "\n");

		printf("\n\n\n");
	}

	if( mostrarComandosExtras == true ){
		printf( ANSI_COLOR_CYAN "\t\n\nAhora vienen Comandos Nuestros, No Pedidos para el TP, por lo tanto no se asegura el soporte de estos comandos (aunque andan)\n" ANSI_COLOR_RESET);

		//Ayuda para el comando "mostrarBloquesDeArchivo"
		printf(ANSI_COLOR_GREEN "\t-mostrarBloquesDeArchivo \"<nombre>\" \"<ruta>\"\n");
		printf( ANSI_COLOR_RESET"\t\t Devuelve la lista de los bloques que componen a un archivo y en que nodos estan, si este no existe en la ruta o el nombre es invalido, devuelve un error. \n");
		printf("Atajo: " ANSI_COLOR_MAGENTA "MBDA" ANSI_COLOR_RESET "\n");

		//Ayuda para el comando "borrarBloqueDeArchivo"
		printf(ANSI_COLOR_GREEN "\t-borrarBloqueDeArchivo \"<nombre>\" \"<ruta>\" \"<bloque>\"\n");
		printf(ANSI_COLOR_RESET"\t\t Borra un bloque correspondiente a un archivo especifico, si este no existe, o el archivo no existe en la ruta o el nombre es invalido, devuelve un error. \n");
		printf("Atajo: " ANSI_COLOR_MAGENTA "BBDA" ANSI_COLOR_RESET "\n");

		printf("\n\n\n");

		//Ayuda para el comando "listarCarpeta"
		printf(ANSI_COLOR_GREEN "\t-listarCarpeta \"<ruta>\"\n");
		printf(ANSI_COLOR_RESET "\t\t Lista el contenido de una carpeta.\n");
		printf("Atajo: " ANSI_COLOR_MAGENTA "LC" ANSI_COLOR_RESET "(Tambien LS) \n");

		//Ayuda para el comando "crearRuta"
		printf(ANSI_COLOR_GREEN "\t-crearRuta \"<ruta>\"\n");
		printf(ANSI_COLOR_RESET "\t\t Crea todos los directorios en la ruta, salteando la creacion de los ya existentes. Valida la cantidad de carpetas.\n");
		printf("Atajo: " ANSI_COLOR_MAGENTA "CR" ANSI_COLOR_RESET "\n");
		
		//Ayuda para el comando "listarNodos"
		printf(ANSI_COLOR_GREEN "\t-listarNodos \"<ruta>\"\n");
		printf(ANSI_COLOR_RESET "\t\t Muestra todos los Nodos Conectados al File System en este momento.\n");
		printf("Atajo: " ANSI_COLOR_MAGENTA "LN" ANSI_COLOR_RESET "\n");
	}

	printf("\n");
}


void console_formatearFS() {
	// Antes de formatear tengo que liberar la memoria de bitarray de todos los archivos
	int iNumeroError;
	tipo_id tIdUltimo, id;
	iNumeroError = DB_getUltimoId("file",&tIdUltimo);
	tipo_datos_file datosDelArchivo;
	for(id=1;id < tIdUltimo;id++){
		iNumeroError = FILE_ConsultarId(id, &datosDelArchivo);
		if (iNumeroError != DB_NOTFOUND) {
			if (iNumeroError < 0) {
				printf(ANSI_COLOR_RED "Error al consultar los Datos del Archivo.\n\n" ANSI_COLOR_RESET);
				return;
			}
			if (datosDelArchivo.disponible == true) {
				if (datosDelArchivo.tBloquesActivos != NULL) {
					//FIXME AHORA SI! esto explotaba porque primero se le hacia FREE al Bitarray. Igual por ahora lo dejo en comentario
					//free(datosDelArchivo.tBloquesActivos->bitarray);

					bitarray_destroy(datosDelArchivo.tBloquesActivos);		//Lo pongo aca porque sino podria llegar a hacer free de NULL

					datosDelArchivo.tBloquesActivos = NULL;
				}

			}
		} else {
			continue;
		}
	}

	// Cierro los Indices que no se utilizan para el formateo

	DB_consultarCursor("dirindice", NULL, NULL, 0, 2, NULL, 0, NULL);
	DB_consultarCursor("indpadre", NULL, NULL, 0, 2, NULL, 0, NULL);
	DB_consultarCursor("filemd5", NULL, NULL, 0, 2, NULL, 0, NULL);
	DB_consultarCursor("filepadre", NULL, NULL, 0, 2, NULL, 0, NULL);
    	DB_consultarCursor("bloquesnodo", NULL, NULL, 0,2, NULL, 0, NULL);
    	DB_consultarCursor("bloquesarch", NULL, NULL, 0,2, NULL, 0, NULL);
    	DB_consultarCursor("bloquesespecifico", NULL, NULL, 0,2, NULL, 0, NULL);
	
	//Llamo a la funcion interna de la Base de Datos para Formatear el FS.
	iNumeroError = DB_FormatFS();

	//Abro nuevamente los indices
	//DB_CrearIndice("dir", "dirindice", DIR_idxBuscarNombre);
	DB_consultarCursor("dirindice", NULL, NULL, 0, 1, NULL, 0, NULL);
	//DB_CrearIndice("dir", "indpadre", DIR_idxBuscarIdPadre);
	DB_consultarCursor("indpadre", NULL, NULL, 0, 1, NULL, 0, NULL);
	//DB_CrearIndice("file", "filemd5", FILE_idxBuscarNombre);
	DB_consultarCursor("filemd5", NULL, NULL, 0, 1, NULL, 0, NULL);
	//DB_CrearIndice("file", "filepadre", FILE_idxBuscarIdPadre);
	DB_consultarCursor("filepadre", NULL, NULL, 0, 1, NULL, 0, NULL);
	//DB_CrearIndice("bloques", "bloquesnodo", BLOQUES_idxbuscarBloqueNodo);
	DB_consultarCursor("bloquesnodo", NULL, NULL, 0,1, NULL, 0, NULL);
	//DB_CrearIndice("bloques", "bloquesarch", BLOQUES_idxbuscarBloqueArchivo);
	DB_consultarCursor("bloquesarch", NULL, NULL, 0,1, NULL, 0, NULL);
	//DB_CrearIndice("bloques", "bloquesespecifico", BLOQUES_idxbuscarBloqueDeArchivo);
	DB_consultarCursor("bloquesespecifico", NULL, NULL, 0,1, NULL, 0, NULL);

	//Chequeo errores
	if (iNumeroError < 0) {
		printf(ANSI_COLOR_RED "No se Puso formatear el File System.\n" ANSI_COLOR_RESET);
		printf("Posibles Causas:\n\t- No se Cerraron Bien los Indices\n\t- Se Corrompieron los Indices\n\t- Se Corrompio la Base de Datos \n\n");
		return;
	}

	// Importante! Despues de formatear, seteo los BITARRAY de los NODOS todos a 0 que es  Bloques No Disponible, porque no contienen Nada Ahora
	iNumeroError = DB_getUltimoId("nodo",&tIdUltimo);
	tipo_datos_nodo datosDelNodo;
	if (iNumeroError < 0) {
		printf(ANSI_COLOR_RED "Error al obtener el ultimo Id de Nodo.\n" ANSI_COLOR_RESET);
		printf("Posibles Causas:\n\t- Se Corrompio la Tabla de Nodos\n\t- Se Corrompio la Base de Datos \n\n");
		return;
	}

	for (id = 1; id < tIdUltimo; id++) {
		//Obtengo los Datos del Nodo
		iNumeroError = NODO_Consultar(id , &datosDelNodo);

		//Si no pude obtener los Datos porque no lo encuentra, que siga de largo al proximo nodo (este nodo no se modifica)
		if (iNumeroError == DB_NOTFOUND) {
			continue;
		} else {
			if (iNumeroError < 0) {
				printf(ANSI_COLOR_RED "Error al consultar los Datos del Nodo.\n" ANSI_COLOR_RESET);
				printf("Posibles Causas:\n\t- Se Corrompio la Tabla de Nodos\n\t- Se Corrompio la Base de Datos \n\n");
				return;
			}
			/*
			bitarray_destroy(datosDelNodo.tBloquesNodo);
			// Inicio el BitArray para contar bloques del Nodo
			char *cBloquesNodo;
			int iSizeNodo;
			iSizeNodo = (datosDelNodo.bloques_totales / 8) + 1;
			cBloquesNodo = malloc(sizeof(char) * iSizeNodo);

			//Te estabas pasando de la memoria que pediste con malloc, por eso despues explotaba al pedir el siguiente malloc
			memset(cBloquesNodo , 0 , sizeof(char) * iSizeNodo);
			datosDelNodo.tBloquesNodo = bitarray_create(cBloquesNodo , sizeof(char) * iSizeNodo);
			*/
			int bitActual;
			//size_t bitMaximos = bitarray_get_max_bit(datosDelNodo.tBloquesNodo);

			//Recorro el BitArray Poniendo los Bits en 0, asi Pongo NO Disponibles a Todos sus Nodos
			if (datosDelNodo.tBloquesNodo != NULL) {
				for(bitActual = 0; bitActual < datosDelNodo.bloques_totales; bitActual++){
					bitarray_clean_bit(datosDelNodo.tBloquesNodo, bitActual);
				}
			}

			//Como Ahora no tiene Datos el Nodo, se debe modificar sus Bloques Libres al Maximo y los Usados a 0
			datosDelNodo.bloques_libres = datosDelNodo.bloques_totales;
			datosDelNodo.bloques_usados = 0;
			datosDelNodo.bloques_reservados = 0 ;

			NODO_Modificar(id , datosDelNodo);
			if (iNumeroError < 0) {
				printf(ANSI_COLOR_RED "Error al actualizar los Datos del Nodo.\n" ANSI_COLOR_RESET);
				printf("Posibles Causas:\n\t- Se Corrompio la Tabla de Nodos\n\t- Se Corrompio la Base de Datos \n\n");
				return;
			}
		}
	}

	//Llegados a este punto, se formateo la totalidad del File System, asi que lo imprimo por pantalla.
	printf(
	ANSI_COLOR_GREEN "Se Formateo la Totalidad del File System.\n\n" ANSI_COLOR_RESET);

	return;
}




void console_crearCarpeta(char* directoryName, char* ruta) {
	//Variable para Controlar Errores, en 0 significa no error
	int iNumeroError = 0;

	//Controlo que los Strings no sean vacios
	if (strlen(directoryName) <= 0 || strlen(ruta) <= 0) {
		printf(ANSI_COLOR_RED "Por favor, ingrese un nombre de directorio y una ruta para crear el directorio %d \n",
				strlen(ruta));
		printf(ANSI_COLOR_GREEN "Ingrese \"Ayuda\" para una lista de comandos\n\n" ANSI_COLOR_RESET);
		return;
	}

	//Controlo que no sobrepase la Maxima cantidad de directorios (1024)
	int records = DB_NumeroRegistros("dir");
	if (records == DIR_CANTIDAD_MAXIMA) {
		printf(
				ANSI_COLOR_RED "Se alcanzo la maxima cantidad de directorios posibles, para poder crear debe eliminar otros\n" ANSI_COLOR_RESET);
		return;
	}

	//Controlo que exista la Ruta donde Crear la Carpeta
	int father = DIR_validarRuta(ruta, 0);
	//Chequeo Errores
	if (father == -1) {
		printf(ANSI_COLOR_RED "La ruta ingresada para crear la carpeta no existe\n\n" ANSI_COLOR_RESET);
		return;
	}

	//Aca Valido
	iNumeroError = DIR_validarRuta(directoryName, father);

	//Chequeo Errores
	if (iNumeroError != -1) {
		printf(ANSI_COLOR_RED "La Carpeta YA Existe\n\n" ANSI_COLOR_RESET);
		return;
	}

	tipo_id tIdNuevoDir;
	int created = DIR_CrearConId(directoryName, father, &tIdNuevoDir);

	if (created >= 0) {
		printf(ANSI_COLOR_GREEN "La carpeta fue creada con exito\n\n" ANSI_COLOR_RESET);
	} else if (created == DB_KEYEXIST) {
		printf(ANSI_COLOR_RED "La carpeta ya existe\n\n" ANSI_COLOR_RESET);
	} else {
		printf(ANSI_COLOR_RED "Hubo un problema al crear la carpeta, por favor intentelo mas tarde\n\n" ANSI_COLOR_RESET);
	}
}


void console_moverCarpeta(char* directoryName, char* rutaOrigen, char* rutaDestino) {
	//Variable para Manejar Errores, inicializo en 0 que significa no hay errores.
	int iNumeroError = 0;

	//Verifico que los argumentos de "console_moverCarpeta" sean validos (no esten vacios)
	if (!strlen(directoryName) > 0 || !strlen(rutaOrigen) > 0 || !strlen(rutaDestino) > 0) {
		printf(
		ANSI_COLOR_RED "Por favor, ingrese el nombre del directorio, la carpeta de origen y la carpeta de destino\n");
		printf(ANSI_COLOR_GREEN "Ingrese \"Ayuda\" para una lista de comandos\n\n" ANSI_COLOR_RESET);
		return;
	}

	//Verifico que Exista la Ruta Origen (donde esta ahora el directorio)
	int oldFather = DIR_validarRuta(rutaOrigen, 0);
	if (oldFather == -1) {
		printf(ANSI_COLOR_RED "La ruta ingresada de origen no existe\n\n" ANSI_COLOR_RESET);
		return;
	}

	//Verifico que exista el directorio en la Ruta Origen
	int idCurrent = DIR_validarRuta(directoryName, oldFather);
	if (idCurrent == -1) {
		printf(ANSI_COLOR_RED "La carpeta a mover no se encuentra en la ruta de origen\n\n" ANSI_COLOR_RESET);
		return;
	}

	//Verifico que Exista la Ruta Destino (donde voy a mover el directorio)
	int newFather = DIR_validarRuta(rutaDestino, 0);
	if (newFather == -1) {
		printf(ANSI_COLOR_RED "La ruta ingresada de destino no existe\n\n" ANSI_COLOR_RESET);
		return;
	}

	//Verifico que NO exista el Directorio en la Ruta Destino
	iNumeroError = DIR_validarRuta(directoryName, newFather);
	if (iNumeroError != -1) {
		printf(
				ANSI_COLOR_RED "La carpeta a mover ya se encuentra en la ruta de Destino o Hay una Carpeta con el mismo Nombre\n\n" ANSI_COLOR_RESET);
		return;
	}

	//Ahora si muevo el Directorio
	iNumeroError = DIR_MoverConId(idCurrent, newFather);

	//Chequeo errores
	if (iNumeroError < 0) {
		printf(ANSI_COLOR_RED "Hubo un error al Mover el Directorio\n\n" ANSI_COLOR_RESET);
		return;
	}

	//Llegados a este punto se movio correctamente el directorio
	printf(ANSI_COLOR_GREEN "Se movio correctamente el directorio\n\n" ANSI_COLOR_RESET);
	return;
}


void console_renombrarCarpeta(char* rutaUbicacion, char*nombreActual, char* nombreNuevo) {
	//Variable para Manejar Errores, inicializo en 0 que significa no hay errores.
	int iNumeroError = 0;

	//Chequeo que  no sean Vacios los Strings
	if (!strlen(rutaUbicacion) > 0 || !strlen(nombreActual) > 0 || !strlen(nombreNuevo) > 0) {
		printf(
				ANSI_COLOR_RED "Por favor, ingrese Correctamente la Ruta dentro del File System, el Nombre Actual y el Nuevo Nombre\n" ANSI_COLOR_RESET);
		printf(ANSI_COLOR_GREEN "Ingrese \"Ayuda\" para una lista de comandos\n\n" ANSI_COLOR_RESET);
		return;
	}

	//Chequeo si existe la Ruta del Directorio dentro del File System
	int father = DIR_validarRuta(rutaUbicacion, 0);
	if (father == -1) {
		printf(ANSI_COLOR_RED "La ruta ingresada no existe\n\n" ANSI_COLOR_RESET);
		return;
	}

	//Verifico que el Directorio mismo exista
	int idCurrent = DIR_validarRuta(nombreActual, father);
	if (idCurrent == -1) {
		printf(ANSI_COLOR_RED "La carpeta a renombrar no se encuentra en la ruta de origen\n\n" ANSI_COLOR_RESET);
		return;
	}

	//Le digo a la Base de Datos que Renombre el Directorio
	iNumeroError = DIR_RenombrarConId(idCurrent, nombreNuevo);
	//Chequeo errores
	if (iNumeroError < 0) {
		printf("ANSI_COLOR_RED Hubo un error al Renombrar el Directorio\n\n" ANSI_COLOR_RESET);
		return;
	}

	//Llegados a este punto, se renombro correctamente la Carpeta dentro del File System, asi que lo imprimo por pantalla.
	printf( ANSI_COLOR_GREEN "Se Renombro con Exito la Carpeta dentro del File System.\n\n" ANSI_COLOR_RESET);
	return;
}


void console_borrarCarpeta(char* directoryName, char* ruta) {
	//Variable para Manejar Errores, inicializo en 0 que significa no hay errores.
	int iNumeroError = 0;

	//Verifico que los argumentos de "console_borrarCarpeta" sean validos (no esten vacios)
	if (string_is_empty(directoryName) || string_is_empty(ruta)) {
		printf(ANSI_COLOR_RED "Por favor, ingrese el Nombre del Directorio a Borrar y la Ruta donde se encuentra\n" ANSI_COLOR_RESET);
		printf(ANSI_COLOR_GREEN "Ingrese \"Ayuda\" para una lista de comandos\n\n" ANSI_COLOR_RESET);
		return;
	}

	//En caso que quiera borra la carpeta Root es lo mismo que formatear, asi que para que no explote, llamo a formatear
	if (string_equals_ignore_case(directoryName , "/") && string_equals_ignore_case(ruta , "/")) {
		//Llamo a FormatearFS
		console_formatearFS();
		//Salgo del Comando, NO debe seguir mas abajo
		return;
	}

	//Si llama solamente a Root como Directorio a Borrar, doy error
	if (string_equals_ignore_case(directoryName , "/")) {
		printf(ANSI_COLOR_RED "No se puede Borrar a Root dentro de %s\n" ANSI_COLOR_RESET, ruta);
		return;
	}

	//Chequeo que exista la ruta del directorio a Borrar, y ya que estoy obtengo su id
	int idRuta = DIR_validarRuta(ruta, 0);
	if (idRuta == -1) {
		printf(ANSI_COLOR_RED "La ruta ingresada no existe\n\n" ANSI_COLOR_RESET);
		return;
	}

	//Chequeo que Exista el Directorio Mismo y ya que estoy obtengo el ID de este
	int idDirectorioPadrePorBorrar = DIR_validarRuta(directoryName, idRuta);
	if (idDirectorioPadrePorBorrar == -1) {
		printf(ANSI_COLOR_RED "La carpeta a Borrar no se encuentra en la ruta indicada\n\n" ANSI_COLOR_RESET);
		return;
	}

	//Le digo a la Base de Datos que elimine el Directorio, los Directorios y Archivos Hijos que contiene
	iNumeroError = console_recursive_delete(idDirectorioPadrePorBorrar);
	//Chequeo errores
	if (iNumeroError < 0) {
		printf(
				ANSI_COLOR_RED "Hubo un error al Borrar el Directorio, sus Directorios Hijos o sus Archivos Hijos\n\n" ANSI_COLOR_RESET);
		return;
	}

	//Llegados a este punto se borro correctamente el directorio
	printf(ANSI_COLOR_GREEN "Se borro correctamente el directorio\n\n" ANSI_COLOR_RESET);
	return;
}




void console_cargarArchivo(char* rutaLinux, char* rutaFileSystem) {
	//Chequeo que  no sean Vacios los Strings
	if (!strlen(rutaLinux) > 0 || !strlen(rutaFileSystem) > 0) {
		printf(ANSI_COLOR_RED "Por favor, ingrese Correctamente la Ruta del Archivo dentro de Linux y dentro del File System" ANSI_COLOR_RESET "\n");
		printf(ANSI_COLOR_GREEN "Ingrese \"Ayuda\" para una lista de comandos" ANSI_COLOR_RESET "\n");
		return;
	}

	//Variable para ver los errores, inicializo en 0, que quiere decir no errores.
	int iNumeroError = 0;

	//Controlo si existe el Archivo y si tiene Permisos de Lectura.
	iNumeroError = Comun_existeArchivo(rutaLinux);
	if (iNumeroError == -1) {
		printf(ANSI_COLOR_RED "El Archivo No existe o No se tienen Permisos de Lectura" ANSI_COLOR_RESET "\n");
		return;
	}

	//Verifico que el Archivo no este Vacio
	uint32_t tamanioArchivo = Bloques_obtener_tamanio_archivo(rutaLinux);
	if (tamanioArchivo <= 0 ){
		printf(ANSI_COLOR_RED "Tamanio Invalido de Archivo" ANSI_COLOR_RESET "\n");
		printf("Posibles Causas:\n\t- El Archivo esta Vacio\n\t- El archivo esta Corrupto \n\n");
		return;
	}


	//Verifico si Hay Suficientes Nodos Conectados para Cumplir con la Cantidad de Copias Minimas
	int cantidadNodos = -1;
	iNumeroError = FS_ContarNodosActivos( &cantidadNodos );
	if (iNumeroError < 0 ){
		printf(ANSI_COLOR_RED "Hubo un Problema Al ver Cuantos Nodos Habia Activos" ANSI_COLOR_RESET "\n");
		return;

	} else if (cantidadNodos < CANTIDAD_COPIAS_MINIMAS ){
		printf(ANSI_COLOR_RED "No se puede subir el Archivo en Estos Momentos" ANSI_COLOR_RESET "\n");
		printf("Causa: hay %d de %d Nodos Activos Requeridos para Subir las %d Copias Minimas por Bloque" "\n\n", cantidadNodos, CANTIDAD_COPIAS_MINIMAS, CANTIDAD_COPIAS_MINIMAS);
		return;
	}

	//Obtenemos la Cantidad de Bloques que Ocurpara ese Archivo
	bool HayEspacio = false;
	tipo_id **NodosReservados = NULL;
	uint32_t cantidadBloques = -1;

	cantidadBloques = Bloques_obtener_cantidad_bloques_archivo( rutaLinux );
	if (cantidadBloques == -1) {
		printf(ANSI_COLOR_RED "No se pudo Obtener Cuantos Bloques Tenia el Archivo" ANSI_COLOR_RESET "\n");
		printf("Posibles Causas:\n\t- El Archivo no tiene un \\n final\n\t- Algun Bloque no tiene un \\n \n\n");
		return;
	}

	//Verificamos que Halla Espacio Suficiente para esa Cantidad de Bloques
	HayEspacio = FS_VerificarEspacioParaCopias(cantidadBloques,CANTIDAD_COPIAS_MINIMAS, &NodosReservados);
	if (HayEspacio == true) {
		FS_LiberarEspacioReservado(cantidadBloques,CANTIDAD_COPIAS_MINIMAS,NodosReservados);
		int bloqueActual;
		for (bloqueActual=cantidadBloques; bloqueActual <= cantidadBloques; bloqueActual++) {
			free(NodosReservados[bloqueActual-1]);
		}
		free(NodosReservados);
	}

	if (HayEspacio == false) {
		printf(ANSI_COLOR_RED "No hay Espacio Suficiente para el Archivo." ANSI_COLOR_RESET "\n\n");
		return;
	}

	//Pido el MD5 del archivo, La funcion ya se encarga de imprimir los errores
	char* md5 = Comun_obtener_MD5(rutaLinux, true);
	if (md5 == NULL) {
		printf(ANSI_COLOR_RED "Error al Obtener el MD5" ANSI_COLOR_RESET "\n\n");
		return;
	}

	//Muestro el MD5 por pantalla
	printf("El MD5 del Archivo es: %s \n", md5);

	//Chequeo si existe la Ruta del Archivo dentro del File System (lo copie de vos Ariel)
	tipo_id tIdDirDondeSubirlo = DIR_validarRuta(rutaFileSystem, 0);
	if (tIdDirDondeSubirlo < 0) {
		printf(ANSI_COLOR_RED "La ruta ingresada no existe" ANSI_COLOR_RESET "\n\n");
		return;
	}

	//Verifico que el archivo NO este ya subido O No este Subido Justo en esa ruta
	iNumeroError = FS_Validar_Archivo_Ruta(basename(rutaLinux), rutaFileSystem);
	//Chequeo si el Archivo ESTABA Subido (usa numeros de Error Especiales de 'console_Validar_Archivo_Ruta' Para chequear mejor
	if (iNumeroError >= 0) {
		printf(ANSI_COLOR_RED "El archivo Ya existe en la Base de Datos" ANSI_COLOR_RESET "\n\n");
		return;

	} else if ((iNumeroError == -1) || (iNumeroError == -4) || (iNumeroError == -5)) {		//NOTA: los Demas Numeros de Error Son validos Justo para este caso especial
		printf(ANSI_COLOR_RED "Exploto en 'FS_Validar_Archivo_Ruta' y No Deberia" ANSI_COLOR_RESET "\n\n");
		return;
	}

	//Funcion que lee el archivo, lo crea en el FileSystem con su MD5 y envia los bloques al File System para que elija a que nodos enviarselos.
	//NOTA: La funcion en si mismo ya se encarga de manejar los errores e imprimirlos por pantalla. Yo solo detecto que hubo un error y  muestro el mensaje de finalizacion.
	iNumeroError = FS_CargarCompletoArchivo(rutaLinux, md5, tIdDirDondeSubirlo, true);
	if (iNumeroError < 0) {
		printf(ANSI_COLOR_RED "No se pudieron leer y enviar los bloques del Archivo." ANSI_COLOR_RESET "\n\n");
		printf("Posibles Causas:\n\t- Se cayeron los Nodos al Enviar y no hay Suficientes Nodos para realizar las %d Copias Minimas \n\n", CANTIDAD_COPIAS_MINIMAS);
		return;
	}

	//Llegados a este punto, se subio correctamente el archivo al File System, asi que lo imprimo por pantalla.
	printf( ANSI_COLOR_GREEN "Se Cargo con Exito el Archivo al File System." ANSI_COLOR_RESET "\n\n");
	return;
}


void console_requestMd5(char* archivoNombre, char* archivoRutaDentroFS) {
	//Variable para Manejar Errores, inicializo en 0 que significa no hay errores.
	int iNumeroError = 0;
	//Variable para guardar el ID de Archivo
	tipo_id tIdArchivo = 0;

	//Chequeo que  no sean Vacios los Strings
	if (!strlen(archivoNombre) > 0 || !strlen(archivoRutaDentroFS) > 0) {
		printf(ANSI_COLOR_RED "Por favor, ingrese el nombre del directorio,y la carpeta de origen" ANSI_COLOR_RESET "\n");
		printf(ANSI_COLOR_GREEN "Ingrese \"Ayuda\" para una lista de comandos\n\n" ANSI_COLOR_RESET);
		return;
	}

	//Verifico que exista la Ruta del Archivo y obtengo su ID
	tIdArchivo = FS_Validar_Archivo_Ruta(archivoNombre, archivoRutaDentroFS);
	//Chequeo Errores
	if (tIdArchivo < 0) {
		printf(ANSI_COLOR_RED "No existe el archivo indicado\n\n" ANSI_COLOR_RESET);
		return;
	}

	//Pido el md5 a la base de datos (en base al ID de archivo) (en realidad estoy pidiendo datos de Mas que no uso luego)
	tipo_datos_file archivoDatos;
	iNumeroError = FILE_ConsultarId(tIdArchivo, &archivoDatos);

	//Chequeo Errores
	if (iNumeroError < 0) {
		printf(ANSI_COLOR_RED "Paso algo raro, si bien existe el archivo, no puedo Obtener su MD5\n\n" ANSI_COLOR_RESET);
		return;
	}

	//Imprimo por Pantalla el MD5
	printf("El MD5 es:  '%s'  \n\n", archivoDatos.md5);
	return;
}


void console_moverArchivo(char* nombreArchivo, char* rutaOrigen, char* rutaDestino) {
	//Variable para Manejar Errores, por defecto es 0 que significa sin errores
	int iNumeroError = 0;

	//Chequeo que  no sean Vacios los Strings
	if (!strlen(nombreArchivo) > 0 || !strlen(rutaOrigen) > 0 || !strlen(rutaDestino) > 0) {
		printf(ANSI_COLOR_RED "Por favor, ingrese el nombre del archivo, la ruta de origen y la ruta de destino\n");
		printf(ANSI_COLOR_GREEN "Ingrese \"Ayuda\" para una lista de comandos\n\n" ANSI_COLOR_RESET);
		return;
	}

	//Primero Valido que SI exista el archivo en su Ruta Origen
	tipo_id archivoID = FS_Validar_Archivo_Ruta(nombreArchivo, rutaOrigen);
	//Chequeo Errores
	if (archivoID < 0) {
		printf(ANSI_COLOR_RED "NO existe la Ruta o el archivo al cual se Quiere Mover\n\n" ANSI_COLOR_RESET);
		return;
	}

	//Segundo Valido que NO exista el archivo en su Ruta Destino
	iNumeroError = FS_Validar_Archivo_Ruta(nombreArchivo, rutaDestino);
	//Chequeo Errores
	if (iNumeroError > 0) {
		printf(ANSI_COLOR_RED "Ya existe el archivo en Esa Ruta o Hay un archivo con el mismo Nombre\n\n" ANSI_COLOR_RESET);
		return;
	} else if ((iNumeroError == -1)) {
		printf(ANSI_COLOR_RED "No existe la Ruta Destino \n\n" ANSI_COLOR_RESET);
		return;
	} else if ((iNumeroError == -4) || (iNumeroError == -5)) {		//NOTA: los Demas Numeros de Error Son validos Justo para este caso especial
		printf(ANSI_COLOR_RED "Exploto en 'console_Validar_Archivo_Ruta' y No Deberia \n\n" ANSI_COLOR_RESET);
		return;
	}

	//Valido que la Ruta Destino exista y obtengo el ID del Nuevo Padre en la Ruta Destino
	tipo_id idPadreDestino = DIR_validarRuta(rutaDestino, 0);
	if (idPadreDestino == -1) {
		printf(ANSI_COLOR_GREEN "La ruta Destino no existe\n\n" ANSI_COLOR_RESET);
		return;
	}

	//Ahora Muevo el archivo de un directorio a otro
	iNumeroError = FILE_Mover(archivoID, idPadreDestino);
	//Chequeo Errores
	if (iNumeroError < 0) {
		printf(ANSI_COLOR_RED "No se pudo Mover el Archivo\n\n" ANSI_COLOR_RESET);
		return;
	}

	//Llegados a este punto, se movio correctamente el Archivo dentro del File System, asi que lo imprimo por pantalla.
	printf( ANSI_COLOR_GREEN "Se Movio con Exito el Archivo dentro del File System.\n\n" ANSI_COLOR_RESET);
	return;
}


void console_renombrarArchivo(char* rutaUbicacion, char*nombreActual, char* nombreNuevo) {
	//Variable para Manejar Errores, por defecto es 0 que significa sin errores
	int iNumeroError = 0;

	//Chequeo que  no sean Vacios los Strings
	if (!strlen(rutaUbicacion) > 0 || !strlen(nombreActual) > 0 || !strlen(nombreNuevo) > 0) {
		printf(ANSI_COLOR_RED "Por favor, ingrese Correctamente la Ruta dentro del File System, el Nombre Actual y el Nuevo Nombre\n" ANSI_COLOR_RESET);
		printf(ANSI_COLOR_GREEN "Ingrese \"Ayuda\" para una lista de comandos\n\n" ANSI_COLOR_RESET);
		return;
	}

	//Primero Valido que SI exista el archivo con el Nombre Actual
	tipo_id archivoID = FS_Validar_Archivo_Ruta(nombreActual, rutaUbicacion);
	//Chequeo Errores
	if (archivoID < 0) {
		printf(ANSI_COLOR_RED "NO existe la Ruta o el archivo al cual se Quiere Renombrar\n\n" ANSI_COLOR_RESET);
		return;
	}

	//Segundo Valido que NO exista el archivo con el Nombre Nuevo
	iNumeroError = FS_Validar_Archivo_Ruta(nombreNuevo, rutaUbicacion);
	//Chequeo Errores
	if (iNumeroError >= 0) {
		printf(ANSI_COLOR_RED "Ya existe un archivo con el nombre al cual se Quiere Renombrar\n\n" ANSI_COLOR_RESET);
		return;
	} else if ((iNumeroError == -1) || (iNumeroError == -4) || (iNumeroError == -5)) {		//NOTA: los Demas Numeros de Error Son validos Justo para este caso especial
		printf(ANSI_COLOR_RED "Exploto en 'console_Validar_Archivo_Ruta' y No Deberia \n\n" ANSI_COLOR_RESET);
		return;
	}

	//Ahora hago el Renombrar por el "nombreNuevo"
	iNumeroError = FILE_Renombrar(archivoID, nombreNuevo);
	//Chequeo Errores
	if (iNumeroError < 0) {
		printf(ANSI_COLOR_RED "No se pudo realizar el Renombrar\n\n" ANSI_COLOR_RESET);
		return;
	}

	//Llegados a este punto, se renombro correctamente el Archivo dentro del File System, asi que lo imprimo por pantalla.
	printf( ANSI_COLOR_GREEN "Se Renombro con Exito el Archivo dentro del File System.\n\n" ANSI_COLOR_RESET);
	return;

}


void console_descargarArchivo(char* rutaLinux, char* rutaFileSystem) {
	//Divido en Nombre archivo y Ruta
	char* archivoNombre = basename(rutaFileSystem);
	char* archivoRuta = Comun_obtenerRutaDirectorio(rutaFileSystem);

	//Chequeo que  no sean Vacios los Strings
	if (!strlen(rutaLinux) > 0 || !strlen(rutaFileSystem) > 0) {
		printf(
				ANSI_COLOR_RED "Por favor, ingrese Correctamente la Ruta del Archivo dentro de Linux y dentro del File System\n" ANSI_COLOR_RESET);
		printf(ANSI_COLOR_GREEN "Ingrese \"Ayuda\" para una lista de comandos\n\n" ANSI_COLOR_RESET);
		return;
	}

	//Variable para ver los errores, inicializo en 0, que quiere decir no errores.
	int iNumeroError = 0;


	//Valido que exista la Ruta, que Existe el archivo Justo en esa ruta y obtengo ID de archivo
	tipo_id archivoID;
	archivoID = FS_Validar_Archivo_Ruta(archivoNombre, archivoRuta);
	//Chequeo Errores
	if (archivoID < 0) {
		printf(ANSI_COLOR_RED "NO existe la Ruta o el archivo al cual se Quiere Descargar\n\n" ANSI_COLOR_RESET);
		return;
	}

	Macro_ImprimirParaDebug("Validada la Ruta, que Existe el archivo Justo en esa ruta y Obtenido ID de archivo\n");

	//Hago Free del Char "archivoRuta" porque no lo uso mas.
	free(archivoRuta);

	//Crea el archivo de texto dentro del linux (si existiese lo sobrescribe)
	FILE *archivoTexto = fopen(rutaLinux, "w");
	if (archivoTexto == NULL) {
		//Corto la ejecucion e informo que paso
		printf(ANSI_COLOR_RED "Problema al crear el Archivo en la ruta: %s" ANSI_COLOR_RESET "\n", rutaLinux);
		Macro_Imprimir_Error("El error detectado es: ");
		return;
	}

	Macro_ImprimirParaDebug("Archivo Creado y listo para escribir los Bloques\n");

	//Voy a pedirle al File System la cantidad de bloques del archivo "rutaFileSystem"
	//Primero, obtengo la cantidad de bloques
	tipo_datos_file archivoDatos;
	iNumeroError = FILE_ConsultarId(archivoID, &archivoDatos);
	//Chequeo Errores
	if (iNumeroError < 0) {
		printf(ANSI_COLOR_RED "No se pudo obtener los Datos del archivo" ANSI_COLOR_RESET "\n");
		printf("Posibles Causas:\n\t- La tabla de Archivos esta Corrupta \n\n");
		return;
	}

	//Tercero y ultimo, copio a una Variable la cantidad de bloques del archivo. Necesito que sea del tipo long por si tiene muchos bloques.
	long cantidadBloques = archivoDatos.iCantBloques;

	//Tercero y Ultimo, Valido si el archivo esta disponible o no
	if (archivoDatos.disponible == false) {
		printf(ANSI_COLOR_RED "El Archivo NO esta Disponible" ANSI_COLOR_RESET "\n");
		return;
	}

	Macro_ImprimirParaDebug("El Archivo Esta Disponible\n");

	//Voy pidiendo al File System que me de bloque por bloque asi lo voy guardando en el archivo nuevo. Empiezo con el 1er Bloque.
	long numeroBloqueActual = 1;
	for (numeroBloqueActual = 1; numeroBloqueActual <= cantidadBloques; numeroBloqueActual++) {

		//Ahora le Pido al FS que se conecte a un Nodo que tenga este bloque y me lo envie
		//Macro_ImprimirParaDebug("Le pido el Bloque %d al FS\n", (int)numeroBloqueActual);

		//Uso un tipo bloque para cargarle los bloques que reciba del file system asi los puedo escribir en el archivo.
		tipo_bloque* bloqueDeTrabajo = console_PedirFSBloqueArchivo(archivoNombre, numeroBloqueActual);
		//Chequeo Errores
		if(bloqueDeTrabajo==NULL){
			//No se pudo obtener este Bloque, Se cancela el Comando
			printf(ANSI_COLOR_RED "Error al Pedirle al FS el Bloque" ANSI_COLOR_RESET "\n");
			printf("Por favor revise dentro del archivo que se estaba creando en <%s> para ver que fue lo ultimo que se escribio, asi se detecta que bloque dio error.\n " , rutaLinux);

			//Libero la Memoria del ultimo bloque (los 20mb).
			Bloques_destruir(bloqueDeTrabajo);

			//Ciero el archivo y veo si hay errores.
			iNumeroError = fclose(archivoTexto);
			if (iNumeroError != 0) {
				printf(ANSI_COLOR_RED "Hubo un Error al cerrar el archivo. Probablemente no se Haya guardado bien los datos del archivo" ANSI_COLOR_RESET "\n");
				return;
			}
			//Se cancela el comando
			return;
		}

		Macro_ImprimirParaDebug("Obtube el Bloque\n");

		//Reviso que el  bloque tenga un tamanio mayor a 0. En caso de no ser mayor a 0 tenemos un problema.
		if (bloqueDeTrabajo->tamanioBloque > 0) {
			Macro_ImprimirParaDebug("El bloque Tiene un Tamalo mayor a 0, es de %d\n", (int)bloqueDeTrabajo->tamanioBloque);

			//Escribo el contenido del bloque en el archivo creado.
			//fputs(bloqueDeTrabajo->contenidoBloque, archivoTexto);
			iNumeroError = fwrite( bloqueDeTrabajo->contenidoBloque, sizeof(char), bloqueDeTrabajo->tamanioBloque, archivoTexto );
			if( ferror(archivoTexto) != 0 ){
				Bloques_destruir(bloqueDeTrabajo);
				printf( ANSI_COLOR_RED "No se Pudo Escribir en el Archivo:'%s'\n" ANSI_COLOR_RESET, rutaLinux);
				return;
			}



			//Libero la Memoria del bloque (los 20mb).
			Bloques_destruir(bloqueDeTrabajo);
		} else {
			//Como el tamanio del bloque no es correcto, Paro de pedir bloques y muestro el error y que paso.
			printf("Estaba leyendo bloque numero %d y se detecto que tenia un tamanio de %d.\n",
					(int) numeroBloqueActual, (int) bloqueDeTrabajo->tamanioBloque);
			printf(	"Por favor revise dentro del archivo que se estaba creando en <%s> para ver que fue lo ultimo que se escribio, asi se detecta que bloque dio error.\n ",
					rutaLinux);

			Macro_ImprimirParaDebug("El contenido del Bloque es:\n %s\n", bloqueDeTrabajo->contenidoBloque);

			//Libero la Memoria del ultimo bloque (los 20mb).
			Bloques_destruir(bloqueDeTrabajo);

			//Ciero el archivo y veo si hay errores.
			iNumeroError = fclose(archivoTexto);
			if (iNumeroError != 0) {
				printf(ANSI_COLOR_RED "Hubo un Error al cerrar el archivo. Probablemente no se Haya guardado bien los datos del archivo" ANSI_COLOR_RESET "\n");
				return;
			}
			//Se cancela el comando
			return;
		}
	}

	Macro_ImprimirParaDebug("Termine de Escribir los Bloques\n");

	//Ciero el archivo y veo si hay errores.
	iNumeroError = fclose(archivoTexto);
	if (iNumeroError != 0) {
		printf( ANSI_COLOR_RED "Hubo un Error al cerrar el archivo" ANSI_COLOR_RESET "\n");
		return;
	}

	Macro_ImprimirParaDebug("Cerre Correctamente el Archivo\n");

	char* md5archivo;
	md5archivo = Comun_obtener_MD5(rutaLinux,true);
	if (md5archivo == NULL) {
		printf( ANSI_COLOR_RED "No se pudo Calcular el MD5 del Archivo Descargado. Probablemente Este Corrupto" ANSI_COLOR_RESET "\n\n");
		unlink(rutaLinux);

	}else if (strcmp(md5archivo,archivoDatos.md5) == 0) {
		//Llegados a este punto, se descargo correctamente el archivo desde el File System, asi que lo imprimo por pantalla.
		printf( ANSI_COLOR_GREEN "Se Descargo con Exito el Archivo desde el File System.\n" ANSI_COLOR_RESET);
		printf("Se comprobo que el MD5 Del Archivo Subido Originalmente y el Descargado Coinciden.\n\n");

	} else {
		printf( ANSI_COLOR_RED "Hubo un Error al descargar el archivo, los MD5 no coinciden" ANSI_COLOR_RESET "\n");
		printf("El MD5 Esperado era '%s' Pero el Calculado del Archivo Descargado es '%s' \n\n", archivoDatos.md5, md5archivo);
		unlink(rutaLinux);
	}
	return;
}


void console_borrarArchivo(char* archivoNombre, char* archivoRuta) {
	//Variable para Manejar Errores, por defecto es 0 que significa sin errores
	int iNumeroError = 0;

	if (!strlen(archivoNombre) > 0 || !strlen(archivoRuta) > 0) {
		printf(ANSI_COLOR_RED "Por favor, ingrese el nombre del archivo y la ruta de origen\n");
		printf(ANSI_COLOR_GREEN "Ingrese \"Ayuda\" para una lista de comandos\n\n" ANSI_COLOR_RESET);
		return;
	}

	//Valido Ruta y Archivo, y obtengo su ID
	tipo_id archivoID = FS_Validar_Archivo_Ruta(archivoNombre, archivoRuta);
	//Chequeo Errores
	if (archivoID < 0) {
		printf(ANSI_COLOR_RED "NO existe la Ruta o el archivo al cual se Quiere Borrar\n\n" ANSI_COLOR_RESET);
		return;
	}


	//Elimino el Archivo y Dejo como Vacios todos los bloques que ocupaba
	iNumeroError = FS_EliminarArchivo(archivoID);
	//Chequeo Errores
	if (iNumeroError < 0) {
		printf(ANSI_COLOR_RED "No se pudo eliminar el archivo\n\n" ANSI_COLOR_RESET);
		return;
	}

	//Llegados a este punto, se Elimino correctamente el Archivo dentro del File System, asi que lo imprimo por pantalla.
	printf( ANSI_COLOR_GREEN "Se Elimino con Exito el Archivo dentro del File System.\n\n" ANSI_COLOR_RESET);
	return;
}




void console_verBloqueNodo(char* nodoNombre, char* numeroBloque, char* rutaArchivoPorGenerar) {
	//Variable para Manejar Errores, por defecto es 0 que significa sin errores
	int iNumeroError = 0;

	//Verifico que los Strings no esten Vacios
	if (string_is_empty(nodoNombre) || string_is_empty(numeroBloque)) {
		printf(ANSI_COLOR_RED "Por favor, ingrese cel nombre del Bloque y el Numero de Bloque" ANSI_COLOR_RESET "\n");
		printf(ANSI_COLOR_GREEN "Ingrese \"Ayuda\" para una lista de comandos\n\n" ANSI_COLOR_RESET);
		return;
	}

	//Valido que Exista el Nodo
	tipo_id nodoID;
	int cantidadNodosConMismoNombre = -1;
	iNumeroError = NODO_BuscarNombre(nodoNombre, &nodoID, DB_SET, &cantidadNodosConMismoNombre);
	//Chequeo Errores
	if (iNumeroError < 0) {
		printf(ANSI_COLOR_RED "No existe el Nodo\n\n" ANSI_COLOR_RESET);
		return;
	}

	//Obtengo los Datos del Nodo
	tipo_datos_nodo nodoDatos;
	iNumeroError = NODO_Consultar(nodoID, &nodoDatos);
	//Chequeo Errores
	if (iNumeroError < 0) {
		printf(ANSI_COLOR_RED "Esto es raro, si bien existe el Nodo, no se pueden obtener sus Datos\n\n" ANSI_COLOR_RESET);
		printf("Posibles Causas:\n\t- La tabla de Nodos esta Corrupta \n\n");
		return;
	}

	//Valido que Contenga al Bloque
	if (nodoDatos.bloques_totales <= atol(numeroBloque)) {
		printf(ANSI_COLOR_RED "No existe ese Numero de Bloque en el Nodo, solo llega hasta los %d Bloques y vos pediste el Bloque %d\n" ANSI_COLOR_RESET, nodoDatos.bloques_totales, (int) atol(numeroBloque));
		return;
	}

	//Crea el archivo de texto dentro del linux (si existiese lo sobrescribe)
	FILE* archivoTexto = fopen(rutaArchivoPorGenerar, "w");
	if (archivoTexto == NULL) {
		//Corto la ejecucion e informo que paso
		printf(ANSI_COLOR_RED "Problema al crear el Archivo en la ruta: %s\n" ANSI_COLOR_RESET, rutaArchivoPorGenerar);
		perror("El error detectado es: ");
		return;
	}

	// FIXME: Esto alguna vez funciono??? estaba datasDelBloque en lugar de IdDelBloque
	tipo_datos_bloques datosDelBloque;
	tipo_id IdDelBloque;
	iNumeroError = FS_ObtenerBloqueNodo(nodoID,atol(numeroBloque),&IdDelBloque);
	//Chequeo Errores
	if (iNumeroError < 0) {
		printf(ANSI_COLOR_RED "No existe el Bloque indicado\n\n" ANSI_COLOR_RESET);

		return;
	}

	iNumeroError = BLOQUES_Consultar(IdDelBloque, &datosDelBloque);
	//Chequeo Errores
	if (iNumeroError < 0) {
		printf(
		ANSI_COLOR_RED "Esto es raro, si bien existe el Bloque, no se pueden obtener sus Datos\n\n" ANSI_COLOR_RESET);
		printf("Posibles Causas:\n\t- La tabla de Bloques de Nodos esta Corrupta \n\n");
		return;
	}


	tipo_bloque* bloqueDeTrabajo = console_PedirNodoBloque(nodoID, atol(numeroBloque), datosDelBloque.tamano);
	//Chequeo Errores
	if(bloqueDeTrabajo==NULL){
		printf(ANSI_COLOR_RED "ERROR: Hubo un Problema al Pedir el Bloque al Nodo\n\n" ANSI_COLOR_RESET);
		//El Nodo esta caido, hay que marcarlo como No Disponible en la Base de Datos
		Macro_ImprimirParaDebug(ANSI_COLOR_RED "No se pudo conectar al Nodo\n" ANSI_COLOR_RESET);

		//El NODO esta caido, hay que marcarlo como No Disponible en la Base de Datos
		iNumeroError = FS_DesactivarNodo(nodoDatos.sNombre);
		if (iNumeroError < 0) {
			Macro_ImprimirParaDebug(ANSI_COLOR_RED "Error: No se pudo Desactivar al Nodo\n" ANSI_COLOR_RESET);
		}
		return;
	}

	printf("El Bloque tiene '%ld' de '%ld' Bytes Utiles, Representan un %d%% del Bloque\n", (long int)bloqueDeTrabajo->tamanioBloque, (long int)TAMANIO_BLOQUE, (int)( (bloqueDeTrabajo->tamanioBloque / TAMANIO_BLOQUE)*100 ));
	printf("El Resto ('%ld') son Byte '0' de Relleno\n", (TAMANIO_BLOQUE - bloqueDeTrabajo->tamanioBloque) );
	printf("Solo te Guardo en el Archivo los Bytes Utiles\n");

	//Guardo el Contenido del Bloque en el Archivo
	//iNumeroError = fputs(bloqueDeTrabajo->contenidoBloque , archivoTexto);
	iNumeroError = fwrite( bloqueDeTrabajo->contenidoBloque, sizeof(char), bloqueDeTrabajo->tamanioBloque, archivoTexto );
	if( ferror(archivoTexto) != 0 ){
		Bloques_destruir(bloqueDeTrabajo);
		printf( ANSI_COLOR_RED "No se Pudo Escribir en el Archivo:'%s'\n" ANSI_COLOR_RESET, rutaArchivoPorGenerar);
		return;
	}


	//Libero la Memoria del bloque (los 20mb).
	Bloques_destruir(bloqueDeTrabajo);

	//Ciero el archivo y veo si hay errores.
	iNumeroError = fclose(archivoTexto);
	if (iNumeroError != 0) {
		perror("Hubo un Error al cerrar el archivo, error: ");
		return;
	}


	//Llegados a este punto, se Guardo correctamente el Bloque, asi que lo imprimo por pantalla.
	printf( ANSI_COLOR_GREEN "Se Guardo con Exito el Bloque, Por Favor Revise el Archivo Indicado.\n\n" ANSI_COLOR_RESET);
	return;
}


void console_copiarBloqueANodo(char*nodoOrigenNombre, char* nodoOrigenNumeroBloque, char* nodoDestinoNombre,
		char* nodoDestinoNumeroBloque) {
	//Variable para Manejar Errores, inicializo en 0 que significa no hay errores.
	int iNumeroError = 0;

	//Chequeo que los argumentos no esten vacios
	if (string_is_empty(nodoOrigenNombre) || string_is_empty(nodoOrigenNumeroBloque)
			|| string_is_empty(nodoDestinoNombre) || string_is_empty(nodoDestinoNumeroBloque)) {
		printf(
				ANSI_COLOR_RED "Por favor, ingrese el nombre del Nodo Origen y destino y los Numeros de bloque Origen y Destino" ANSI_COLOR_RESET "\n");
		printf(ANSI_COLOR_GREEN "Ingrese \"Ayuda\" para una lista de comandos\n\n" ANSI_COLOR_RESET);
		return;
	}

	//Valido exista el Nodo Origen
	tipo_id nodoOrigenID;
	int cantidadNodosConMismoNombre = -1;
	iNumeroError = NODO_BuscarNombre(nodoOrigenNombre, &nodoOrigenID, DB_SET, &cantidadNodosConMismoNombre);
	if (iNumeroError < 0) {
		printf(ANSI_COLOR_RED "No existe el Nodo Origen\n\n" ANSI_COLOR_RESET);
		return;
	}

	//Busco los Datos del Nodo Origen, para obtener la IP y Puerto
	tipo_datos_nodo nodoOrigenDatos;
	iNumeroError = NODO_Consultar(nodoOrigenID, &nodoOrigenDatos);
	if (iNumeroError < 0) {
		printf(	ANSI_COLOR_RED "Esto es Raro, si bien existe el Nodo Origen, no se pueden obtener los datos de este\n\n" ANSI_COLOR_RESET);
		printf("Posibles Causas:\n\t- La tabla de Bloques de Nodos esta Corrupta \n\n");
		return;
	}

	//Valido que el Numero de Bloque Origen exista
	// Podria usar la funcion para buscar el Numero de bloque del nodo, y si no existe explota, porque que este dentro de los valores
	// permitidos, no significa que el bloque efectivamente exista, en especial el de Origen

	if (nodoOrigenDatos.bloques_totales <= atol(nodoOrigenNumeroBloque)) {
		printf(	ANSI_COLOR_RED "No existe ese Numero de Bloque en el Nodo Origen, solo llega hasta los %d Bloques y vos pediste el Bloque %d\n" ANSI_COLOR_RESET,
				nodoOrigenDatos.bloques_totales, (int) atol(nodoOrigenNumeroBloque));
		return;
	}

	//Valido que el Nodo Origen Tenga el Bloque Disponible

	//Primero Obtengo el ID del Bloque
	tipo_id nodoOrigenBloqueID;
	iNumeroError = FS_ObtenerBloqueNodo(nodoOrigenID, atol(nodoOrigenNumeroBloque), &nodoOrigenBloqueID);
	if (iNumeroError < 0) {
		printf(ANSI_COLOR_RED "El Bloque en el Nodo Origen esta Vacio. No hay nada para Copiar.\n\n" ANSI_COLOR_RESET);
		return;
	}

	//Segundo Obtengo los Datos
	tipo_datos_bloques nodoOrigenBloqueDatos;
	iNumeroError = BLOQUES_Consultar(nodoOrigenBloqueID, &nodoOrigenBloqueDatos);
	if (iNumeroError < 0) {
		printf(ANSI_COLOR_RED "No se pudo los Datos del Bloque en el Nodo Origen\n\n" ANSI_COLOR_RESET);
		return;
	}

	//Tercero y Final, ahorasi verifico si esta Disponible
	if (nodoOrigenBloqueDatos.disponible == false) {
		printf(ANSI_COLOR_RED "El Bloque en Nodo Origen no esta Disponible\n\n" ANSI_COLOR_RESET);
		return;
	}

	//Busco el ID del Nodo Destino
	tipo_id nodoDestinoID;
	cantidadNodosConMismoNombre = -1;
	iNumeroError = NODO_BuscarNombre(nodoDestinoNombre, &nodoDestinoID, DB_SET, &cantidadNodosConMismoNombre);
	if (iNumeroError < 0) {
		printf(ANSI_COLOR_RED "No existe el Nodo Destino\n\n" ANSI_COLOR_RESET);
		return;
	}

	//Busco los Datos del Nodo Destino, para obtener la IP y Puerto
	tipo_datos_nodo nodoDestinoDatos;
	iNumeroError = NODO_Consultar(nodoDestinoID, &nodoDestinoDatos);
	if (iNumeroError < 0) {
		printf(	ANSI_COLOR_RED "Esto es Raro, si bien existe el Nodo Destino, no se pueden obtener los datos de este\n\n" ANSI_COLOR_RESET);
		printf("Posibles Causas:\n\t- La tabla de Bloques de Nodos esta Corrupta \n\n");
		return;
	}

	//Valido que el Numero de Bloque Destino exista
	if (nodoDestinoDatos.bloques_totales <= atol(nodoDestinoNumeroBloque)) {
		printf(	ANSI_COLOR_RED "No existe ese Numero de Bloque en el Nodo Destino, solo llega hasta los %d Bloques y vos pediste el Bloque %d\n" ANSI_COLOR_RESET,
				nodoDestinoDatos.bloques_totales, (int) atol(nodoDestinoNumeroBloque));
		return;
	}

	//Valido que el Nodo Destino Tenga el Bloque Vacio o No Disponible

	//Primero Obtengo el ID del Bloque
	tipo_id nodoDestinoBloqueID;
	// Busco el Id del Nodo y Bloque a ver si existe, si existe es porque esta asignado y aborto la operacion, si no existe esta libre
	iNumeroError = FS_ObtenerBloqueNodo(nodoDestinoID, atol(nodoDestinoNumeroBloque), &nodoDestinoBloqueID);
	if (iNumeroError != DB_NOTFOUND) {
		printf(ANSI_COLOR_RED "El Bloque en Nodo Destino Ya esta Ocupado con un Bloque de Un Archivo\n\n" ANSI_COLOR_RESET);
		return;
	}


	//iNumeroError = FS_ObtenerBloqueNodo(nodoDestinoID, atol(nodoDestinoNumeroBloque), &nodoDestinoBloqueID);
	// Esta es la funcion para buscar un bloque libre en un nodo
/*	iNumeroError = FS_ObtenerNumeroBloquesNodo(nodoDestinoID, &nodoDestinoBloqueID);
	if (iNumeroError < 0) {
		printf(ANSI_COLOR_RED "No se pudo el ID del Bloque en el Nodo Destino\n\n" ANSI_COLOR_RESET);
		return;
	}
	*/

	//Segundo Obtengo los Datos
	// No es necesario obtener los datos del Destino ya que se supone el bloque esta vacio, con basura, o datos que no nos importan
	/*tipo_datos_bloques nodoDestinoBloqueDatos;
	iNumeroError = BLOQUES_Consultar(nodoDestinoBloqueID, &nodoDestinoBloqueDatos);
	if (iNumeroError < 0) {
		printf(ANSI_COLOR_RED "No se pudo los Datos del Bloque en el Nodo Destino\n\n" ANSI_COLOR_RESET);
		return;
	}*/

	//Tercero y Final, ahora si verifico que No este Disponible
	/*if (nodoDestinoBloqueDatos.disponible == true) {
		printf(ANSI_COLOR_RED "El Bloque en Nodo Destino Ya esta Ocupado con otra cosa!\n\n" ANSI_COLOR_RESET);
		return;
	}*/

	//Veo si NO esta Disponible el Nodo Destino
	if (  !Sockets_estaDisponibleServidor(nodoDestinoDatos.nodoIP, nodoDestinoDatos.nodoPuerto)  ) {
		printf(ANSI_COLOR_RED "No se pudo conectar al Nodo Destino\n\n" ANSI_COLOR_RESET);
		//El NODO esta caido, hay que marcarlo como No Disponible en la Base de Datos
		iNumeroError = FS_DesactivarNodo(nodoDestinoNombre);
		if (iNumeroError < 0) {
			printf(ANSI_COLOR_RED "No se pudo Desactivar al Nodo Destino\n\n" ANSI_COLOR_RESET);
		}

		//Si esta caido el Nodo destino (donde queremos que se copie el bloque), se cancela el Comando, Ya que no tiene sentido.
		return;
	}

	//Para Reservar al Bloque necesito el numero de Copia, asi lo guardo como la siguiente copia.

	int numeroCopiaBloque = 0;
	iNumeroError = FS_CantidadCopiasBloqueArchivo(nodoOrigenBloqueDatos.tIdArchivo, nodoOrigenBloqueDatos.numeroBloqueDentroArchivo,
			&numeroCopiaBloque);
	if (iNumeroError < 0) {
		printf(
				ANSI_COLOR_RED "No se obtener la Cantidad de Copias que lleva hasta el Momento el Archivo y por ende no puedo reservar el bloque\n\n" ANSI_COLOR_RESET);
		return;
	}


	//Reservo en la Base el Bloque del Nodo Destino
	tipo_id bloqueReservadoID;
	iNumeroError = FS_ReservarBloque(nodoDestinoID, atol(nodoDestinoNumeroBloque), numeroCopiaBloque + 1,
			nodoOrigenBloqueDatos.numeroBloqueDentroArchivo, nodoOrigenBloqueDatos.tIdArchivo, &bloqueReservadoID);
	if (iNumeroError < 0) {
		printf(ANSI_COLOR_RED "No se pudo reservar el Bloque en el Nodo Destino\n\n" ANSI_COLOR_RESET);
		return;
	}

	//Me conecto al Nodo (Si me pude conectar es que el nodo NO esta caido)
	tipo_socket* socketConectadoAlNodo = Sockets_conectar_servidor(nodoOrigenDatos.nodoIP, nodoOrigenDatos.nodoPuerto);
	if (socketConectadoAlNodo == NULL) {
		//El Nodo esta caido, hay que marcarlo como No Disponible en la Base de Datos

		printf(ANSI_COLOR_RED "No se pudo conectar al Nodo Origen\n\n" ANSI_COLOR_RESET);

		//Marco como Desactivado el Nodo en la Base de Datos
		iNumeroError = FS_DesactivarNodo(nodoOrigenNombre);
		if (iNumeroError < 0) {
			Macro_ImprimirParaDebug(ANSI_COLOR_RED "No se pudo Desactivar al Nodo Origen\n" ANSI_COLOR_RESET);
		}

		//Des-Reservo el Bloque reservado
		iNumeroError = FS_LiberarBloque(bloqueReservadoID);
		if (iNumeroError < 0) {
			Macro_ImprimirParaDebug(ANSI_COLOR_RED "No se pudo Des-Reservar al Bloque Reservado\n" ANSI_COLOR_RESET);
		}

		return;
	}

	//Ahora voy a preparar la Orden que le indique al Nodo Origen que bloque (que numero de bloque de su archivo .bin) va a copiar al Nodo destino (pasamos la Ip y el Puerto) en que bloque de su archivo BIN
	t_datos_orden_copiar_bloque datosPorSerializar;
	datosPorSerializar.nodoOrigenNumeroBloque = atol(nodoOrigenNumeroBloque);
	strcpy(datosPorSerializar.nodoDestinoIP , nodoDestinoDatos.nodoIP);
	datosPorSerializar.nodoDestinoPuerto = atol(nodoDestinoDatos.nodoPuerto);
	datosPorSerializar.nodoDestinoNumeroBloque = atol(nodoDestinoNumeroBloque);
	datosPorSerializar.tamanioBloque = nodoOrigenBloqueDatos.tamano;

	uint32_t tamanioSerializacion = -1;
	char* orden = serializar_orden_copiar(datosPorSerializar, &tamanioSerializacion);


	//Serializamos la orden con el Header
	char* ordenSerializada = package_create(orden, tamanioSerializacion, "copiarBloqueANodo", FILESYSTEM);

	//Libero la Memoria del Char* serializado
	free(orden);

	//Enviamos la Orden de Copia
	iNumeroError = Sockets_enviar_datos(socketConectadoAlNodo, ordenSerializada);
	if (iNumeroError <= 0) {
		printf(ANSI_COLOR_RED "Se cayo el Nodo Origen al enviar la Orden de Copia\n\n" ANSI_COLOR_RESET);

		//Marco como Desactivado el Nodo en la Base de Datos
		iNumeroError = FS_DesactivarNodo(nodoOrigenNombre);
		if (iNumeroError < 0) {
			Macro_ImprimirParaDebug(ANSI_COLOR_RED "No se pudo Desactivar al Nodo Origen\n" ANSI_COLOR_RESET);
		}

		//Des-Reservo el Bloque reservado
		iNumeroError = FS_LiberarBloque(bloqueReservadoID);
		if (iNumeroError < 0) {
			Macro_ImprimirParaDebug(ANSI_COLOR_RED "No se pudo Des-Reservar al Bloque Reservado\n" ANSI_COLOR_RESET);
		}
		return;
	}

	//Libero memoria porque ya no se usa mas
	package_destroy(ordenSerializada);

	//Esperamos confirmacion de que se copio el bloque
	t_header headerRecibido;
	iNumeroError = Sockets_recibir_Header(socketConectadoAlNodo, &headerRecibido);
	if (iNumeroError == 0) {
		//Se cayo el Nodo a medio copiar el bloque
		printf(
		ANSI_COLOR_RED "Se cayo el Nodo Origen a medio copiar el bloque\nSe cancela el comando\n\n" ANSI_COLOR_RESET);

		///Marco como Desactivado el Nodo en la Base de Datos
		iNumeroError = FS_DesactivarNodo(nodoOrigenNombre);
		if (iNumeroError < 0) {
			Macro_ImprimirParaDebug(ANSI_COLOR_RED "No se pudo Desactivar al Nodo Origen\n" ANSI_COLOR_RESET);
		}

		//Des-Reservo el Bloque reservado
		iNumeroError = FS_LiberarBloque(bloqueReservadoID);
		if (iNumeroError < 0) {
			Macro_ImprimirParaDebug(ANSI_COLOR_RED "No se pudo Des-Reservar al Bloque Reservado\n" ANSI_COLOR_RESET);
		}

		return;
	}

	char* confirmacion;
	iNumeroError = Sockets_recibir_Datos(socketConectadoAlNodo, &confirmacion, headerRecibido);
	if (iNumeroError == 0) {
		//Se cayo el Nodo a medio copiar el bloque
		printf(ANSI_COLOR_RED "Se cayo el Nodo Origen a medio copiar el bloque\nSe cancela el comando\n\n" ANSI_COLOR_RESET);

		//Marco como Desactivado el Nodo en la Base de Datos
		iNumeroError = FS_DesactivarNodo(nodoOrigenNombre);
		if (iNumeroError < 0) {
			Macro_ImprimirParaDebug(ANSI_COLOR_RED "No se pudo Desactivar al Nodo Origen\n" ANSI_COLOR_RESET);
		}

		//Des-Reservo el Bloque reservado
		iNumeroError = FS_LiberarBloque(bloqueReservadoID);
		if (iNumeroError < 0) {
			Macro_ImprimirParaDebug(ANSI_COLOR_RED "No se pudo Des-Reservar al Bloque Reservado\n" ANSI_COLOR_RESET);
		}

		return;
	}

	//Corroboramos que nos enviaron una confirmacion de que se copio correctamente, en caso de Error NO marco como No disponible al nodo destino, simplemente aviso al usuario que hubo un error.
	//FEATURE Estaria bueno que EL NODO ORIGEN me envie la causa del error, asi yo la puedo imprimir en pantalla
	if (!string_equals_ignore_case(confirmacion, "Confirmacion")) {

		printf(ANSI_COLOR_RED "No se pudo Copiar del Nodo Origen al Nodo Destino\n" ANSI_COLOR_RESET "Razon: <%s>\n\n", confirmacion);

		//Des-Reservo el Bloque reservado
		iNumeroError = FS_LiberarBloque(bloqueReservadoID);
		if (iNumeroError < 0) {
			Macro_ImprimirParaDebug(ANSI_COLOR_RED "No se pudo Des-Reservar al Bloque Reservado\n" ANSI_COLOR_RESET);
		}

		//Libero la memoria
		liberar_memoria_payload(confirmacion);

		//Como ya enviamos, cerramos el socket
		Sockets_cerrar_desconectar(socketConectadoAlNodo);

		return;
	}

	//Libero la memoria
	liberar_memoria_payload(confirmacion);

	//Como ya enviamos, cerramos el socket
	Sockets_cerrar_desconectar(socketConectadoAlNodo);

	//Modifico los Datos del Bloque Destino que Ya fue Copiado
	/*tipo_datos_bloques nodoDestinoBloqueDatos;
	iNumeroError = BLOQUES_Consultar(bloqueReservadoID, &nodoDestinoBloqueDatos);
	if (iNumeroError < 0) {
		printf(ANSI_COLOR_RED "No se pudo los Datos del Bloque en el Nodo Destino\n\n" ANSI_COLOR_RESET);
		return;
	}
	*/

	//Ahora actualizo en la Base de Datos al Bloque Copiado
	// Esta funcion asigna los valore restantes despues de reservado el bloque
	iNumeroError = FS_GuardarBloque(bloqueReservadoID, nodoOrigenBloqueDatos.tamano, nodoOrigenBloqueDatos.md5);
	//nodoDestinoBloqueDatos.numeroBloqueDentroArchivo = nodoOrigenBloqueDatos.numeroBloqueDentroArchivo;
	//iNumeroError = BLOQUES_Modificar(nodoDestinoBloqueID, nodoDestinoBloqueDatos);
	//Chequeo Errores
	if (iNumeroError < 0) {
		printf(
				ANSI_COLOR_RED "Hubo un error al actualizar en la Base de Datos el Bloque que Fue Correctamente Copiado al Nodo Destino\n\n" ANSI_COLOR_RESET);
		return;
	}

	//A estas alturas se envio correctamente la orden y se copio el Bloque
	printf(ANSI_COLOR_GREEN "Se copio correctamente el Bloque\n\n" ANSI_COLOR_RESET);

	return;
}


void console_borrarBloqueNodo(char* nodoNombre, char* numeroBloque) {
	//Variable para Manejar Errores, por defecto es 0 que significa sin errores
	int iNumeroError = 0;

	//Verifico que los Strings no esten Vacios
	if (string_is_empty(nodoNombre) || string_is_empty(numeroBloque)) {
		printf(ANSI_COLOR_RED "Por favor, ingrese cel nombre del Bloque y el Numero de Bloque" ANSI_COLOR_RESET "\n");
		printf(ANSI_COLOR_GREEN "Ingrese \"Ayuda\" para una lista de comandos\n\n" ANSI_COLOR_RESET);
		return;
	}

	//Valido que Exista el Nodo
	tipo_id nodoID;
	int cantidadNodosConMismoNombre = -1;
	iNumeroError = NODO_BuscarNombre(nodoNombre, &nodoID, DB_SET, &cantidadNodosConMismoNombre);
	//Chequeo Errores
	if (iNumeroError < 0) {
		printf(ANSI_COLOR_RED "No existe el Nodo\n\n" ANSI_COLOR_RESET);
		return;
	}

	//Obtengo los Datos del Nodo
	tipo_datos_nodo nodoDatos;
	iNumeroError = NODO_Consultar(nodoID, &nodoDatos);
	//Chequeo Errores
	if (iNumeroError < 0) {
		printf(
		ANSI_COLOR_RED "Esto es raro, si bien existe el Nodo, no se pueden obtener sus Datos\n\n" ANSI_COLOR_RESET);
		printf("Posibles Causas:\n\t- La tabla de Bloques de Nodos esta Corrupta \n\n");
		return;
	}

	//Valido que Contenga al Bloque
	if (nodoDatos.bloques_totales <= atol(numeroBloque)) {
		printf(
				ANSI_COLOR_RED "No existe ese Numero de Bloque en el Nodo, solo llega hasta los %d Bloques y vos pediste el Bloque %d\n\n" ANSI_COLOR_RESET,
				nodoDatos.bloques_totales, (int) atol(numeroBloque));
		return;
	}

	//Obtengo el ID del Bloque
	tipo_id bloqueID = -1;
	iNumeroError = FS_ObtenerBloqueNodo(nodoID, atol(numeroBloque), &bloqueID);
	//Chequeo Errores
	if (iNumeroError < 0) {
		printf(ANSI_COLOR_RED "No se pudo obtener el ID del Bloque a Eliminar\n\n" ANSI_COLOR_RESET);
		printf("Posibles Causas:\n\t- La tabla de Bloques de Nodos esta Corrupta \n\n");
		return;
	}

	tipo_datos_bloques datosBloque;
	BLOQUES_Consultar(bloqueID, &datosBloque);
	datosBloque.disponible = false;
	BLOQUES_Modificar(bloqueID, datosBloque);

	// Verifico si con el bloque a eliminar el archivo aun esta disponible
	iNumeroError = FS_DesActivarArchivo(bloqueID);
	if (iNumeroError < 0) {
		printf(ANSI_COLOR_RED "No se pudo Verificar si el archivo queda Desactivado\n\n" ANSI_COLOR_RESET);
		return;
	}

	//Elimino al Bloque
	iNumeroError = BLOQUES_Eliminar(bloqueID);
	//Chequeo Errores
	if (iNumeroError < 0) {
		printf(ANSI_COLOR_RED "No se pudo Eliminar al Bloque\n\n" ANSI_COLOR_RESET);
		return;
	}

	bitarray_clean_bit(nodoDatos.tBloquesNodo,  atol(numeroBloque));
	nodoDatos.bloques_libres++;
	nodoDatos.bloques_usados--;

	// Actualizo los datos del espacio en el Nodo
	iNumeroError = NODO_Modificar(nodoID, nodoDatos);
	if (iNumeroError < 0) {
		printf(ANSI_COLOR_RED "No se pudieron actualizar los datos del Nodo\n\n" ANSI_COLOR_RESET);
		return;
	}


	//Llegados a este punto, se Elimino correctamente el Bloque, asi que lo imprimo por pantalla.
	printf( ANSI_COLOR_GREEN "Se Elimino con Exito el Bloque del Nodo.\n\n" ANSI_COLOR_RESET);
	return;
}



void console_agregarNodo(char* nodoNombre) {
	//Variable para Manejar Errores, por defecto es 0 que significa sin errores
	int iNumeroError = 0;

	if (!strlen(nodoNombre) > 0) {
		printf( ANSI_COLOR_RED "Por favor, ingrese el nombre del nodo\n" ANSI_COLOR_RESET);
		printf(ANSI_COLOR_GREEN "Ingrese \"Ayuda\" para una lista de comandos\n\n" ANSI_COLOR_RESET);
		return;
	}

	//Primero Valido si hay algun Nodo con ese nombre
	tipo_id nodoID;
	int cantidadNodosConMismoNombre = -1;
	iNumeroError = NODO_BuscarNombre(nodoNombre, &nodoID, DB_SET, &cantidadNodosConMismoNombre);
	//Chequeo Errores
	if (iNumeroError < 0) {
		printf( ANSI_COLOR_RED "No existe un Nodo Con este Nombre\n\n" ANSI_COLOR_RESET);
		return;
	}

	//Busco los Datos del Nodo
	tipo_datos_nodo nodoDatos;
	iNumeroError = NODO_Consultar(nodoID, &nodoDatos);
	//Chequeo Errores
	if (iNumeroError < 0) {
		printf( ANSI_COLOR_RED "Esto es Raro, si bien Existe el Nodo no puedo Obtener sus Datos\n\n" ANSI_COLOR_RESET);
		printf("Posibles Causas:\n\t- La tabla de Bloques de Nodos esta Corrupta \n\n");
		return;
	}

	//Verifico que no esta YA Disponible
	if (nodoDatos.activo == true) {
		printf(ANSI_COLOR_GREEN "El Nodo Ya estaba Agregado (Disponible)\n\n" ANSI_COLOR_RESET);
		return;
	}

	//Marco Como Disponible Al Nodo
	iNumeroError = FS_ActivarNodo(nodoNombre);
	//Chequeo Errores
	if (iNumeroError < 0) {
		printf( ANSI_COLOR_RED "No Pudo Agregarse (Marcar como Disponible) el Nodo\n\n" ANSI_COLOR_RESET);
		return;
	}

	//Llegados a este punto, se Marco Como Disponible Al Nodo correctamente , asi que lo imprimo por pantalla.
	printf(ANSI_COLOR_GREEN "Se Agrego (Marco como Disponible) con Exito al Nodo '%s' dentro del File System.\n\n" ANSI_COLOR_RESET, nodoDatos.sNombre);
	return;
}

void console_eliminarNodo(char* nodoNombre) {
	//Variable para Manejar Errores, por defecto es 0 que significa sin errores
	int iNumeroError = 0;

	if (!strlen(nodoNombre) > 0) {
		printf( ANSI_COLOR_RED "Por favor, ingrese el nombre del nodo" ANSI_COLOR_RESET "\n");
		printf(ANSI_COLOR_GREEN "Ingrese \"Ayuda\" para una lista de comandos\n\n" ANSI_COLOR_RESET);
		return;
	}

	//Primero Valido si hay algun Nodo con ese nombre
	tipo_id nodoID;
	int cantidadNodosConMismoNombre = -1;
	iNumeroError = NODO_BuscarNombre(nodoNombre, &nodoID, DB_SET, &cantidadNodosConMismoNombre);
	//Chequeo Errores
	if (iNumeroError < 0) {
		printf( ANSI_COLOR_RED "No existe un Nodo Con este Nombre\n\n" ANSI_COLOR_RESET);
		return;
	}

	//Busco los Datos del Nodo
	tipo_datos_nodo nodoDatos;
	iNumeroError = NODO_Consultar(nodoID, &nodoDatos);
	//Chequeo Errores
	if (iNumeroError < 0) {
		printf( ANSI_COLOR_RED "Esto es Raro, si bien Existe el Nodo no puedo Obtener sus Datos\n\n" ANSI_COLOR_RESET);
		printf("Posibles Causas:\n\t- La tabla de Bloques de Nodos esta Corrupta \n\n");
		return;
	}

	//Verifico que no esta YA Disponible
	if (nodoDatos.activo == false) {
		printf(ANSI_COLOR_GREEN "El Nodo Ya estaba Eliminado (NO Disponible)\n\n" ANSI_COLOR_RESET);
		return;
	}


	//Marco Como NO Disponible Al Nodo
	iNumeroError = FS_DesactivarNodo(nodoNombre);
	//Chequeo Errores
	if (iNumeroError < 0) {
		printf( ANSI_COLOR_RED "No se Pudo Eliminar (Marcar como No Disponible) al Nodo\n\n" ANSI_COLOR_RESET);
		return;
	}

	//Llegados a este punto, se Marco Como NO Disponible Al Nodo correctamente , asi que lo imprimo por pantalla.
	printf(
			ANSI_COLOR_GREEN "Se Elimino (Marco como No Disponible) con Exito el Nodo dentro del File System.\n\n" ANSI_COLOR_RESET);
	return;
}




void console_listarContenido(char* rutaPadre) {
	//Variable para Manejar Errores, por defecto es 0 que significa sin errores
	int iNumeroError = 0;

	//Verifico que los Strings no esten Vacios
	if (!strlen(rutaPadre) > 0 || !strlen(rutaPadre) > 0) {
		printf(ANSI_COLOR_RED "Por favor, ingrese la ruta de la carpeta" ANSI_COLOR_RESET "\n");
		printf(ANSI_COLOR_GREEN "Ingrese \"Ayuda\" para una lista de comandos\n\n" ANSI_COLOR_RESET);
		return;
	}

	//Valido que exista la Ruta al Directorio, y obtengo el ID del ultimo directorio de la Ruta
	int idPadreRuta = DIR_validarRuta(rutaPadre, 0);
	if (idPadreRuta == -1) {
		printf(ANSI_COLOR_RED "La ruta ingresada no es valida\n\n" ANSI_COLOR_RESET);
		return;
	}

	tipo_id found;
	tipo_datos_dir currentDir;

	int printedResults = 0; //esta variable la voy a usar para validar si la carpeta imprimio algo o no.

	//uso la variable firstSearch para buscar cuantos directorios hijos tiene el directorio padre, cuyo id esta representado por idRuta
	int firstSearch;
	iNumeroError = DIR_buscarIdPadre(idPadreRuta, &found, DB_SET, &firstSearch);
	if (firstSearch > 0) {
		int count;
		//Para cada Directorio hijo veo que exista
		for (count = 0; count < firstSearch; count++) {
			//No hace falta volver a validar para el primer Directorio hijo del Padre, ya que acabamos de buscar
			if (count != 0) {
				//Veo que el directorio hijo exista y obtengo el ID dado el nombre
				iNumeroError = DIR_buscarIdPadre(idPadreRuta, &found, DB_NEXT_DUP, NULL);
				//Chequeo Errores
				if (iNumeroError < 0) {
					printf(	"Error, No existe el Directorio de ID:<%i>. Esto es raro, ya que acabamos de validar y se suponia que existia..\n",
							idPadreRuta);
					printf("Posibles Causas:\n\t- La tabla de Carpetas (Directorios) esta Corrupta \n\n");
					return;
				}
			}

			//Traigo los Datos del directorio hijo encontrado
			iNumeroError = DIR_Consultar(found, &currentDir);
			//Chequeo Errores
			if (iNumeroError < 0) {
				printf(	"Error, No se pueden obtener los datos del Directorio de ID:<%i>. Esto es raro, ya que acabamos de validar y se suponia que existia..\n",
						idPadreRuta);
				printf("Posibles Causas:\n\t- La tabla de Carpetas (Directorios) esta Corrupta \n\n");
				return;
			}

			//valido que el padre sea el especificado antes de imprimirlo
			if (currentDir.iPadre == idPadreRuta) {
				printf(" - %s\n", currentDir.sNombre);
				printedResults++; // aumento por cada printf que hice,se usa al final de la funcion
			}
		}
	} else {
		printf("La carpeta No tiene Directorios Hijos\n");
	}

	printf("\nTermine de Listar los Directorios, ahora empiezo a Listar los Archivos...\n\n");

	tipo_datos_file currentFile; //la voy a usar para guardar un file si encuentro ids mas adelante

	//reuseo la variable firstSearch para buscar cuantos archivos hijos tiene el directorio padre, cuyo id esta representado por idRuta

	iNumeroError = FILE_BuscarPorPadre(idPadreRuta, &found, DB_SET, &firstSearch);
	if (firstSearch > 0) {
		int count; // indice para recorrer hasta llegar a la cantidad
		//Para cada Archivo hijo veo que exista
		for (count = 0; count < firstSearch; count++) {
			//No hace falta volver a validar para el primer Archivo hijo, ya que acabamos de buscar
			if (count != 0) {
				//Veo que el archivo exista
				iNumeroError = FILE_BuscarPorPadre(idPadreRuta, &found, DB_NEXT_DUP, NULL);
				//Chequeo Errores
				if (iNumeroError < 0) {
					printf("No se Pudo obtener cual es el Archivo %d dentro del directorio.\n", count);
					return;
				}
			}

			//Traigo los datos del archivo hijo encontrado
			iNumeroError = FILE_ConsultarId(found, &currentFile);
			//Chequeo Errores
			if (iNumeroError < 0) {
				printf("No se Pudo obtener los Datos del Archivo %d dentro del directorio.\n", count);
				return;
			}

			//valido que el padre sea el especificado antes de imprimirlo
			if (currentFile.dirPadre == idPadreRuta) {
				printf(" + %s\n", currentFile.sNombre);
				printedResults++; // aumento por cada printf que hice,se usa al final de la funcion
			}
		}
	} else {
		printf("La carpeta No tiene Archivos Hijos\n");
	}

	if (printedResults == 0) {
		printf("La carpeta esta vacia\n"); //si printed results es 0, implica que no imprimio nada, entonces muestra este mensaje
	}
	return;
}


void console_crearRuta(char* ruta) {
	if (!strlen(ruta) > 0 || !strlen(ruta) > 0) {
		printf(ANSI_COLOR_RED "Por favor, ingrese la ruta a crear" ANSI_COLOR_RESET "\n");
		printf(ANSI_COLOR_GREEN "Ingrese \"Ayuda\" para una lista de comandos\n\n" ANSI_COLOR_RESET);
		return;
	}

	//Divido la ruta, es necesario para despues recorrer carpeta a carpeta
	char** ruta_dividida = string_split(ruta, "/");
	//indice para recorrer el array de strings
	int indice_ruta = 0;

	//Ultimo padre encontrado, empieza en 0 (root)
	tipo_id lastFoundParent = 0;

	while (ruta_dividida[indice_ruta] != NULL) {
		//Busco si ya existe el directorio
		int currentId = DIR_validarRuta(ruta_dividida[indice_ruta], lastFoundParent);

		//Si no se encontro, entonces desde ese punto debe crearse la ruta
		if (currentId == -1) {
			//Verifico que la cantidad de carpetas a crear este disponible (carpetas totales en la ruta menos carpetas ya existentes de la ruta)
			int carpetasExistentes = DB_NumeroRegistros("dir"); //total de Directorios/Carpetas en el FS

			if (carpetasExistentes == DIR_CANTIDAD_MAXIMA) {
				printf(ANSI_COLOR_RED "No se puede crear la ruta desde %s, ya que se pasaria la cantidad maxima de directorios, para poder crear el resto de la ruta debe eliminar otros\n" ANSI_COLOR_RESET,ruta_dividida[indice_ruta]);
				return;
			}

			//idCreado es el id de la carpeta que se acaba de crear
			tipo_id idCreado;

			//Creo la carpeta, si es creado, esto devuelve el id de la nueva carpeta, que voy a usar a futuro para seguir creando
			//Si no puede, devuelve -1
			int created = DIR_CrearConId(ruta_dividida[indice_ruta], lastFoundParent, &idCreado);

			if (created >= 0) {
				if (idCreado <= 0) {
					printf(ANSI_COLOR_RED "ERROR, 'DIR_CrearConId' Devuelve que Creo bien pero El ID del Directorio Creado es Cualquier Cosa. ID: %d\n" ANSI_COLOR_RESET , idCreado);
					printf("Posibles Causas:\n\t- La tabla de Carpetas (Directorios) esta Corrupta \n\n");
					return;
				}
				//Guardo el ID de la carpeta que acabo de crear, asi las proximas carpetas se crean dentro de esta carpeta
				lastFoundParent = idCreado;
			} else {
				printf(ANSI_COLOR_RED "Hubo un problema al crear la carpeta %s, la seccion anterior ya existe o fue creada, por favor intentelo mas tarde\n" ANSI_COLOR_RESET,ruta_dividida[indice_ruta]);
				return;
			}
		} else {
			//Si se encontro, asigno el encontrado al last found
			lastFoundParent = currentId;
		}
		//Recorro para adelante el array de strings
		indice_ruta++;
	}
	//Si llego aca, es que pudo crear la ruta
	printf(ANSI_COLOR_GREEN "La ruta fue creada o ya existia en su totalidad\n\n" ANSI_COLOR_RESET);
	return;
}


void console_mostrarBloquesDeArchivo(char* archivoNombre, char* archivoRuta) {

	//Variable para Manejar Errores, inicializo en 0 que significa no hay errores.
	int iNumeroError = 0;
	//Variable para guardar el ID de Archivo
	tipo_id archivoID = 0;

	//Chequeo que  no sean Vacios los Strings
	if (!strlen(archivoNombre) > 0 || !strlen(archivoRuta) > 0) {
		printf(ANSI_COLOR_RED "Por favor, ingrese el nombre del archivo,y la carpeta de origen" ANSI_COLOR_RESET "\n");
		printf(ANSI_COLOR_GREEN "Ingrese \"Ayuda\" para una lista de comandos\n\n" ANSI_COLOR_RESET);
		return;
	}

	//Verifico Ruta Archivo y Obtengo su ID
	archivoID = FS_Validar_Archivo_Ruta(archivoNombre, archivoRuta);
	//Chequeo Errores
	if (archivoID < 0) {
		printf(ANSI_COLOR_RED "NO existe la Ruta o el archivo al cual se Quieren Mostrar sus Bloques\n\n" ANSI_COLOR_RESET);
		return;
	}

	//Ahora voy a pedir lista de nodos (en base al ID de archivo) e imprimir por pantalla

	//Voy a imprimir como si fuera una Tablita, primero imprimo "El Encabezado" de la tabla
	/*NOTA: Como Formateo tuve que hardcodear a mano los Numeros de las Constantes, asi que listo las constantes que se supone que representan esos numeros:
	 11 por la palabra "NumBloqArch"
	 5 por la palabra "Copia"
	 4 por la Palabra "Disp"
	 50 = NODO_LONGITUD_NOMBRE
	 17 = LONGITUD_CHAR_IP
	 6 = LONGITUD_CHAR_PUERTOS (le pongo 6 y no 5 porque la palabra "Puerto" es de 6
	 11 por la palabra "NumBloqNodo"
	 50 = FILE_LONGITUD_NOMBRE
	 */
	printf("\n %11s %5s %4s %50s %17s %6s %11s %50s\n", "NumBloqArch", "Copia", "Disp", "Nombre de Nodo", "IP del Nodo",
			"Puerto", "NumBloqNodo", "Archivo del Cual es Parte ese Bloque en la B.D. ");

	//Primero obtengo la cantidad de bloques, para hacer un FOR
	tipo_datos_file archivoDatos;
	iNumeroError = FILE_ConsultarId(archivoID, &archivoDatos);
	//Chequeo Errores
	if (iNumeroError < 0) {
		printf(ANSI_COLOR_RED "No se pudo obtener los Datos del archivo %s\n" ANSI_COLOR_RESET, archivoNombre);
		printf("Posibles Causas:\n\t- La tabla de Archivos esta Corrupta \n\n");
		return;
	}

	//Valido que tenga almenos 1 Bloque, sino doy error
	if (archivoDatos.iCantBloques < 1) {
		printf(ANSI_COLOR_RED "El archivo No tiene Bloques\n\n" ANSI_COLOR_RESET);
		return;
	}

	tipo_datos_file datosArchivoBloqueNodo;

	//Busco todos los Bloques del Archivo
	uint32_t bloqueActual;
	for (bloqueActual = 1; bloqueActual <= archivoDatos.iCantBloques; bloqueActual++) {

		//Obtengo la Cantidad de Copias de Este Bloque
		int bloqueCantidadCopias = 0;
		iNumeroError = FS_CantidadCopiasBloqueArchivo(archivoID, bloqueActual, &bloqueCantidadCopias);
		//Chequeo Errores
		if (iNumeroError < 0) {
			printf(
			ANSI_COLOR_RED "No se pudo obtener la Cantidad de Copias del archivo %s bloque %d\n" ANSI_COLOR_RESET,
					archivoNombre, (int) bloqueActual);
			//Salgo al Proximo Bloque
			continue;
		}

		//Valido que tenga almenos 1 Copias
		if (bloqueCantidadCopias < 1) {
			printf(ANSI_COLOR_RED "El bloque %d No tiene Copias\n" ANSI_COLOR_RESET, (int) bloqueActual);
			//Salgo al Proximo Bloque
			continue;
		}

		//Uso otro FOR para las copias de un mismo numero de bloque
		int copiaActual;
		for (copiaActual = 1; copiaActual <= bloqueCantidadCopias; copiaActual++) {

			//Obtengo los Datos, si hay error, imprimo el error en toda la linea con quien queria buscar y sigo con el siguiente Bloque (hago un continue)
			tipo_datos_bloques datosBloqueDentroNodo;
			iNumeroError = FS_ObtenerCopiaBloqueArchivo(archivoID, bloqueActual, copiaActual, &datosBloqueDentroNodo);
			if (iNumeroError < 0) {
				printf("    ERROR::[Bloque %d Copia %d] Queria Buscar Datos de la Copia del Bloque y dio Error. Lo mas probable es que se haya Borrado la Copia (El bloque que la contenia en el Nodo)\n", bloqueActual, copiaActual);
				continue;
			}

			//Obtengo Datos extras del Nodo en base a su ID
			tipo_datos_nodo datosNodo;
			iNumeroError = NODO_Consultar(datosBloqueDentroNodo.tIdNodo, &datosNodo);
			if (iNumeroError < 0) {
				printf("ERROR::[Bloque %d Copia %d] Queria Buscar Datos Extas del Nodo %d y dio Error\n", bloqueActual,
						copiaActual, datosBloqueDentroNodo.tIdNodo);
				continue;
			}

			//[Re UTIL] Le digo a la Base de Datos que me busque el nombre del archivo en base a la ID que estaba guardada en el BloqueNodo.
			char* nombreArchivoBloqueNodo = string_new();
			iNumeroError = FILE_ConsultarId(datosBloqueDentroNodo.tIdArchivo, &datosArchivoBloqueNodo);
			//Chequeo Errores
			if (iNumeroError < 0) {
				//En caso de Errores simplemente le asigno a la variable un Valor para que se pueda imprimir de todas Maneras
				string_append(&nombreArchivoBloqueNodo, "ERROR");
			} else {
				//Si no hay errores, asigno bien el nombre
				string_append(&nombreArchivoBloqueNodo, datosArchivoBloqueNodo.sNombre);
			}

			//Me FIjo si la Copia esta Disponible o No. Para Imprimirlo
			char* disponible = string_new();
			if (datosBloqueDentroNodo.disponible == true) {
				string_append(&disponible, "SI");
			} else {
				string_append(&disponible, "NO");
			}

			//Ahora Imprimo Formateados para Esta Copia sus Datos
			printf("\n %11d %5d %4s %50s %17s %6s %11d %50s \n", bloqueActual, copiaActual, disponible,
					datosNodo.sNombre, datosNodo.nodoIP, datosNodo.nodoPuerto, datosBloqueDentroNodo.numeroBloqueDentroNodo,
					nombreArchivoBloqueNodo);

			free(nombreArchivoBloqueNodo);
			free(disponible);
			//Voy a la siguiente Copia para el Numero de Bloque Actual
		}
		//Voy al Siguiente Bloque
	}
	//No hace falta hacer nada aca, excepto quizas un \n
	printf("Archivo: %s | Cant. Bloques: %i | Tamaño: %lu |Disponible: %s \n", datosArchivoBloqueNodo.sNombre, datosArchivoBloqueNodo.iCantBloques, datosArchivoBloqueNodo.tamanioArchivo, ((datosArchivoBloqueNodo.disponible == true) ? "SI" : "NO"));

	return;
}


void console_borrarBloqueDeArchivo(char* archivoNombre, char* archivoRuta, char* archivoNumeroBloque) {
	//Variable para Manejar Errores, inicializo en 0 que significa no hay errores.
	int iNumeroError = 0;
	//Variable para guardar el ID de Archivo
	tipo_id archivoID = 0;

	//Verifico que los argumentos no sean vacios
	if (!strlen(archivoNombre) > 0 || !strlen(archivoRuta) > 0 || !strlen(archivoNumeroBloque)) {
		printf(
				ANSI_COLOR_RED "Por favor, ingrese el nombre del archivo, la ruta del archivo y el numero de bloque, este ultimo entre \"s" ANSI_COLOR_RESET "\n");
		printf(ANSI_COLOR_GREEN "Ingrese \"Ayuda\" para una lista de comandos\n\n" ANSI_COLOR_RESET);
		return;
	}

	//Valido Ruta Archivo y obtengo su ID
	archivoID = FS_Validar_Archivo_Ruta(archivoNombre, archivoRuta);
	//Chequeo Errores
	if (archivoID < 0) {
		printf(ANSI_COLOR_RED "NO existe la Ruta o el archivo al cual se Quiere Borrar el Bloque\n\n" ANSI_COLOR_RESET);
		return;
	}

	iNumeroError = console_Validar_Archivo_Bloque(archivoNombre, atol(archivoNumeroBloque));
	//Chequeo Errores
	if (iNumeroError < 0) {
		printf( ANSI_COLOR_RED"No Existe el Bloque a Borrar" ANSI_COLOR_RESET "\n\n");
		return;
	}

	//Dado el Nombre del Archivo Busco las Copias de ese Bloque para Borrarlos de la Base de Datos (los pongo como Bloques Vacios en esos Nodos), esten en los Nodos que esten.
	iNumeroError = FS_EliminarBloqueArchivo(archivoID, atol(archivoNumeroBloque));
	//Chequeo Errores
	if (iNumeroError < 0) {
		printf(ANSI_COLOR_RED "Error al Borrar las Copias de los Bloques\n\n" ANSI_COLOR_RESET);
		return;
	}

	//A estas alturas se borro correctamente el Bloque
	printf(ANSI_COLOR_GREEN "Se borro correctamente el Bloque\n\n" ANSI_COLOR_RESET);
	return;
}




int console_recursive_delete(int idCarpetaPadrePorBorrar) {
	Macro_ImprimirParaDebug("Se ha llamado a Recursive Delete\n");

	//Variable para Manejar Errores, inicializo en 0 que significa no hay errores.
	int iNumeroError = 0;

	tipo_id idDirectorioEncontrado = -1;
	tipo_datos_dir datosDirectorioEncontrado;
	int cantidadDirectoriosEncontrados = 0;

	//Ahora busco los Sub-Directorios del Directorio Padre, para borrarlos, primero busco cuantos Directorios Hijos tiene
	iNumeroError = DIR_buscarIdPadre(idCarpetaPadrePorBorrar, &idDirectorioEncontrado, DB_SET,
			&cantidadDirectoriosEncontrados); //firstsearch es la cantidad encontrada, mientras que found es el primer encontrado
	Macro_CheckError(iNumeroError, "No se pudo buscar IdPadre");
	if (cantidadDirectoriosEncontrados > 0) {

		//Ahora voy a Recorrer y Eliminar Cada Directorio Hijo Encontrado
		int count;
		for (count = 0; count < cantidadDirectoriosEncontrados; count++) {
			//Si es el Primer Directorio, no hace falta buscar su ID porque lo acabamos de hacer.
			if (count != 0) {
				//Obtengo el ID de otro de los Directorios Hijos
				iNumeroError = DIR_buscarIdPadre(idCarpetaPadrePorBorrar, &idDirectorioEncontrado, DB_NEXT_DUP, NULL);
				Macro_CheckError(iNumeroError, "Error al Obtener ID de Directorio");
			}

			Macro_ImprimirParaDebug("ID antes de Dir Consultar: %d, Cantidad DE Directorios Encontrados: %d\n", idDirectorioEncontrado, cantidadDirectoriosEncontrados);

			//Obtengo los Datos del Directorio a Eliminar
			iNumeroError = DIR_Consultar(idDirectorioEncontrado, &datosDirectorioEncontrado);
			Macro_CheckError(iNumeroError, "Error al Obtener Datos de Directorio");

			//Antes de Eliminar el Directorio Hijo, valido que realmente tenga el Mismo Padre del cual quiero borrar, sino no no lo borro
			//En caso de que tenga el Padre que quiero, No lo elimino asi de una, sino que llamo recursivamente a esta Funcion para que borre los Directorios Hijos y Archivos Hijos que pueda tener este Directorio
			if (datosDirectorioEncontrado.iPadre == idCarpetaPadrePorBorrar) {
				int iNumeroErrorSubCarpeta = console_recursive_delete(idDirectorioEncontrado);

				//Chequeo Errores, si al intentar Eliminar Algun Directorio Hijo Dio Error, hago que se Pare Completamente (devolviendo Errores entre todos los Llamados Recursivos que se hallan Hecho)
				Macro_CheckError(iNumeroErrorSubCarpeta,
						"Error al Eliminar Recursivamente un Directorio Hijo que Verificamos pertenece al Padre que queremos eliminar");
			}
		}
	}else{
		Macro_ImprimirParaDebug("el Directorio de ID: %d no tiene Sub-Directorios para Borrar\n", idCarpetaPadrePorBorrar);
	}

	tipo_id idArchivoEncontrado = -1;
	tipo_datos_file datosArchivoEncontrado;

	//Ahora busco los Archivos del Directorio Padre, para borrarlos, primero busco cuantos Archivos Hijos tiene
	int cantidadArchivosEncontrados;
	iNumeroError = FILE_BuscarPorPadre(idCarpetaPadrePorBorrar, &idArchivoEncontrado, DB_SET,
			&cantidadArchivosEncontrados);
	if (cantidadArchivosEncontrados > 0) {

		//Ahora voy a Recorrer y Eliminar Cada Archivo Hijo Encontrado
		int count;
		for (count = 0; count < cantidadArchivosEncontrados; count++) {
			Macro_CheckError(iNumeroError, "Error al Obtener ID de Archivo");
			//Si es el Primer Archivo, no hace falta buscar su ID porque lo acabamos de hacer.
			if (count != 0) {
				//Obtengo el ID de otro de los Archivos Hijos
				iNumeroError = FILE_BuscarPorPadre(idCarpetaPadrePorBorrar, &idArchivoEncontrado, DB_NEXT_DUP, NULL);
				Macro_CheckError(iNumeroError, "Error al Obtener ID de Archivo");
			}

			//Obtengo los Datos del Archivo a Eliminar
			iNumeroError = FILE_ConsultarId(idArchivoEncontrado, &datosArchivoEncontrado);
			Macro_CheckError(iNumeroError, "Error al Obtener Datos de Archivo");

			//Antes de Eliminar el Archivo, valido que realmente tenga el Mismo Padre del cual quiero borrar, sino no no lo borro
			if (datosArchivoEncontrado.dirPadre == idCarpetaPadrePorBorrar) {
				iNumeroError = FS_EliminarArchivo(idArchivoEncontrado);
				Macro_CheckError(iNumeroError,
						"Error al Eliminar Archivo que Verificamos pertenece al Padre que queremos eliminar");
			}
		}
	}else{
		Macro_ImprimirParaDebug("el Directorio de ID: %d no tiene Archivos para Borrar\n", idCarpetaPadrePorBorrar);
	}

	//A esta altura, ya no tiene mas Directorios Hijos ni Archivos Hijos, asi que borro el Directorio Padre
	int numeroErrorBorradoPadre = DIR_EliminarConId(idCarpetaPadrePorBorrar);
	//Chequeo Errores, en caso de Error lo devuelvo (asi se termina arrastrando el Error recursivamente), sino devuelvo 1
	if (numeroErrorBorradoPadre < 0) {
		return numeroErrorBorradoPadre;
	} else {
		return 1;
	}
}

void console_listarNodos() {
	// TODO: Chequear errores
	tipo_datos_nodo t_datosDelNodo;
	//tipo_id t_IdNodo;
	int ultimoNodo, idActual, iReturn;
	DB_getUltimoId("nodo", &ultimoNodo);
	printf("%15s | %5s | %5s | %5s | %5s\n","NOMBRE","DISP","USAD","LIB","TOTAL");
	for (idActual = 1; idActual <= ultimoNodo; idActual++) {
		iReturn = NODO_Consultar(idActual,&t_datosDelNodo);
		if (iReturn == DB_NOTFOUND) {
			continue;
		} else if (iReturn >= 0) {
			printf("%15s | %5s | %5d | %5d | %5d\n",t_datosDelNodo.sNombre, ((t_datosDelNodo.activo == true) ? "SI" : "NO"), t_datosDelNodo.bloques_usados, t_datosDelNodo.bloques_libres, t_datosDelNodo.bloques_totales); 
		}
		
	}
}
int console_Validar_Archivo_Bloque(char* archivoNombre, uint32_t archivoNumeroBloque) {
	//Variable para Manejar Errores, inicializo en 0 que significa no hay errores.
	int iNumeroError = 0;
	//Variable para guardar el ID de Archivo
	tipo_id archivoID = 0;
	//Variable para Datos del Archivo
	tipo_datos_file archivoDatos;

	//Primero le digo a la Base de Datos que me busque el archivoNombre y me obtenga su ID.
	int cantidadArchivosConMismoNombre = -1;
	iNumeroError = FILE_BuscarNombre(archivoNombre, &archivoID, DB_SET, &cantidadArchivosConMismoNombre);

	//Chequeo Errores
	if (iNumeroError < 0) {
		Macro_ImprimirParaDebug(ANSI_COLOR_RED "No existe el archivo indicado\n" ANSI_COLOR_RESET);
		return -1;
	}

	//Segundo, obtengo el Directorio Padre Real que tiene el archivo
	iNumeroError = FILE_ConsultarId(archivoID, &archivoDatos);
	//Chequeo Errores
	if (iNumeroError < 0) {
		Macro_ImprimirParaDebug(ANSI_COLOR_RED "Esto es Raro, si bien existe el archivo no se pueden obtener sus Datos\n" ANSI_COLOR_RESET);
		return -2;
	}

	//Tercero y final, verifico que el numero de bloque para ese archivo exista (sea menor o igual al total de bloques)
	if (archivoNumeroBloque > archivoDatos.iCantBloques) {
		Macro_ImprimirParaDebug("Me pediste que Trabaje sobre el Bloque %d, pero el Archivo solo tiene %u Bloques\n",archivoNumeroBloque, archivoDatos.iCantBloques);
		return -3;
	}
	//Si llegamos hasta aca es que anda bien, devuelvo 1
	return 1;
}


tipo_bloque* console_PedirFSBloqueArchivo(char* archivoNombre, uint32_t archivoNumeroBloque){
	//Variable para Manejar Errores, por defecto es 0 que significa sin Errores
	int iNumeroError = 0;

	//Obtengo el ID del archivo
	tipo_id archivoID;
	int cantidadArchivosConMismoNombre = -1;
	iNumeroError = FILE_BuscarNombre(archivoNombre , &archivoID , DB_SET, &cantidadArchivosConMismoNombre);
	//Chequeo Errores
	if (iNumeroError < 0) {
		Macro_ImprimirParaDebug(ANSI_COLOR_RED "No se pudo obtener el ID del archivo %s\n" ANSI_COLOR_RESET, archivoNombre);
		return NULL;
	}

	//Obtengo cantidad de Copias del Bloque
	int numeroCopiasBloque = 0;
	iNumeroError = FS_CantidadCopiasBloqueArchivo(archivoID, archivoNumeroBloque, &numeroCopiasBloque);
	if (iNumeroError < 0) {
		Macro_ImprimirParaDebug(ANSI_COLOR_RED "No se pudo obtener la Cantidad de Copias que lleva hasta el Momento el Archivo %s\n" ANSI_COLOR_RESET, archivoNombre);
		return NULL;
	}

	char mensajebloque[40];
	sprintf(mensajebloque,"Descargando Bloque %d del Archivo",archivoNumeroBloque);
	Comun_ImprimirMensajeConBarras(mensajebloque);
	printf("\n");
	//Uso un For para recorrer las Copias
	int copiaActual;
	for(copiaActual=1;copiaActual<=numeroCopiasBloque;copiaActual++){

		//Obtengo el los Datos de la Copia del Bloque
		tipo_datos_bloques datosBloqueDentroNodo;
		iNumeroError = FS_ObtenerCopiaBloqueArchivo(archivoID,archivoNumeroBloque,copiaActual,&datosBloqueDentroNodo);
		if (iNumeroError < 0) {
			Macro_ImprimirParaDebug("ERROR::[Bloque %d Copia %d] Queria Buscar Datos de la Copia del Bloque y dio Error\n" , archivoNumeroBloque , copiaActual);

			//En caso de Error, salto a la proxima copia
			continue;
		}


		//Valido que este Disponible la Copia, sino salto a la Siguiente Copia
		if(datosBloqueDentroNodo.disponible==false){
			continue;
		}

		char *md5bloque;
		tipo_bloque* bloquePorDevolver = console_PedirNodoBloque(datosBloqueDentroNodo.tIdNodo,datosBloqueDentroNodo.numeroBloqueDentroNodo, datosBloqueDentroNodo.tamano);
//		tipo_bloque* bloquePorDevolver = console_PedirNodoBloque(datosBloqueDentroNodo.tIdNodo,datosBloqueDentroNodo.numeroBloqueDentroNodo);
		if (bloquePorDevolver==NULL) {
			//En caso de Error, salto a la proxima copia
			continue;
		}else{
			md5bloque = Comun_obtener_MD5_Bloque(bloquePorDevolver,true);
			printf("\n");
			if (strcmp(md5bloque,datosBloqueDentroNodo.md5) == 0) {
				return bloquePorDevolver;
			} else {
				Bloques_destruir(bloquePorDevolver);
				printf("Error en el MD5 del Bloque recibido\n");
				printf("El MD5 recibido es: %s y deberia ser: %s\n",md5bloque,datosBloqueDentroNodo.md5);
			}
		}
	}
	//Si llego aca es que Revisamos todas las Copias y Ninguna estaba Disponible (o hubo errores en cada una al tratar de obtener el Bloque), por eso devuelvo NULL como error
	printf(ANSI_COLOR_RED "No se Pudo Obtener Ninguna Copia del Bloque\n\n" ANSI_COLOR_RESET);
	return NULL;
}


tipo_bloque* console_PedirNodoBloque(tipo_id nodoID, uint32_t nodoNumeroBloque, size_t sizeArchivoDentroDelBloque){
	//Variable para Manejar Errores, por defecto es 0 que significa sin Errores
	int iNumeroError = 0;

	//Obtengo Datos del Nodo
	tipo_datos_nodo nodoDatos;
	iNumeroError = NODO_Consultar(nodoID , &nodoDatos);
	if (iNumeroError < 0) {
		Macro_ImprimirParaDebug("ERROR:: Queria Buscar Datos Extas del Nodo de ID %d y dio Error\n" , nodoID);
		return NULL;
	}

	//Me Conecto al Nodo
	tipo_socket* socketConectadoAlNodo = Sockets_conectar_servidor(nodoDatos.nodoIP , nodoDatos.nodoPuerto);
	//Controlo Errores
	if (socketConectadoAlNodo == NULL) {
		//El Nodo esta caido, hay que marcarlo como No Disponible en la Base de Datos
		Macro_ImprimirParaDebug(ANSI_COLOR_RED "No se pudo conectar al Nodo\n" ANSI_COLOR_RESET);

		//El NODO esta caido, hay que marcarlo como No Disponible en la Base de Datos
		iNumeroError = FS_DesactivarNodo(nodoDatos.sNombre);
		if (iNumeroError < 0) {
			Macro_ImprimirParaDebug(ANSI_COLOR_RED "Error: No se pudo Desactivar al Nodo\n" ANSI_COLOR_RESET);
		}
		return NULL;
	}

	//Primero Armo Orden para Pedirle el Bloque
	//Convierto el numero del Bloque dentro del Nodo a Char
	//char* numeroBloque = string_itoa(nodoNumeroBloque);
	uint32_t tamanioSerializado;
	t_datos_orden_leer_bloque datosDelBloqueALeer;
	datosDelBloqueALeer.nodoDestinoNumeroBloque = nodoNumeroBloque;
	datosDelBloqueALeer.tamanioBloque = sizeArchivoDentroDelBloque;
	char* BloqueALeerSerializado = serializar_orden_leer_bloque(datosDelBloqueALeer, &tamanioSerializado);

	char* ordenSerializada = package_create(BloqueALeerSerializado , tamanioSerializado , "pedirBloque" , FILESYSTEM);

	free(BloqueALeerSerializado);

	//Segundo Mando la Orden
	iNumeroError = Sockets_enviar_datos(socketConectadoAlNodo, ordenSerializada);
	if (iNumeroError <= 0) {
		Macro_ImprimirParaDebug(ANSI_COLOR_RED "Se cayo el Nodo  al enviar la Orden de Pedir Bloque\n" ANSI_COLOR_RESET);

		//El NODO esta caido, hay que marcarlo como No Disponible en la Base de Datos
		iNumeroError = FS_DesactivarNodo(nodoDatos.sNombre);
		if (iNumeroError < 0) {
			Macro_ImprimirParaDebug(ANSI_COLOR_RED "Error: No se pudo Desactivar al Nodo\n" ANSI_COLOR_RESET);
		}
		return NULL;
	}

	//Libero memoria porque ya no se usa mas
	package_destroy(ordenSerializada);

	//Esperamos que envie el Bloque
	t_header headerRecibido;
	iNumeroError = Sockets_recibir_Header(socketConectadoAlNodo, &headerRecibido);
	if (iNumeroError <= 0) {
		//Se cayo el Nodo a medio copiar el bloque
		printf(ANSI_COLOR_RED "Se cayo el Nodo a medio copiar el bloque\nSe cancela el comando\n\n" ANSI_COLOR_RESET);

		//El NODO esta caido, hay que marcarlo como No Disponible en la Base de Datos
		iNumeroError = FS_DesactivarNodo(nodoDatos.sNombre);
		if (iNumeroError < 0) {
			Macro_ImprimirParaDebug(ANSI_COLOR_RED "Error: No se pudo Desactivar al Nodo\n" ANSI_COLOR_RESET);
		}
		return NULL;
	}

	//Obtengo el Payload, que contiene al Bloque
	char* bloqueSerializado;
	iNumeroError = Sockets_recibir_Datos(socketConectadoAlNodo, &bloqueSerializado, headerRecibido);
	if (iNumeroError <= 0) {
		//Se cayo el Nodo a medio copiar el bloque
		printf(ANSI_COLOR_RED "Se cayo el Nodo a medio copiar el bloque\nSe cancela el comando\n\n" ANSI_COLOR_RESET);

		//El NODO esta caido, hay que marcarlo como No Disponible en la Base de Datos
		iNumeroError = FS_DesactivarNodo(nodoDatos.sNombre);
		if (iNumeroError < 0) {
			Macro_ImprimirParaDebug(ANSI_COLOR_RED "Error: No se pudo Desactivar al Nodo\n" ANSI_COLOR_RESET);
		}
		return NULL;
	}

	//Des-Serializo al Bloque
	tipo_bloque* bloquePorDevolver = Bloques_des_serializar(bloqueSerializado);

	//Libero la memoria del Payload porque ya no la necesito mas (son 20mb)
	liberar_memoria_payload(bloqueSerializado);

	//Como llegue a Obtener el Bloque que Queria, lo devuelvo
	return bloquePorDevolver;
}
