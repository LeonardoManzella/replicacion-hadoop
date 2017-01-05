#include "../headers/bloqdb.h"
#include "../../Biblioteca_Comun/Biblioteca_Comun.h"

#include <sys/types.h> //Para Compatibilidad
//#include <stdio.h>
//#include <db.h>
#include <string.h>
//#include <unistd.h>
#include <stdlib.h> // para malloc
//#include <dirent.h>
//#include <sys/stat.h>


//Funciones Privadas que sirven para Ordenar Indices, son "Comparadores"
static int BLOQUES_idxbuscarBloqueNodo(DB *psdb, const DBT *pkey, const DBT *pdata, DBT *skey);

static int BLOQUES_idxbuscarBloqueArchivo(DB *psdb, const DBT *pkey, const DBT *pdata, DBT *skey);

static int BLOQUES_idxbuscarBloqueDeArchivo(DB *psdb, const DBT *pkey, const DBT *pdata, DBT *skey);

int BLOQUES_Modificar(tipo_id tId, tipo_datos_bloques tBloque) {
	int iReturn = 0;

	iReturn = DB_Modificar("bloques", &tBloque,sizeof(tBloque),tId);
	Macro_CheckError(iReturn,"Error al modificar bloque en BD");

	return iReturn;
}


int BLOQUES_Consultar(tipo_id tId, tipo_datos_bloques* tBloque) {
	int iReturn = 0;
	/* Crea las variables locales */
	DBT dbtDatos;
	memset(&dbtDatos,0,sizeof(DBT));
	dbtDatos.flags = DB_DBT_MALLOC;


	/* Consulta en la tabla "dir" por el Id y devuelve la estructura*/
	iReturn = DB_Consultar("bloques", tId, &dbtDatos);
	if (iReturn != DB_NOTFOUND) {
		Macro_CheckError(iReturn,"Error al consultar BD");
	}
	/* Convierte los datos a la estructura de tipo_datos_dir */
	if (dbtDatos.data != 0) {
		*tBloque = *((tipo_datos_bloques*) dbtDatos.data);
	} else {
		iReturn = DB_NOTFOUND;
		//CheckError(iReturn,"Error al convertir datos");
	}

	return iReturn;
}

int BLOQUES_Nuevo(tipo_datos_bloques tBloque, tipo_id *tId) {
	int iReturn = 0;
	/* Crea las variables locales */
	tipo_id tIdBloque;
	int iTamano;

	iTamano = sizeof(tBloque);
	/* Inserta el registro en la tabla "dir" pasando los datos de tDir
	 y su tamaño */
	iReturn = DB_Insertar("bloques", &tBloque, iTamano, &tIdBloque);
	Macro_CheckError(iReturn,"Error al insertar Bloque en DB");

	if (tId != NULL) {
		*tId = tIdBloque;
	}

	return iReturn;
}


int BLOQUES_Eliminar(tipo_id tIdBloque) {
	int iReturn = 0;

	iReturn = DB_Eliminar("bloques", tIdBloque);
	Macro_CheckError(iReturn,"Error al eliminar Bloque en DB");


	return iReturn;
}

/*
 * Funcion interna que define los campos que se guardan en el indice
 */
static int BLOQUES_idxbuscarBloqueNodo(DB *psdb, const DBT *pkey, const DBT *pdata, DBT *skey) {
	/* Defino el tipo de datos de tDir y casteo los datos de pdata
	 al tipo de datos de directorio */
	tipo_datos_bloques *tBloque;
	tBloque = ((tipo_datos_bloques *) pdata->data);

	/* Reservo la direccion de memoria */
	memset(skey, 0, sizeof(DBT));

	/* Asigno los valore que van a ser mi CLAVE de busqueda */
	skey->data = &tBloque->tIdNodo;
	skey->size = sizeof(tBloque->tIdNodo);
	return (0);
}

/*
 * Funcion interna que define los campos que se guardan en el indice
 */
static int BLOQUES_idxbuscarBloqueArchivo(DB *psdb, const DBT *pkey, const DBT *pdata, DBT *skey) {
	/* Defino el tipo de datos de tDir y casteo los datos de pdata
	 al tipo de datos de directorio */
	tipo_datos_bloques *tBloque;
	tBloque = ((tipo_datos_bloques *) pdata->data);

	/* Reservo la direccion de memoria */
	memset(skey, 0, sizeof(DBT));

	/* Asigno los valore que van a ser mi CLAVE de busqueda */
	skey->data = &tBloque->tIdArchivo;
	skey->size = sizeof(tBloque->tIdArchivo);
	return (0);
}

/*
 * Funcion interna que define los campos que se guardan en el indice
 */
static int BLOQUES_idxbuscarBloqueDeArchivo(DB *psdb, const DBT *pkey, const DBT *pdata, DBT *skey) {
	/* Defino el tipo de datos de tDir y casteo los datos de pdata
	 al tipo de datos de directorio */
	tipo_datos_bloques *tBloque;
	tBloque = ((tipo_datos_bloques *) pdata->data);

	/* Reservo la direccion de memoria */
	memset(skey, 0, sizeof(DBT));


	/* Asigno los valore que van a ser mi CLAVE de busqueda */
	skey->data = &tBloque->numeroBloqueDentroArchivo;
	skey->size = sizeof(tBloque->numeroBloqueDentroArchivo);
	return (0);
}

int BLOQUES_AbrirIndices() {
	/* Llamo a a funcion CrearIndice, pasando como tabla que contiene
	 los datos "dir" y sTabla va a ser el Indice, y paso el puntero
	 a la funcion que va a usar el campo a indexar */
	int iReturn = 0;

	DB_CrearIndice("bloques", "bloquesnodo", BLOQUES_idxbuscarBloqueNodo);
    Macro_CheckError(iReturn,"Error al crear Indice");
	DB_consultarCursor("bloquesnodo", NULL, NULL, 0,1, NULL, 0, NULL);
    Macro_CheckError(iReturn,"Error al crear Cursor");
	DB_CrearIndice("bloques", "bloquesarch", BLOQUES_idxbuscarBloqueArchivo);
    Macro_CheckError(iReturn,"Error al crear Indice");
	DB_consultarCursor("bloquesarch", NULL, NULL, 0,1, NULL, 0, NULL);
    Macro_CheckError(iReturn,"Error al crear Cursor");
	DB_CrearIndice("bloques", "bloquesespecifico", BLOQUES_idxbuscarBloqueDeArchivo);
    Macro_CheckError(iReturn,"Error al crear Indice");
	DB_consultarCursor("bloquesespecifico", NULL, NULL, 0,1, NULL, 0, NULL);
    Macro_CheckError(iReturn,"Error al crear Cursor");

	return iReturn;
}

int BLOQUES_CerrarIndices() {
	int iReturn = 0;

	//Primero Debo Repasar Todos los Bloques que haya. Asi la proxima vez que Inicie el FS no hace referencia a memoria inexistente.
	tipo_id tIdUltimo, id;
	iReturn = DB_getUltimoId("bloques" , &tIdUltimo);
	tipo_datos_bloques datosDelBloque;
	Macro_CheckError(iReturn , "Error al obtener el ultimo Id de Bloque.\n");

	for (id = 1; id < tIdUltimo; id++) {
		//Obtengo los Datos del Bloque
		iReturn = BLOQUES_Consultar(id , &datosDelBloque);

		//Si no pude obtener los Datos porque no lo encuentra, que siga de largo al proximo bloque (este nodo no se modifica)
		if (iReturn == DB_NOTFOUND) {
			continue;
		}
		Macro_CheckError(iReturn , "Error al consultar los Datos del Bloque.\n");

		datosDelBloque.disponible = false;

		//Actualizo el Bloque
		BLOQUES_Modificar(id , datosDelBloque);
		Macro_CheckError(iReturn , "Error al actualizar los Datos del Bloque.\n");
	}

    iReturn = DB_consultarCursor("bloquesnodo", NULL, NULL, 0,2, NULL, 0, NULL);
    Macro_CheckError(iReturn,"Error al cerrar Cursor");
    iReturn = DB_consultarCursor("bloquesarch", NULL, NULL, 0,2, NULL, 0, NULL);
    Macro_CheckError(iReturn,"Error al cerrar Cursor");
    iReturn = DB_consultarCursor("bloquesespecifico", NULL, NULL, 0,2, NULL, 0, NULL);
    Macro_CheckError(iReturn,"Error al cerrar Cursor");

    return iReturn;
}

int BLOQUES_buscarBloqueNodo(tipo_id tIdNodo, tipo_id *tId, int iRegistro, int *iCantRegistros) {
	/* Verifico si el directorio no es el Padre o sea "" */
	int iTamano = 0;
	int iReturn = 0;

	iTamano = sizeof(tIdNodo);
	/* Obtengo la cantidad de registros, el Id del registro que correponde
	 al directorio que busco, y especifico como manejar los duplicados */
	//records = buscarIndice("bloquenodo", sBloque, iTamano, tId, iRegistro);
    iReturn = DB_consultarCursor("bloquesnodo", tId, NULL, iRegistro, 0, &tIdNodo, iTamano, iCantRegistros);
    if (iReturn != DB_NOTFOUND) {
    	Macro_CheckError(iReturn,"Error al consultar en DB");
    }

	if(*tId==0){
		return DB_NOTFOUND;
	}
	return iReturn;
}

int BLOQUES_buscarBloqueArchivo(tipo_id idArchivo, tipo_id* idBloqueBuscado, int iRegistro, int *iCantRegistros) {
	/* Verifico si el directorio no es el Padre o sea "" */
	int iTamano = 0;
	int iReturn = 0;
	iTamano = sizeof(idArchivo);
	/* Obtengo la cantidad de registros, el Id del registro que correponde
	 al directorio que busco, y especifico como manejar los duplicados */
	//records = buscarIndice("bloquenodo", sBloque, iTamano, tId, iRegistro);
    iReturn = DB_consultarCursor("bloquesarch", idBloqueBuscado, NULL, iRegistro, 0, &idArchivo, iTamano, iCantRegistros);
	if (iReturn != DB_NOTFOUND) {
		Macro_CheckError(iReturn,"Error al consultar en DB");
	}

	//Como no encontramos el Bloque, devuelvo DB_NOTFOUND
	if(*idBloqueBuscado==0){
		return DB_NOTFOUND;
	}
	return iReturn;
}

int BLOQUES_buscarBloqueEspecificoArchivo(tipo_id idArchivo, int numeroBloqueDentroArchivo, tipo_id* idBloqueBuscado, bool NuevaConsulta) {

	int iReturn = 0;
	tipo_id tIdBloqueArchivo;

	iReturn = DB_consultarDosIndicesCursor("bloques","bloquesarch","bloquesespecifico", idArchivo, numeroBloqueDentroArchivo, &tIdBloqueArchivo, NuevaConsulta);
	if ((iReturn != DB_NOTFOUND) && (iReturn < 0)) {
		Macro_CheckError(iReturn,"Error al realizar consulta sobre el Cursor");
	}

	if(tIdBloqueArchivo==0){
		return DB_NOTFOUND;
	}

	*idBloqueBuscado = tIdBloqueArchivo;

	return iReturn;
}

