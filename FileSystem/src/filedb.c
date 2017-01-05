#include "../headers/filedb.h"
#include "../../Biblioteca_Comun/Biblioteca_Comun.h"

#include <sys/types.h> //Para Portabilidad
#include <stdio.h>
//#include <db.h>
#include <string.h>
//#include <unistd.h>


//Funciones Privadas que sirven para Ordenar Indices, son "Comparadores"
static int FILE_idxBuscarNombre(DB *psdb, const DBT *pkey, const DBT *pdata, DBT *skey);
static int FILE_idxBuscarIdPadre(DB *psdb, const DBT *pkey, const DBT *pdata, DBT *skey);

int FILE_Crear(tipo_datos_file tArch, tipo_id *tId) {
	//tipo_id tIdArch;
	int iReturn = 0;
	iReturn = DB_Insertar("file", &tArch, sizeof(tArch), tId);
	Macro_CheckError(iReturn,"Error al insertar en file");
	if (*tId == 0){
		iReturn = -1;
		Macro_CheckError(iReturn,"No se pudo insertar el registro, Id = 0");
	}
	//*tId = iReturn;
	return iReturn;
}

int FILE_Renombrar(tipo_id idArchivoActual, char sNombreArchivoNuevo[FILE_LONGITUD_NOMBRE]) {
	int iReturn = 0;
	DBT dbtDatos;
	tipo_datos_file tArch;

	iReturn = DB_Consultar("file", idArchivoActual, &dbtDatos);
	Macro_CheckError(iReturn,"Error al consultar file");

	if (dbtDatos.data != NULL) {
		tArch = *((tipo_datos_file*) dbtDatos.data);
	} else {
		iReturn = -1;
		Macro_CheckError(iReturn,"No se encontraron datos");
	}


	memcpy(tArch.sNombre, sNombreArchivoNuevo, strlen(sNombreArchivoNuevo) + 1);

	iReturn = DB_Modificar("file", &tArch, sizeof(tArch), idArchivoActual);
	Macro_CheckError(iReturn,"Error al modificar file");

	return iReturn;
}

int FILE_ConsultarId(tipo_id tIdArchivo, tipo_datos_file *tArchivo) {
	//Variable temporal para extraer los datos
	DBT dbtDatos;
	//Variable para Retorno de Errores
	int iReturn = 0;

	//Obtengo de la Tabla "file" los datos asociados a ese ID de archivo
	iReturn = DB_Consultar("file", tIdArchivo, &dbtDatos);
	if (iReturn != DB_NOTFOUND) {
		Macro_CheckError(iReturn,"Error al consultar file");

		//Transformo los tipos, para poder devovler "tipo_datos_file"
		if (dbtDatos.data != NULL) {
			*tArchivo = *((tipo_datos_file*) dbtDatos.data);
		} else {
			iReturn = -1;
			Macro_CheckError(iReturn,"No se encontraron datos");
		}
	}

	return iReturn;
}

int FILE_Eliminar(char sNombreArchivo[FILE_LONGITUD_NOMBRE]) {
	int iReturn = 0;
	tipo_id tId;
	iReturn = FILE_BuscarNombre(sNombreArchivo, &tId, 0, NULL);
	Macro_CheckError(iReturn,"Error al buscar Nombre");

	iReturn = DB_Eliminar("file", tId);
	Macro_CheckError(iReturn,"Error al eliminar en file");

	return iReturn;
}

int FILE_EliminarId(tipo_id tIdArchivo) {
	int iReturn = 0;

	iReturn = DB_Eliminar("file", tIdArchivo);
	Macro_CheckError(iReturn, "Error al eliminar en file");

	return iReturn;
}

int FILE_Mover(tipo_id idArchivoPorMover, tipo_id tIdCarpetaNueva) {
	int iReturn = 0;
	DBT dbtDatos;
	tipo_datos_file tArch;


	//Ahora obtengo de la tabla "file" los datos de ese archivo
	iReturn = DB_Consultar("file", idArchivoPorMover, &dbtDatos);
	Macro_CheckError(iReturn,"Error al consultar en file");

	//Transformo el tipo de dato
	if (dbtDatos.data != NULL) {
		tArch = *((tipo_datos_file*) dbtDatos.data);
	} else {
		iReturn = -1;
		Macro_CheckError(iReturn,"No se encontraron datos");
	}

	//Cambio el Directorio Padre
	tArch.dirPadre = tIdCarpetaNueva;

	//Modifico/Actualizo con el nuevo padre en la tabla "file"
	iReturn = DB_Modificar("file", &tArch, sizeof(tArch), idArchivoPorMover);
	Macro_CheckError(iReturn,"Error al modificar en file");

	return iReturn;
}

int FILE_Copiar(char sNombreArchivo[FILE_LONGITUD_NOMBRE], char sNombreArchivoCopiado[FILE_LONGITUD_NOMBRE],
		tipo_id tIdCarpetaNueva) {

	int iReturn = 0;
	DBT dbtDatos;
	tipo_id tIdOrigen;
	tipo_datos_file tArch;
	tipo_id tIdCopia;

	iReturn = FILE_BuscarNombre(sNombreArchivo, &tIdOrigen, DB_SET, NULL);
	Macro_CheckError(iReturn,"Error al buscar Nombre");

	iReturn = DB_Consultar("file", tIdOrigen, &dbtDatos);
	Macro_CheckError(iReturn, "Error al consultar file");

	//Transformo el tipo de dato
	if (dbtDatos.data != NULL) {
		tArch = *((tipo_datos_file*) dbtDatos.data);
	} else {
		iReturn = -1;
		Macro_CheckError(iReturn,"No se encontraron datos");
	}

	if (strcmp(sNombreArchivo,sNombreArchivoCopiado) != 0) {
		memcpy(tArch.sNombre,sNombreArchivoCopiado,strlen(sNombreArchivoCopiado)+1);
	}

	tArch.dirPadre = tIdCarpetaNueva;

	iReturn = DB_Insertar("file",&tArch,sizeof(tArch), &tIdCopia);

	return iReturn;
}

static int FILE_idxBuscarNombre(DB *psdb, const DBT *pkey, const DBT *pdata, DBT *skey) {
	tipo_datos_file *tArch;
	tArch = ((tipo_datos_file *) pdata->data);

	memset(skey, 0, sizeof(DBT));
	skey->data = tArch->sNombre;
	skey->size = strlen(tArch->sNombre)+1;
	return (0);
}

static int FILE_idxBuscarIdPadre(DB *psdb, const DBT *pkey, const DBT *pdata, DBT *skey) {
	tipo_datos_file *tArch;
	tArch = ((tipo_datos_file *) pdata->data);

	memset(skey, 0, sizeof(DBT));
	skey->data = &tArch->dirPadre;
	skey->size = sizeof(tArch->dirPadre);
	return (0);
}

int FILE_AbrirIndices() {
	int iReturn = 0;

	iReturn = DB_CrearIndice("file", "filemd5", FILE_idxBuscarNombre);
	Macro_CheckError(iReturn,"Error al crear Indice");
	iReturn = DB_consultarCursor("filemd5", NULL, NULL, 0, 1, NULL, 0, NULL);
	Macro_CheckError(iReturn,"Error al crear cursor");
	iReturn = DB_CrearIndice("file", "filepadre", FILE_idxBuscarIdPadre);
	Macro_CheckError(iReturn,"Error al crear Indice");
	iReturn = DB_consultarCursor("filepadre", NULL, NULL, 0, 1, NULL, 0, NULL);
	Macro_CheckError(iReturn,"Error al crear cursor");

	return iReturn;
}

int FILE_CerrarIndices() {
	int iReturn = 0;

	//Primero Debo Repasar Todos los Archivo que haya y Poner en NULL su BitArray porque ya no existira mas en memoria. Asi la proxima vez que Inicie el FS no hace referencia a memoria inexistente.
	tipo_id tIdUltimo, id;
	iReturn = DB_getUltimoId("file" , &tIdUltimo);
	tipo_datos_file datosFile;
	Macro_CheckError(iReturn , "Error al obtener el ultimo Id de File.\n");

	for (id = 1; id < tIdUltimo; id++) {
		//Obtengo los Datos del Nodo
		iReturn = FILE_ConsultarId(id, &datosFile);

		//Si no pude obtener los Datos porque no lo encuentra, que siga de largo al proximo nodo (este nodo no se modifica)
		if (iReturn == DB_NOTFOUND) {
			continue;
		} else {
			Macro_CheckError(iReturn , "Error al consultar los Datos del Archivo.\n");

			datosFile.tBloquesActivos = NULL;
			datosFile.disponible = false;

			//Actualizo el File
			FILE_Modificar(id , datosFile);
			Macro_CheckError(iReturn , "Error al actualizar los Datos del Archivo.\n");
		}
	}



	iReturn = DB_consultarCursor("filemd5", NULL, NULL, 0, 2, NULL, 0, NULL);
	Macro_CheckError(iReturn,"Error al cerrar cursor");
	iReturn = DB_consultarCursor("filepadre", NULL, NULL, 0, 2, NULL, 0, NULL);
	Macro_CheckError(iReturn,"Error al cerrar cursor");

	return iReturn;
}

/*
 * sNombreArchivo: Nombre del archivo (SIN la ruta de Linux o del File System, EJ: datos.txt )
 * tId: ID del Archivo, este parametro debe pasarse por referencia para que lo modifique y lo devuelva.
 * iRegistro: Parametro para recorrer en caso de duplicados, los valores
 *		posibles son 0,1,2,3,4 (NUEVO, PRIM, SIG, ANT, ULT)
 */
int FILE_BuscarNombre(char sNombreArchivo[FILE_LONGITUD_NOMBRE], tipo_id *tId, int iRegistro, int *iCantRegistros) {
	//Variable para Manejar Errores, inicializo en 0 que significa no hay errores.
	int iReturn = 0;
	int iTamano;

	if (strlen(sNombreArchivo) == 0) {
		*tId = 0;
		return -1;
	}
	iTamano = strlen(sNombreArchivo)+1;
	//Le digo a la Base de Datos que busque el ID del Archivo en la tabla "filemd5"
	iReturn = DB_consultarCursor("filemd5", tId, NULL, iRegistro, 0, sNombreArchivo, iTamano, iCantRegistros);
	Macro_CheckError(iReturn,"Error al consultar cursor");

	//Llegados a este punto se pudo buscar correctamente, devuelvo el ID del Archivo, que deberia ser mayor o igual a 0.
	if(*tId==0){
		return -1;
	}

	return iReturn;
}

int FILE_BuscarPorPadre(tipo_id tIdPadre, tipo_id *tId, int iRegistro, int *iCantRegistros) {
	int iTamano = 0;
	int iReturn = 0;
	iTamano = sizeof(tIdPadre);
	/* Obtengo la cantidad de registros, el Id del registro que correponde
	 al directorio que busco, y especifico como manejar los duplicados */
	iReturn = DB_consultarCursor("filepadre", tId, NULL, iRegistro, 0, &tIdPadre, iTamano, iCantRegistros);
	Macro_CheckError(iReturn,"Error al consultar cursor");

	return iReturn;
}

int FILE_PonerDisponible(tipo_id tIdArch, t_bitarray *tBloquesActivos, int iCantBloques) {
	int iReturn = 0;
	tipo_datos_file tArch;

	//Obtengo los Datos del Archivo
	iReturn = FILE_ConsultarId(tIdArch, &tArch);
	Macro_CheckError(iReturn,"Error al consultar el archivo");

	//Actualizo los Datos del archivo como Disponible
	tArch.disponible = true;
	tArch.tBloquesActivos = tBloquesActivos;
	tArch.iCantBloques = iCantBloques;

	//Actualizo la modificacion en la Base de Datos
	iReturn = DB_Modificar("file", &tArch, sizeof(tArch), tIdArch);
	Macro_CheckError(iReturn,"Error al modificar en file");

	return iReturn;
}

int FILE_PonerNoDisponible(tipo_id tIdArch) {
	int iReturn = 0;
	tipo_datos_file tArch;

	iReturn = FILE_ConsultarId(tIdArch, &tArch);
	Macro_CheckError(iReturn,"Error al consultar el archivo");

	tArch.disponible = false;

	iReturn = DB_Modificar("file", &tArch, sizeof(tArch), tIdArch);
	Macro_CheckError(iReturn,"Error al modificar en file");

	return iReturn;
}

int FILE_Modificar(tipo_id tIdArch, tipo_datos_file tDatosArch) {
	int iReturn = 0;

	iReturn = DB_Modificar("file", &tDatosArch, sizeof(tDatosArch), tIdArch);
	Macro_CheckError(iReturn,"Error al modificar en file");

	return iReturn;
}


bool FILE_estaDisponibleArchivo(char* archivoNombre){
		//Variable para Manejar Errores, por defecto es 0 que significa sin Error
		int iNumeroError = 0;

		//Obtengo el ID del Archivo
		tipo_id archivoID;
		iNumeroError = FILE_BuscarNombre(archivoNombre, &archivoID, 1, NULL);
		//Chequeo Errores
		if (iNumeroError < 0) {
			Macro_ImprimirParaDebug("[Funcion %s] No se pudo obtener el ID del archivo %s\n", __func__, archivoNombre);
			return false;
		}

		//Ahora Obtengo los Datos
		tipo_datos_file archivoDatos;
		iNumeroError = FILE_ConsultarId(archivoID, &archivoDatos);
		//Chequeo Errores
		if (iNumeroError < 0) {
			Macro_ImprimirParaDebug("[Funcion %s] No se pudo obtener los Datos del archivo %s\n", __func__, archivoNombre);
			return false;
		}

		//Ahora Verifico si Esta Disponible o NO
		int bitActual;
		//Estaba mal, porque Revisaba todos los bits que tiene el bitarray, que siempre es multiplo de 8 y nosotros podriamos no usar el ultimo Bit por ejemplo. Ahora se usan la Cantidad de Bloques del archivo
		for (bitActual = 0; bitActual < archivoDatos.iCantBloques; ++bitActual) {
			//Verifico por Cada Bit que este OK, si alguno esta mal, es que no esta disponible el archivo, asi que devuelvo false.
			if(bitarray_test_bit(archivoDatos.tBloquesActivos, bitActual)==false){
				return false;
			}
		}
		//Si llegamos hasta aca es que todos los Bits estan en SET, estan OK
		return true;
}
