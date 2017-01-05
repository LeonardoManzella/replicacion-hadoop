#include "../headers/dirdb.h"

#include <sys/types.h> //Para Portabilidad
#include <stdio.h>
//#include <db.h>
#include <string.h>
//#include <unistd.h>
//#include <stdlib.h>
//#include <dirent.h>
//#include <sys/stat.h>

#include <commons/string.h>

#include "../../Biblioteca_Comun/Biblioteca_Comun.h"

//Funciones Privadas que sirven para Ordenar Indices, son "Comparadores"
int DIR_idxBuscarNombre(DB *psdb, const DBT *pkey, const DBT *pdata, DBT *skey);
int DIR_idxBuscarIdPadre(DB *psdb, const DBT *pkey, const DBT *pdata, DBT *skey);

int DIR_Consultar(tipo_id tId, tipo_datos_dir* tDir) {
	/* Crea las variables locales */
	DBT dbtDatos;
	//Variable para Manejo de Errores, Inicia en 0 que significa No errores
	int iReturn = 0;

	/* Consulta en la tabla "dir" por el Id y devuelve la estructura*/
	iReturn = DB_Consultar("dir", tId, &dbtDatos);
	Macro_CheckError(iReturn,"Error en DB_Consultar");

	/* Convierte los datos a la estructura de tipo_datos_dir */
	//	*tDir = *((tipo_datos_dir*) dbtDatos.data);
	if (dbtDatos.data != NULL) {
		*tDir = *((tipo_datos_dir*) dbtDatos.data);
	} else {
		// No se encontraron datos
		iReturn = -1;
	}
	
	return iReturn;
}

int DIR_Crear(char sDir[DIR_LONGITUD_NOMBRE], char sPadre[DIR_LONGITUD_NOMBRE]) {
	/* Crea las variables locales */
	tipo_datos_dir tDir;
	tipo_id tId;
	int iTamano;
	int iReturn = 0;
	tipo_id tIdDir;

	/* Copia sDir a la estructura tDir*/
	memset(tDir.sNombre, 0, DIR_LONGITUD_NOMBRE);
	memcpy(tDir.sNombre, sDir, strlen(sDir)+1);

	/* Buscar el Id del Padre */
	iReturn = DIR_buscarId(sPadre, &tId, DB_SET, NULL);
	Macro_CheckError(iReturn, "Error al buscar Id Padre");

	/* Asigna el id del padre a tDir */
	tDir.iPadre = tId;

	//iTamano = sizeof(tDir.iPadre) + strlen(tDir.sNombre) + 1;
	iTamano = sizeof(tDir);
	/* Inserta el registro en la tabla "dir" pasando los datos de tDir
	 y su tamaño */
	iReturn = DB_Insertar("dir", &tDir, iTamano, &tIdDir);
	Macro_CheckError(iReturn,"Error al Insertar dir");

	return iReturn;
}

int DIR_CrearConId(char sDirName[DIR_LONGITUD_NOMBRE], int padre, tipo_id *tId) {
	int iReturn;
	tipo_datos_dir tDir;

	/* Copia sDir a la estructura tDir*/
	memset(&tDir.sNombre,0,sizeof(tDir.sNombre));
	memcpy(tDir.sNombre, sDirName, strlen(sDirName)+1);

	/* Asigna el id del padre a tDir */
	tDir.iPadre = padre;

	/* Inserta el registro en la tabla "dir" pasando los datos de tDir
	 y su tamaño */

	iReturn = DB_Insertar("dir", &tDir, sizeof(tDir), tId);
	Macro_CheckError(iReturn,"Error al insertar Dir");

	// Esto es para mantener compatibilidad con la funcion CrearId Original
//	if (*tId != NULL) {
//		iReturn = *tId;
//	}

	return iReturn;
}

int DIR_Renombrar(char sDirViejo[DIR_LONGITUD_NOMBRE], char sDirNuevo[DIR_LONGITUD_NOMBRE]) {
	/* Creo las variables locales */
	int iReturn;
	DBT dbtDatos;
	tipo_id tId;
	tipo_datos_dir tDir;

	/* Busco el Id del directorio original y devuelvo los datos y casteo */
	iReturn = DIR_buscarId(sDirViejo, &tId, DB_SET, NULL);
	Macro_CheckError(iReturn,"Error al buscar Id");

	iReturn = DB_Consultar("dir", tId, &dbtDatos);
	Macro_CheckError(iReturn,"Error al consultar datos de Dir");

	if (dbtDatos.data != NULL) {
		tDir = *((tipo_datos_dir*) dbtDatos.data);
	} else {
		iReturn = -1;
		Macro_CheckError(iReturn,"No se encontraron datos");
	}

	/* Cambio el nombre del Directorio y paso los datos para guardarlo */
	memcpy(tDir.sNombre, sDirNuevo, strlen(sDirNuevo)+1);
	iReturn = DB_Modificar("dir", &tDir, sizeof(tDir), tId);
	Macro_CheckError(iReturn,"Error al modificar los datos");

	return iReturn;
}

int DIR_RenombrarConId(tipo_id idCarpeta, char sDirNuevo[DIR_LONGITUD_NOMBRE]) {
	//Variable para Manejar Errores, inicializo en 0 que significa no hay errores.
	int iReturn = 0;

	/* Creo las variables locales */
	DBT dbtDatos;
	tipo_datos_dir tDir;

	/* Busco el Id del directorio original y devuelvo los datos y casteo */
	iReturn = DB_Consultar("dir", idCarpeta, &dbtDatos);
	Macro_CheckError(iReturn,"Error al consultar los datos");

	if (dbtDatos.data != NULL) {
		tDir = *((tipo_datos_dir*) dbtDatos.data);
	} else {
		iReturn = -1;
		Macro_CheckError(iReturn,"No se encontraron datos");
	}

	/* Cambio el nombre del Directorio y paso los datos para guardarlo */
	memcpy(tDir.sNombre, sDirNuevo, strlen(sDirNuevo)+1);


	iReturn = DB_Modificar("dir", &tDir, sizeof(tDir), idCarpeta);
	Macro_CheckError(iReturn,"Error al modificar los datos");

	return iReturn;

}

int DIR_MoverDir(char sDir[DIR_LONGITUD_NOMBRE], char sNuevoPadre[DIR_LONGITUD_NOMBRE]) {
	/* Creo las variables locales */
	DBT dbtDatos;
	tipo_id tId, tIdPadre;
	tipo_datos_dir tDir;
	int iReturn;

	/* Busco el Id del Directorio actual, y busco el Id del nuevo padre */
	iReturn = DIR_buscarId(sDir, &tId, DB_SET, NULL);
	Macro_CheckError(iReturn,"Error al buscar Id Dir");

	iReturn = DIR_buscarId(sNuevoPadre, &tIdPadre, DB_SET, NULL);
	Macro_CheckError(iReturn,"Error al buscar Id Padre");

	/* Busco los datos del directorio y lo casteo */
	iReturn = DB_Consultar("dir", tId, &dbtDatos);
	Macro_CheckError(iReturn, "Error al Consultar Dir");

	if (dbtDatos.data != NULL) {
		tDir = *((tipo_datos_dir*) dbtDatos.data);
	} else {
		iReturn = -1;
		Macro_CheckError(iReturn,"No se encontraron datos");
	}

	/* Cambio los datos del directorio y los guardo en la Tabla */
	tDir.iPadre = tIdPadre;

	iReturn = DB_Modificar("dir", &tDir, sizeof(tDir), tId);
	Macro_CheckError(iReturn,"Error al modificar datos Dir");

	return iReturn;
}

int DIR_MoverConId(tipo_id aMoverId, tipo_id detinoId) {
	//---------------Leo Modifico Aca----------------
	DBT dbtDatos;
	tipo_datos_dir tDir;
	//Variable para Manejar Errores, inicializo en 0 que significa no hay errores.
	int iReturn = 0;

	/* Busco los datos del directorio y lo casteo */
	iReturn = DB_Consultar("dir", aMoverId, &dbtDatos);
	Macro_CheckError(iReturn,"Error al consultar datos Dir");

	if (dbtDatos.data != NULL) {
		tDir = *((tipo_datos_dir*) dbtDatos.data);
	} else {
		iReturn = -1;
		Macro_CheckError(iReturn,"No se encontraron datos");
	}

	/* Cambio los datos del directorio y los guardo en la Tabla */
	tDir.iPadre = detinoId;

	iReturn = DB_Modificar("dir", &tDir, sizeof(tDir), aMoverId);
	Macro_CheckError(iReturn, "Error al modificar Dir");

	return iReturn;
	//---------------Termine de Modificar----------------
}

int DIR_EliminarConId(int id) {
	//---------------Leo Modifico Aca----------------
	//Variable para Manejar Errores, inicializo en 0 que significa no hay errores.
	int iReturn = 0;

	/* realizo el comando de eliminar en la tabla de dirs */
	tipo_id tId;
	tId = id;

	iReturn = DB_Eliminar("dir", tId);
	Macro_CheckError(iReturn,"Error al eliminar dir");

	return iReturn;

	//---------------Termine de Modificar----------------
}

int DIR_Eliminar(char sDir[DIR_LONGITUD_NOMBRE]) {
	/* Busco el Id del directorio y realizo el comando de eliminar */
	tipo_id tId;
	int iReturn = 0;

	iReturn = DIR_buscarId(sDir, &tId, DB_SET, NULL);
	Macro_CheckError(iReturn,"Error al buscar Id");

	iReturn = DB_Eliminar("dir", tId);
	Macro_CheckError(iReturn,"Error al eliminar dir");

	return iReturn;
}

/*
 * Funcion interna que define los campos que se guardan en el indice
 */
int DIR_idxBuscarNombre(DB *psdb, const DBT *pkey, const DBT *pdata, DBT *skey) {
	/* Defino el tipo de datos de tDir y casteo los datos de pdata
	 al tipo de datos de directorio */
	tipo_datos_dir *tDir;
	tDir = ((tipo_datos_dir *) pdata->data);

	/* Reservo la direccion de memoria */
	memset(skey, 0, sizeof(DBT));

	/* Asigno los valore que van a ser mi CLAVE de busqueda */
	skey->data = tDir->sNombre;
	//skey->size = sizeof(tDir->sNombre);
	skey->size = strlen(tDir->sNombre) + 1;
	return (0);
}

/*
 * Funcion interna que define los campos que se guardan en el indice
 */
int DIR_idxBuscarIdPadre(DB *psdb, const DBT *pkey, const DBT *pdata, DBT *skey) {
	/* Defino el tipo de datos de tDir y casteo los datos de pdata
	 al tipo de datos de directorio */
	tipo_datos_dir *tDir;
	tDir = ((tipo_datos_dir *) pdata->data);

	/* Reservo la direccion de memoria */
	memset(skey, 0, sizeof(DBT));

	/* Asigno los valore que van a ser mi CLAVE de busqueda */
	skey->data = &tDir->iPadre;
	//skey->size = sizeof(tDir->sNombre);
	skey->size = sizeof(tDir->iPadre);
	return (0);
}

int DIR_AbrirIndices() {
	/* Llamo a a funcion CrearIndice, pasando como tabla que contiene
	 los datos "dir" y sTabla va a ser el Indice, y paso el puntero
	 a la funcion que va a usar el campo a indexar */
	int iReturn = 0;
	
	iReturn = DB_CrearIndice("dir", "dirindice", DIR_idxBuscarNombre);
	Macro_CheckError(iReturn,"Error al crear dirindice");
	iReturn = DB_consultarCursor("dirindice", NULL, NULL, 0, 1, NULL, 0, NULL);
	Macro_CheckError(iReturn,"Error al abrir cursor dirindice");
	iReturn = DB_CrearIndice("dir", "indpadre", DIR_idxBuscarIdPadre);
	Macro_CheckError(iReturn,"Error al crear indpadre");
	iReturn = DB_consultarCursor("indpadre", NULL, NULL, 0, 1, NULL, 0, NULL);
	Macro_CheckError(iReturn,"Error al abrir cursor indpadre");
	
	return iReturn;
}

int DIR_CerrarIndices() {
	int iReturn = 0;
	
	iReturn = DB_consultarCursor("dirindice", NULL, NULL, 0, 2, NULL, 0, NULL);
	Macro_CheckError(iReturn,"Error al cerrar cursor dirindice");
	iReturn = DB_consultarCursor("indpadre", NULL, NULL, 0, 2, NULL, 0, NULL);
	Macro_CheckError(iReturn,"Error al cerrar cursos indpadre");

	return iReturn;
}

int DIR_buscarId(char sDir[DIR_LONGITUD_NOMBRE], tipo_id *tId, int iRegistro, int *iCantRegistros) {
	/* Verifico si el directorio no es el Padre o sea "" */
	int iTamano = 0;
	if (strlen(sDir) == 0) {
		*tId = 0;
		return (0);
	}
	int iReturn = 0;
	iTamano = strlen(sDir) + 1;
	/* Obtengo la cantidad de registros, el Id del registro que correponde
	 al directorio que busco, y especifico como manejar los duplicados */
	//records = buscarIndice("dirindice", sDir, iTamano, tId, iRegistro);
	iReturn = DB_consultarCursor("dirindice", tId, NULL, iRegistro, 0, sDir, iTamano, iCantRegistros);
	if (iReturn != DB_NOTFOUND) {
		Macro_CheckError(iReturn,"Error al consultar Datos");
	}

	if(*tId==0) {
		return -1;
	}
	return iReturn;
}

int DIR_buscarIdPadre(tipo_id tIdPadre, tipo_id *tId, int iRegistro, int *iCantRegistros) {
	int iTamano = 0;
	int iReturn = 0;
	iTamano = sizeof(tIdPadre);
	/* Obtengo la cantidad de registros, el Id del registro que correponde
	 al directorio que busco, y especifico como manejar los duplicados */
	iReturn = DB_consultarCursor("indpadre", tId, NULL, iRegistro, 0, &tIdPadre, iTamano, iCantRegistros);
	Macro_CheckError(iReturn,"Error al consultar Datos");

	return iReturn;
}


int DIR_validarRuta(char* ruta, int startID) {
	//Variable para Devolver Valor, Inicia en -1 para que por defecto rompa la validacion
	int returnId = -1;

	//Primero nos fijamos si no nos Pidio a Root "/" (que siempre es valido)
	if (!strcmp(ruta, "/")) {
		returnId = 0; //Root tiene ID 0
		return returnId;
	}

	//Si no nos pidio a Root, vemos si es valida la ruta
	//Primero dividimos la ruta en sus directorios que la componen
	// FIXME: memory leak
	char** rutaDividida = string_split(ruta, "/");
	int sizerutaDiv = 0;
	while (rutaDividida[sizerutaDiv]) {
		sizerutaDiv++;
	}
	int directorioActual = 0; //se usa parar recorrer dentro del splat, es el numero del directorio que estamos validando en el momento
	tipo_id padreAnterior = startID; //el padre de inicio, es algo asi como un id de offset desde donde empezar a validar, pero ojo, es ID, NO POSICION.
	tipo_id searchId = -1;
	int firstSearch = -1;
	int iReturn = 0;
	tipo_datos_dir datosDirActual;

	//Vamos a Validar Todos los Directorios de la Ruta
	//Se sale del while al terminarse de recorrer los directorios
	while (rutaDividida[directorioActual]) {
		//Primero buscamos si existe algun directorio con el nombre de mi directorio actual (pueden haber varios con el mismo nombre o ninguno)

		//iReturn = DIR_buscarId(rutaDividida[directorioActual], &searchId, DB_SET, &firstSearch);
		iReturn = DIR_buscarId(rutaDividida[directorioActual], &searchId, DB_SET, &firstSearch);
		if ((iReturn != -1) && (iReturn < 0)) {
			FS_LiberarMemoriaDobleArray( (void**)rutaDividida, sizerutaDiv);
			Macro_CheckError(iReturn, "Error al buscar Id");
		}

		//Aca devolvio la cantidad de Directorios encontrados que tienen el mismo nombre que el directorio que busco, o -1 si no habia ninguno con ese nombre, y en searchid esta el id del primero
		if (firstSearch > 0) {
			//Encontro Directorios, entonces vamos a validar que exista 1 que tenga de padre al ultimo directorio que valide

			int count; //count se usa para no pasarse de la cantidad "encontrada" de directorios con mismo nombre

			//Para cada Directorio con mismo Nombre veo que exista 1 que tenga de padre al ultimo directorio que valide
			for (count = 0; count < firstSearch; count++) {
				//Variable para Manejo de Errores, Inicia en 0 que significa No errores
				int iNUmeroError = 0;

				//salteo el primer buscarId en la primer consulta porque ya la hice con el buscar anterior (en "firstSearch"),
				//si no hago esto, la cabecera del cursor se saltea siempre al primero por el flag de DB_NEXT_DUP.
				if (count != 0) {
					//Veo que el proximo directorio de la ruta exista
					iReturn = DIR_buscarId(rutaDividida[directorioActual], &searchId, DB_NEXT_DUP, NULL);
					//Chequeo Errores
					if (iNUmeroError < 0) {
						Macro_ImprimirParaDebug("Hubo un Error Al Buscar un Directorio por ID en validatePath. Esto no deberia pasar\n");
						FS_LiberarMemoriaDobleArray((void**)rutaDividida, sizerutaDiv);
						return -1;
					}
				}

				//Consulto los Datos del Directorio
				iNUmeroError = DIR_Consultar(searchId, &datosDirActual);
				//Chequeo Errores
				if (iNUmeroError < 0) {
					Macro_ImprimirParaDebug("Hubo un Error Al Obtener Datos de un Directorio en validatePath\n");
					FS_LiberarMemoriaDobleArray((void**)rutaDividida, sizerutaDiv);
					return -1;
				}

				//Me aseguro que el padre de este Directorio sea el ultimo directorio que valide , si lo es, asigno a "padreAnterior", y
				//Si siguen habiendo Directorios por Validar en "rutaDividida", las buscare con este id de padre, ya que serian hijos de este Directorio
				if (datosDirActual.iPadre == padreAnterior) {
					padreAnterior = searchId;
					//Como encontre entre los Direcontorios con mismo nombre el que queria, salgo del For
					break;
				}

			}
			//Si llego al final del for es que no habia ningun directorio que tenga de padre al que buscabamos, debe devolver error
			if (count == firstSearch) {
				FS_LiberarMemoriaDobleArray((void**)rutaDividida, sizerutaDiv);
				return -1;
			}
		} else {
			//Si llegamos aca es que algun directorio de la Ruta no exite
			FS_LiberarMemoriaDobleArray((void**)rutaDividida, sizerutaDiv);
			return -1;
		}

		//aumento indice para seguir recorriendo los Directorios en "rutaDividida"
		directorioActual++;
	}

	//si el ultimo padre encontrado es distinto del Inicial, entonces retorno ese ultimo padre encontrado, sino, devuelve -1
	if (padreAnterior != startID) {
		returnId = (int) padreAnterior;
	} else {
		returnId = -1;
	}

	FS_LiberarMemoriaDobleArray((void**)rutaDividida, sizerutaDiv);

	return returnId;
}
