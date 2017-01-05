#include "../headers/nododb.h"
#include "../../Biblioteca_Comun/Biblioteca_Comun.h"

//#include <unistd.h>
#include <stdlib.h>
//#include <dirent.h>
//#include <sys/stat.h>
#include <sys/types.h>
//#include <stdio.h>
//#include <db.h>
#include <string.h>


//Funciones Privadas que sirven para Ordenar Indices, son "Comparadores"
static int NODO_idxBuscarNombre(DB *psdb, const DBT *pkey, const DBT *pdata, DBT *skey);
static int NODO_idxBuscarEspacio(DB *psdb, const DBT *pkey, const DBT *pdata, DBT *skey);


int NODO_Modificar(tipo_id tIdNodo, tipo_datos_nodo tNodo) {
	int iReturn = 0;

	iReturn = DB_Modificar("nodo",&tNodo,sizeof(tNodo),tIdNodo);
	Macro_CheckError(iReturn,"Error al modificar datos del Nodo");

	return iReturn;
}

int NODO_Consultar(tipo_id tId, tipo_datos_nodo* tNodo) {
	/* Crea las variables locales */
	DBT dbtDatos;
	memset(&dbtDatos,0,sizeof(DBT));
	int iReturn = 0;

	/* Consulta en la tabla "dir" por el Id y devuelve la estructura*/
	iReturn = DB_Consultar("nodo", tId, &dbtDatos);
	if (iReturn != DB_NOTFOUND) {
		Macro_CheckError(iReturn, "Error al consultar Nodo Id");
	} else if (iReturn == DB_NOTFOUND) {
		return iReturn;
	}

	/* Convierte los datos a la estructura de tipo_datos_dir */
	if (dbtDatos.data != NULL) {
		*tNodo = *((tipo_datos_nodo*) dbtDatos.data);
	} else {
		iReturn = -1;
		Macro_CheckError(iReturn,"Error al obtener datos del Nodo");
	}

	return iReturn;
}

int NODO_Nuevo(char sNodo[NODO_LONGITUD_NOMBRE], char* nodoIP, char* nodoPuerto, int iBloquesTot) {
	/* Crea las variables locales */
	tipo_datos_nodo tNodo;
	tipo_id tIdNodo;
	int iTamano;
	int iReturn = 0;

	/* Copia y asigna los datos a la estructura */
	memset(tNodo.sNombre, 0, NODO_LONGITUD_NOMBRE);
	memcpy(tNodo.sNombre, sNodo, strlen(sNodo)+1);
	memcpy(tNodo.nodoIP, nodoIP, strlen(nodoIP)+1);
	memcpy(tNodo.nodoPuerto, nodoPuerto, strlen(nodoPuerto)+1);
	tNodo.activo = false;
	tNodo.bloques_totales = iBloquesTot;
	tNodo.bloques_usados = 0;
	tNodo.bloques_libres = tNodo.bloques_totales - tNodo.bloques_usados;
	tNodo.bloques_reservados = 0;

	//Inicializo el BitArray todos con 0, asi no contiene Basura
	//tNodo.tBloquesNodo = NULL;
	char *cBloquesNodo;
	int iSizeNodo;
	iSizeNodo = (tNodo.bloques_totales / 8) + 1;
	cBloquesNodo = malloc(sizeof(char)*iSizeNodo);
	memset(cBloquesNodo,0,sizeof(char)*iSizeNodo);

	//tNodo.tBloquesNodo = bitarray_create(cBloquesNodo,tNodo.bloques_totales);
	tNodo.tBloquesNodo = bitarray_create(cBloquesNodo,sizeof(char)*iSizeNodo);

	// Obtiene el tamaño de la estructura
	iTamano = sizeof(tNodo);

	// Inserta los datos en la tabla
	iReturn = DB_Insertar("nodo", &tNodo, iTamano, &tIdNodo);
	Macro_CheckError(iReturn,"Error al insertar Nodo en BD");

	return iReturn;
}

int NODO_Eliminar(char sNodo[NODO_LONGITUD_NOMBRE]) {
	/* Busco el Id  y realizo el comando de eliminar */
	tipo_id tId;
	int iReturn = 0;

	iReturn = NODO_BuscarNombre(sNodo, &tId, 0, NULL);
	Macro_CheckError(iReturn,"Error al buscar Nombre del Nodo");

	iReturn = DB_Eliminar("nodo", tId);
	Macro_CheckError(iReturn,"Error al Eliminar el Nodo de la BD");

	return iReturn;
}

/*
 * Funcion interna que define los campos que se guardan en el indice
 */
static int NODO_idxBuscarNombre(DB *psdb, const DBT *pkey, const DBT *pdata, DBT *skey) {
	/* Defino el tipo de datos de tDir y casteo los datos de pdata
	 al tipo de datos de directorio */
	tipo_datos_nodo *tNodo;
	tNodo = ((tipo_datos_nodo *) pdata->data);

	/* Reservo la direccion de memoria */
	memset(skey, 0, sizeof(DBT));

	/* Asigno los valore que van a ser mi CLAVE de busqueda */
	skey->data = tNodo->sNombre;
	skey->size = strlen(tNodo->sNombre)+1;
	return (0);
}

/*
 * Funcion interna que define los campos que se guardan en el indice
 */
static int NODO_idxBuscarEspacio(DB *psdb, const DBT *pkey, const DBT *pdata, DBT *skey) {
	/* Defino el tipo de datos de tDir y casteo los datos de pdata
	 al tipo de datos de directorio */
	tipo_datos_nodo *tNodo;
	tNodo = ((tipo_datos_nodo *) pdata->data);
	/* Reservo la direccion de memoria */
	memset(skey, 0, sizeof(DBT));

	/* Asigno los valore que van a ser mi CLAVE de busqueda */
	skey->data = &tNodo->bloques_libres;
	skey->size = sizeof(tNodo->bloques_libres);
	return (0);
}

int NODO_AbrirIndices() {
	/* Llamo a a funcion CrearIndice, pasando como tabla que contiene
	 los datos "dir" y sTabla va a ser el Indice, y paso el puntero
	 a la funcion que va a usar el campo a indexar */
	int iReturn = 0;

	iReturn = DB_CrearIndice("nodo", "nodonombre", NODO_idxBuscarNombre);
	Macro_CheckError(iReturn,"Error al crear el Indice nodonombre");
	iReturn = DB_consultarCursor("nodonombre", NULL, NULL, 0,1, NULL, 0, NULL);
	Macro_CheckError(iReturn, "Error al abrir cursor nodonombre");
	iReturn = DB_CrearIndice("nodo", "nodoespacio", NODO_idxBuscarEspacio);
	Macro_CheckError(iReturn,"Error al crear el Indice nodoespacio");
	iReturn = DB_consultarCursor("nodoespacio", NULL, NULL, 0,1, NULL, 0, NULL);
	Macro_CheckError(iReturn, "Error al abrir el cursos nodoespacio");

	return iReturn;
}

int NODO_CerrarIndices() {
	int iReturn = 0;

	//Primero Debo Repasar Todos los Nodos que haya y Poner en NULL su BitArray porque ya no existira mas en memoria. Asi la proxima vez que Inicie el FS no hace referencia a memoria inexistente.
	tipo_id tIdUltimo, id;
	iReturn = DB_getUltimoId("nodo" , &tIdUltimo);
	tipo_datos_nodo datosDelNodo;
	Macro_CheckError(iReturn , "Error al obtener el ultimo Id de Nodo.\n");

	for (id = 1; id < tIdUltimo; id++) {
		//Obtengo los Datos del Nodo
		iReturn = NODO_Consultar(id , &datosDelNodo);

		//Si no pude obtener los Datos porque no lo encuentra, que siga de largo al proximo nodo (este nodo no se modifica)
		if (iReturn == DB_NOTFOUND) {
			continue;
		}
		Macro_CheckError(iReturn , "Error al consultar los Datos del Nodo.\n");

		datosDelNodo.tBloquesNodo = NULL;
		datosDelNodo.activo = false;

		//Actualizo el Nodo
		NODO_Modificar(id , datosDelNodo);
		Macro_CheckError(iReturn , "Error al actualizar los Datos del Nodo.\n");
	}

	//Ahora si Puedo Cerrar los Indices
	iReturn = DB_consultarCursor("nodonombre", NULL, NULL, 0,2, NULL, 0, NULL);
	Macro_CheckError(iReturn, "Error al cerrar el cursos nodonombre");
	iReturn = DB_consultarCursor("nodoespacio", NULL, NULL, 0, 2, NULL, 0, NULL);
	Macro_CheckError(iReturn, "Error al cerrar el cursos nodoespacio");

	return iReturn;
}

int NODO_BuscarNombre(char sNodo[NODO_LONGITUD_NOMBRE], tipo_id *tId, int iRegistro, int *iCantRegistros) {
	/* Verifico si que se pase un nombre de nodo posible */
	int iTamano = 0;
	int iReturn = 0;

	if (strlen(sNodo) == 0) {
		return -1;
	}

	iTamano = strlen(sNodo)+1;
	/* Obtengo la cantidad de registros, el Id del registro que correponde
	 al nodo que busco, y especifico como manejar los duplicados */
	//records = buscarIndice("nodonombre", sNodo, iTamano, tId, iRegistro);
	
	iReturn = DB_consultarCursor("nodonombre", tId, NULL, iRegistro, 0, sNodo, iTamano, iCantRegistros);
	Macro_CheckError(iReturn,"Error al ejecutar consulta en BD nodonombre");

	if(*tId==0){
		return -1;
	}

	return iReturn;
}

int NODO_BuscarEspacio(tipo_id *tId, int iIdAnt) {
	/* Verifico si que se pase un nombre de nodo posible */

	int iReturn = 0;
	DBT dbtDatos;
	tipo_id tIdBuscar;
	int EspacioLibre = 0;
	tipo_datos_nodo tNodo;
	//int ibloques_libres = 0;

	/* Obtengo la cantidad de registros, el Id del registro que correponde
	 al nodo que busco, y especifico como manejar los duplicados */
	// IMPORTANTE: En este caso, tIdBuscar me esta devolviendo el Espacio Libre,
	//iReturn = DB_consultarCursor("nodoespacio", &tIdBuscar, &dbtDatos, DB_LAST, 0, NULL, 0, NULL);
	iReturn = DB_consultarCursor("nodoespacio", &EspacioLibre, &dbtDatos, DB_LAST, 0, NULL, 0, NULL);
	Macro_CheckError(iReturn,"Error al ejecutar consulta en BD nodoespacio");

	while (iReturn >= 0) {
/*		if (ibloques_libres != tNodo.bloques_libres) {
			ibloques_libres = tNodo.bloques_libres;
			iIdAnt--;
		}
*/
/*		if (ibloques_libres != EspacioLibre) {
			ibloques_libres = EspacioLibre;
			iIdAnt--;
		}
*/
		tNodo = *((tipo_datos_nodo *)dbtDatos.data);
		if (tNodo.activo == true) {
			if (EspacioLibre > 0) {
				iIdAnt--;
			}
			if (iIdAnt == 0) {
				iReturn = NODO_BuscarNombre(tNodo.sNombre, &tIdBuscar, DB_SET,NULL);
				Macro_CheckError(iReturn, "Error al consultar el Id del Nodo");
				*tId = tIdBuscar;
				break;
			}
		}
		iReturn = DB_consultarCursor("nodoespacio", &EspacioLibre, &dbtDatos, DB_PREV, 0, NULL, 0, NULL);
		if (iReturn != DB_NOTFOUND) {
			Macro_CheckError(iReturn,"Error al ejecutar consulta en BD nodoespacio");
		}
	}

	return iReturn;
}
