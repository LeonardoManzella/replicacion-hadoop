#include "../headers/fsdb.h"
#include "../../Biblioteca_Comun/Biblioteca_Comun.h"

#include <sys/types.h> //Para Compatibilidad
#include <sys/stat.h> //Para el "mkdir" para Crear la Carpeta de la Base de Datos
#include <errno.h>
#include <stdio.h>
#include <string.h>
//#include <unistd.h>
#include <stdlib.h> //Para "Malloc" y "Free"
#include <stdbool.h> //para Tipo Bool

/*
 * Definicion de ubicacion de la carpeta de base de datos
*/
#define DB_DIR 					"fsdb"
#define DB_RUTA_TABLA_SISTEMA	"fsdb/tablasis.db"

//#define ID_TABLA_BASE_TABLA_DE_TABLAS 1
//#define ID_INICIO_RESTO_TABLAS 2



//Estructura para las tablas del sistema que contienen datos sobre el resto de las tablas
typedef struct  __attribute__ ((__packed__)) {
	char sTabla[DB_LONGITUD_NOMBRE_TABLA]; // Nombre de la Tabla
	char sNombreArch[DB_LONGITUD_ARCHIVO_TABLA]; // Nombre del Archivo
	DB *pdbTabla; // Puntero a la Taba una vez cargada
	tipo_id idUltimoRegistro; // Id del ultimo registro
	// --- Comentado por Julian 06/06/2015
	// Ya no se usa el Array
	//tipo_id idTabla; // Id de la Tabla dentro del Array
	// ---
	bool bDuplicado; // Si soporta duplicados
	DBC *pdbcTabla; // Puntero al Cursos para recorrer la tabla
	bool bIndiceAbierto; //Indica si es un indice y ya esta usado
	int iCantReg; //Cantidad de Registros de la Tabla?
	bool bFormatear; // Flag par saber si la tabla se formatea o no
} tipo_tabla_sistema;


//Definicion de funciones Privadas/Internas
//static int buscarDuplicado(char sTablaBuscar[DB_LONGITUD_NOMBRE_TABLA], void* tBusqueda, int iTamano);
//static int Duplicados(DB *pTabla, const DBT *pkey, const DBT *data, DBT *skey);
void LogErrores(const DB_ENV *Origen, const char *sError, const char *sFuncion);

//Funcion Interna para Crear Entorno de Datos (si no esta abierto) o leerlo (si ya esta abierto), es un Super Puntero necesario para poder trabajar con Berkeley
//pdeParam: puntero de DB_ENV y lo devuelve con el valor
static int DB_CrearLeerEntorno(DB_ENV **pdeParam);

/* Funcion Interna que almacena/Lee/Agrega los datos de las Tablas
 * 	Id: 0 asigna automaticamente Id, si >0 lo asigna a la tabla
 * 	tTablas: puntero a la Tabla
 * 	cUpdate: con 1 actualiza los datos de la tabla, con 0 leo las tablas y lo carga en "tTablas"
 */
//static int DB_Tablas(tipo_id Id, tipo_tabla_sistema* tTablas, int cUpdate);

/* Funcion Interna que dado el nombre devuelve el Id de la Tabla
 * sTabla: Nombre de la tabla
 * tId: devuelve el Id de la Tabla
 */
//static int DB_getTablaId(char sTabla[DB_LONGITUD_NOMBRE_TABLA], tipo_id *tId);

/*	Funcion que dado el nombre devuelve el puntero a la Tabla
 *	sTabla: nombre de la Tabla
 *	tTabla: puntero de la Tabla
 */
static int DB_getTabla(char sTabla[DB_LONGITUD_NOMBRE_TABLA], tipo_tabla_sistema *tTabla);

/*	Funcion Interna que asigna nuevas tablas a la Tablas del Sistema/ tabla base / tabla de tablas
 *	sTabla: nombre de la Tabla
 *	sNombreArch: nombre del archivo
 *	tIdUltimo: ultimo Id que genero esa Tabla
 *	tTabla: puntero a la Tabla de Sistema
 */
//static int DB_crearTablaSis(char sTabla[DB_LONGITUD_NOMBRE_TABLA], char sNombreArch[DB_LONGITUD_ARCHIVO_TABLA],tipo_id tIdUltimo, char cDuplicado);

/*	Funcion Interna que Abre una Tabla y le asigna un puntero
 *	sTabla: nombre de la Tabla
 *	cDup: permitir duplicados en la tabla
 */
static int DB_AbrirTabla(char sTabla[DB_LONGITUD_NOMBRE_TABLA]);

/*	Funcion Interna que Llama a las funciones de Berkeley parar Abrir El entorno de Datos, es llamada una sola ves por 'DB_CrearLeerEntorno'
 *	pdeFileSystem: puntero del Enterno de datos
 *	sDir: Carpeta donde se encuentra o crea el entorno de datos
 * return = 0 si es exitoso
 */
static int DB_AbrirEntorno(DB_ENV **pdeFileSystem, char* sDir);

//Actualiza la Tabla indicada por "Id" con los datos indicados en "tTablas"
//"tTablas" debe pasarse por referencia: &tTablas
//Devuelve 0 en caso de ejecucion correcta o Un numero negativo en caso de error
//Debe ser Static para que solo sea una funcion Internat y No Pueda ser llamada desde afuera
//static int DB_Actualizar_Tabla(tipo_id Id, tipo_tabla_sistema *tTablas);

//Obtengo los Datos en "tTablas" de la Tabla indicada por "Id"
//"tTablas" debe pasarse por referencia: &tTablas
//Devuelve 0 en caso de ejecucion correcta o Un numero negativo en caso de error
//Debe ser Static para que solo sea una funcion Internat y No Pueda ser llamada desde afuera
//static int DB_Leer_Tabla(tipo_id Id, tipo_tabla_sistema *tTablas);

//Agrego a las tablas Ya existentes una tabla (se agrega al final)
//Los datos de la Nueva Tabla estan indicados por "tTablas"
//"tTablas" debe pasarse por referencia: &tTablas
//Devuelve 0 en caso de ejecucion correcta o Un numero negativo en caso de error
//Debe ser Static para que solo sea una funcion Internat y No Pueda ser llamada desde afuera
//static int DB_Agregar_Tabla(tipo_tabla_sistema *tTablas);



/*
 * Definicion Interna de Funcion generica para la busqueda por indices
 */
//static int (*callback)(DB * secondary, const DBT *key, const DBT *data, DBT *result);

/*static int CompDatos(DB *tabla, const DBT *datos1, const DBT *datos2, size_t *a) {
	if (datos1->data == datos2->data) {
		return DB_KEYEXIST;
	}
	return DB_KEYEXIST;
}

static int CompKeys(DB *tabla, const DBT *datos1, const DBT *datos2, size_t *a) {
	return 0;
}

static int OrdenarPorId(DB *dbp, const DBT *a, const DBT *b)
{
    int ai, bi;

     *
     * < 0 if a < b 
     * = 0 if a = b 
     * > 0 if a > b 
     *
    memcpy(&ai, a->data, sizeof(int)); 
    memcpy(&bi, b->data, sizeof(int)); 
    return (ai - bi); 
} 
*/

/*
static int Duplicados(DB *pTabla, const DBT *pkey, const DBT *data, DBT *skey) {
	memset(skey, 0, sizeof(DBT));
	skey->data = data->data;
	skey->size = data->size;
	return 0;
}

static int buscarDuplicado(char sTablaBuscar[DB_LONGITUD_NOMBRE_TABLA], void* tBusqueda, int iTamano) {
	DBT data, pkey, skey;
	DBC *pcTabla;
	char sTabla[DB_LONGITUD_NOMBRE_TABLA];
	int iReturn = 0;

	sprintf(sTabla, "%sdup", sTablaBuscar);

	tipo_tabla_sistema tTabla;
	DB_getTabla(sTabla, &tTabla);

	tTabla.pdbTabla->cursor(tTabla.pdbTabla, NULL, &pcTabla, 0);

	memset(&skey, 0, sizeof(skey));
	memset(&pkey, 0, sizeof(pkey));
	memset(&data, 0, sizeof(data));
	skey.data = tBusqueda;
	skey.size = iTamano;
	skey.flags = DB_DBT_MALLOC;
	pkey.flags = DB_DBT_MALLOC;
	data.flags = DB_DBT_MALLOC;

	iReturn = pcTabla->c_pget(pcTabla, &skey, &pkey, &data, DB_SET);

	pcTabla->c_close(pcTabla);

	return iReturn;
}
*/

/*
 * Funcion para logeo de errores
 */

void DB_TablaSis(tipo_tabla_sistema *tTabla, tipo_tabla_sistema *tIndice, bool bSet) {
	static tipo_tabla_sistema tTablaSis, tIndiceTablaSis;
	if (tTabla != NULL) {
		if ((bSet == false) && (tTablaSis.pdbTabla != NULL) ){
			*tTabla = tTablaSis;
		} else {
			tTablaSis = *tTabla;
		}
	}
	if (tIndice != NULL) {
		if ((bSet == false) && (tIndiceTablaSis.pdbTabla != NULL)) {
			*tIndice = tIndiceTablaSis;
		} else {
			tIndiceTablaSis = *tIndice;
		}
	}
}


static int DB_buscarTablaSisNombre(char sTablaBuscada[DB_LONGITUD_NOMBRE_TABLA], tipo_tabla_sistema *tTabla, tipo_id *tIdTabla) {
	DBT data, pkey, skey;
	DBC *pcTabla;
	int iReturn = 0;

	tipo_tabla_sistema tIndiceSis;
	DB_TablaSis(NULL,&tIndiceSis, false);

	tIndiceSis.pdbTabla->cursor(tIndiceSis.pdbTabla, NULL, &pcTabla, 0);

	memset(&skey, 0, sizeof(DBT));
	memset(&pkey, 0, sizeof(DBT));
	memset(&data, 0, sizeof(DBT));
	skey.data = sTablaBuscada;
	skey.size = strlen(sTablaBuscada)+1;
	//skey.flags = DB_DBT_MALLOC;
	//pkey.flags = DB_DBT_MALLOC;
	//data.flags = DB_DBT_MALLOC;

	iReturn = pcTabla->c_pget(pcTabla, &skey, &pkey, &data, DB_SET);
	if (iReturn <0) {
		//free(skey);
		//free(pkey);
		//free(data);
	}
	Macro_CheckError(iReturn, "%s", db_strerror(iReturn));
	if (tTabla != NULL) {
		*tTabla = *((tipo_tabla_sistema*) data.data);
	}
	if (tIdTabla != NULL) {
		*tIdTabla = *((int*) pkey.data);
	}

	pcTabla->c_close(pcTabla);

	return iReturn;
}


void LogErrores(const DB_ENV *Origen, const char *sError, const char *sFuncion) {
	FILE *fError;
	//fError = (FILE *) malloc(sizeof(FILE));

	fError = fopen("dberror.log", "a+");

	fprintf(fError, "Error: %s\n", sFuncion);
	Macro_ImprimirParaDebug("Error de Berkeley %s\n", sFuncion);

	fclose(fError);
}

/*
 * NumeroRegistros: devuelve el numero de registros de una tabla
 */

int DB_NumeroRegistros(char sTabla[DB_LONGITUD_NOMBRE_TABLA]) {
	int iReturn = 0;
	tipo_tabla_sistema tTabla;
	iReturn = DB_getTabla(sTabla, &tTabla);
	Macro_CheckError(iReturn,"Error al obtener datos de la Tabla");
	return tTabla.iCantReg;
}

/*
 * Funcion Interna para Crear Entorno de Datos
 * 	pdeParam: puntero de DB_ENV y lo devuelve con el valor
 */
static int DB_CrearLeerEntorno(DB_ENV **pdeParam) {
	int iReturn = 0;
	/* Define una variable estatica para guardar el puntero al 
	 entorno de datos */
	static DB_ENV *pdeAbierto;

	/*if (*pdeParam == NULL) {
		iReturn = -1;
		CheckError(iReturn,"Se paso un parametro de Entorno de Datos NULL");
	}*/
	
	/* Verifica si el puntero tiene datos, si esta vacio, lo abre */
	if (!(pdeAbierto)) {
		//pdeAbierto = (DB_ENV*) malloc(sizeof(pdeAbierto));
		iReturn = DB_AbrirEntorno(&pdeAbierto, DB_DIR);
		Macro_CheckError(iReturn,"Error al Abrir entorno de Datos Estatico");
	}

	/* Devuelve la direccion del puntero */
	*pdeParam = pdeAbierto;
	return iReturn;
}

/*
 * Comentado por Julian 06/06/2015
 * La funcion ya no es necesaria
static int DB_Leer_Tabla(tipo_id Id, tipo_tabla_sistema *tTablas) {
	int iReturn = 0;
	iReturn = DB_Tablas(Id,tTablas,0);
	CheckError(iReturn,"Error al leer datos de la Tabla");
	return iReturn;
}
*/

static int DB_Actualizar_Tabla(char sTabla[DB_LONGITUD_NOMBRE_TABLA], tipo_tabla_sistema *tTablas) {
	int iReturn = 0;
	// TOOD: Usar Modfiicar para actualizar datos en tablasis
	//iReturn = DB_Tablas(Id,tTablas,1);
	//CheckError(iReturn,"Error al actuaizar datos de la Tabla");
	tipo_id tIdTablaSis;
	if (strcmp(sTabla,"tablasis") != 0) {
		DB_buscarTablaSisNombre(sTabla,NULL,&tIdTablaSis);
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
		iReturn = DB_Modificar("tablasis", tTablas, sizeof(*tTablas), tIdTablaSis);
#pragma GCC diagnostic pop
		Macro_CheckError(iReturn,"Error al Actualizar Datos en la Tabla de Sistema");
	} else {
		DB_TablaSis(tTablas, NULL, true);
	}
	return iReturn;
}


/*
static int DB_Agregar_Tabla(tipo_tabla_sistema *tTablas) {
	//NOTA: Ignora el ultimo parametro totalmente, no importa que valor se le ponga
	int iReturn = 0;

	// --- Comentado Julian 06/06/2015
	//iReturn = DB_Tablas(0,tTablas,0);
	//CheckError(iReturn,"Error al Agregar una Tabla");
	// ---

	return iReturn; 
}
*/

/* Funcion Interna que almacena/Lee/Agrega los datos de las Tablas
 * 	Id: 0 asigna automaticamente Id, si >0 lo asigna a la tabla
 * 	tTablas: puntero a la Tabla
 * 	cUpdate: con 1 actualiza los datos de la tabla, con 0 leo las tablas y lo carga en "tTablas"
 */
/*
 * Funcion Deprecada por Julian 06/06/2015

static int DB_Tablas(tipo_id Id, tipo_tabla_sistema* tTablas, int cUpdate) {
	// Define los tipos de datos estaticos, un Array de Tablas y un Id 
	static tipo_tabla_sistema tTablasSis[MAXCTABLAS];
	static tipo_id tId = 0;
	int iReturn = 0;
	
	// El parametro no puede ser nulo nunca!
	if (tTablas == NULL) { 
		iReturn = -1;
		CheckError(iReturn, "Parametro tTablas NULL");
	}
	// Si el Id = 0 entonces guardo la tabla en el array estatico para futuras consultas
	if (Id == 0) {
		tId++;
		// En caso de ya tener todas las posiciones del Array ocupadas da error
		if (tId >= MAXCTABLAS) {
			iReturn = -1;
			CheckError(iReturn, "tId Excedio la Maxima Cantidad de Tablas (MAXCTABLAS)");
		}

		// Asigno el Id de la Tabla, a un valor mas que el que corresponde con el Id del Array 
		// Ya que el valor 0 esta usado para asignar una tabla a una posicion nueva en los parametros
		tTablas->idTabla = tId;
		
		// Guardo los datos de la tabla en el array estatico
		tTablasSis[tId-1] = *tTablas; 
	} else {
		// Si el Id es != 0 entonces tengo un Id de una tabla ya cargada
		// Verifico que efectivamente tenga datos la variable
		if (strlen(tTablasSis[Id-1].sTabla) == 0 ) {
			iReturn = -1;
			CheckError(iReturn,"Se solicitan datos de una tabla no cargada");
		}
		// Utilizo Id - 1, ya que el Id de la Tabla arranca en 1, y el del array en 0
		// cUpdate = 1, entonces actualizo los datos de la table en memoria
		if (cUpdate == 1) {
			// Se actualizan los datos de la tabla en el array estatico
			// tTablas->idTabla = Id; // Tiene sentido esta linea??
			tTablasSis[Id - 1] = *tTablas;
		// cUpdate = 0 entonces me fijo los datos de la tabla y los devuelvo
		} else if (cUpdate == 0) {
			// Se devuelven los datos de la tabla del array estatico
			*tTablas = tTablasSis[Id - 1];
		} else {
			// Se paso un paremtro no valido en cUpdate
			iReturn = -1;
			CheckError(iReturn,"Se paso un valor invalido a cUpdata");
		}
	}
	if (iReturn == 0) {
		iReturn = tId;
	}
	return iReturn;
}
*/

/*
 * getUltimoId: consulta el ultimo Id de la tabla
 */

int DB_getUltimoId(char sTabla[DB_LONGITUD_NOMBRE_TABLA], tipo_id *tIdUltimo) {
	int iReturn = 0;
	tipo_tabla_sistema tTabla;
	
	if (strlen(sTabla) == 0) {
		iReturn = -1;
		Macro_CheckError(iReturn, "Nombre de tabla en blanco");
	}

	iReturn = DB_getTabla(sTabla, &tTabla);
	Macro_CheckError(iReturn,"No se encontro la tabla");
	
	/* Asigna los valores a los punteros DBT key y data */
	*tIdUltimo = tTabla.idUltimoRegistro;
	return iReturn;
}

/*
 * Funcion Interna que devuelve el Id de la Tabla
 * sTabla: Nombre de la tabla
 * tId: devuelve el Id de la Tabla
 */
/*
 * Funcion comentada Julian 06/06/2015
static int DB_getTablaId(char sTabla[DB_LONGITUD_NOMBRE_TABLA], tipo_id *tId) {
	// Define las variables locales
	tipo_tabla_sistema tTablas;
	int tablaActual;
	int iReturn = 0;
	int nTablas = MAXCTABLAS;
	int nTablasRead = 0;

	if (strlen(sTabla) == 0) {
		iReturn = -1;
		CheckError(iReturn,"Se paso un nombre de tabla vacio");
	}

	// Define tId como -1 por si no encuentra ninguna tabla
	*tId = -1;

	// Recorre el Array de Tablas, llamando a la funcion para mantener los
	// datos dentro del contexto de la funcion, y compara el nombre pasado,
	// con el nombre de la tabla devuelta, hasta encontrarlo
	for (tablaActual = 1; tablaActual <= nTablas; tablaActual++) {
	
		iReturn = DB_Leer_Tabla(tablaActual,&tTablas);
		CheckError(iReturn,"Error al leer datos de la Tabla");
		if(!nTablasRead) {
			nTablas = iReturn;
			nTablasRead = 1;
		}
	
		if (strcmp(tTablas.sTabla, sTabla) == 0) {
			*tId = tablaActual;
			break;
		}
	}
	return iReturn;
}
*/

/*
 * Funcion que escribe los datos en el disco o cierra la Tabla
 * 	sTabla: nombre de la tabla
 *	cClose: 0 escribe datos, 1 cierra la tabla
 */
int DB_synccloseTablas(char sTabla[DB_LONGITUD_NOMBRE_TABLA], t_tipo_syncclose cClose) {
	tipo_tabla_sistema tTablas;
//	tipo_id tIdTabla;
	int iReturn = 0;
	
	int tablaActual, iTotalTablas;
	DBT tDatos;
	memset(&tDatos,0,sizeof(DBT));
	//int nTablas = MAXCTABLAS;
	//int nTablasRead = 0;

	// Si el nombre de la tabla es vacio, entonces aplica sobre todas las tablas
	if (strcmp(sTabla, "") == 0) {
/*		for (tablaActual = 1; tablaActual <= nTablas; tablaActual++) {
			iReturn = DB_Leer_Tabla(tablaActual,&tTablas);
			CheckError(iReturn, "Error al leer datos de la Tabla");
			if (!nTablasRead) {
				nTablas = iReturn;
				nTablasRead = 1;
			}
*/
		DB_getUltimoId("tablasis",&iTotalTablas);
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
		for (tablaActual = 1; tablaActual < iTotalTablas; tablaActual++) {
#pragma GCC diagnostic pop
			iReturn = DB_Consultar("tablasis",tablaActual,&tDatos);
			Macro_CheckError(iReturn,"No se pudo consultar en la Tabla de Sistema");

#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
			tTablas = *((tipo_tabla_sistema *) tDatos.data);
#pragma GCC diagnostic pop
			// Verifica que la tabla no se encuentre cerrada
			if (tTablas.pdbTabla != 0) {
				if (cClose == SYNC) {
					// Graba los datos en el disco
					iReturn = tTablas.pdbTabla->sync(tTablas.pdbTabla, 0);
					Macro_CheckError(iReturn, "%s",db_strerror(iReturn));
				} else {
					// Cierra la tabla y guarda los datos
					iReturn = tTablas.pdbTabla->close(tTablas.pdbTabla, 0);
					Macro_CheckError(iReturn, "%s",db_strerror(iReturn));
					tTablas.pdbTabla = 0;
					iReturn = DB_Actualizar_Tabla(tTablas.sTabla,&tTablas);
					Macro_CheckError(iReturn,"Error al Actualizar Datos en la Tabla de Sistema");
				}
			}
		}
		if (cClose == SYNC) {
			iReturn = DB_getTabla("tablasisNombres", &tTablas);
			Macro_CheckError(iReturn,"No se pudo encontrar la Tabla");
			iReturn = tTablas.pdbTabla->sync(tTablas.pdbTabla, 0);
			Macro_CheckError(iReturn, "%s",db_strerror(iReturn));
			iReturn = DB_getTabla("tablasis", &tTablas);
			Macro_CheckError(iReturn,"No se pudo encontrar la Tabla");
			iReturn = tTablas.pdbTabla->sync(tTablas.pdbTabla, 0);
			Macro_CheckError(iReturn, "%s",db_strerror(iReturn));
		} else {
			// Cierra la Tabla y guarda los datos
			iReturn = DB_getTabla("tablasisNombres", &tTablas);
			Macro_CheckError(iReturn,"No se pudo encontrar la Tabla");
			iReturn = tTablas.pdbTabla->close(tTablas.pdbTabla, 0);
			Macro_CheckError(iReturn, "%s",db_strerror(iReturn));
			iReturn = DB_getTabla("tablasis", &tTablas);
			Macro_CheckError(iReturn,"No se pudo encontrar la Tabla");
			iReturn = tTablas.pdbTabla->close(tTablas.pdbTabla, 0);
			Macro_CheckError(iReturn, "%s",db_strerror(iReturn));
		}
	} else {
		// Busca la tabla y devuelve sus datos
		iReturn = DB_getTabla(sTabla, &tTablas);
		Macro_CheckError(iReturn,"No se pudo encontrar la Tabla");

		// --- Comentado por Julian 06/06/2015
		// iReturn = DB_getTablaId(sTabla, &tIdTabla);
		// CheckError(iReturn,"No se pudo encontrar la Tabla");
		// iReturn = DB_Leer_Tabla(tIdTabla,&tTablas);
		//CheckError(iReturn,"No se puedieron cargar los datos de la Tabla");
		// ---
		
		if (cClose == SYNC) {
			// Guarda los datos de la tabla en el disco
			iReturn = tTablas.pdbTabla->sync(tTablas.pdbTabla, 0);
			Macro_CheckError(iReturn, "%s",db_strerror(iReturn));
		} else {
			// Cierra la Tabla y guarda los datos
			iReturn = tTablas.pdbTabla->close(tTablas.pdbTabla, 0);
			Macro_CheckError(iReturn, "%s",db_strerror(iReturn));
			tTablas.pdbTabla = 0;
			iReturn = DB_Actualizar_Tabla(sTabla,&tTablas);
			Macro_CheckError(iReturn,"Error al Actualizar Datos en la Tabla de Sistema");
		}
	}
	return iReturn;
}

/*
 * Funcion que devuelve un puntero a una Tabla
 *	sTabla: nombre de la Tabla
 *	tTabla: puntero de la Tabla
 */
static int DB_getTabla(char sTabla[DB_LONGITUD_NOMBRE_TABLA], tipo_tabla_sistema *tTabla) {
	/* Llama a la funcion getTablaId para devolver el Id de la Tabla
	 y despues busca los datos de la tabla y los devuelve */
	int iReturn = 0;
//	tipo_id tIdTabla;
	
	if (strlen(sTabla) == 0){
		iReturn = -1;
		Macro_CheckError(iReturn,"Se paso un nombre de tabla vacio");
	}
	
	if (tTabla == NULL) {
		iReturn = -1;
		Macro_CheckError(iReturn,"Se paso un parametro como NULL");
	}
	
	if (strcmp(sTabla,"tablasis") == 0) {
		DB_TablaSis(tTabla, NULL, false);
	} else if (strcmp(sTabla,"tablasisNombres") == 0) {
			DB_TablaSis(NULL, tTabla, false);
	} else {
		// --- Comentado por Julian 06/06/2015
		// Ya no es necesaria la funcion
		// iReturn = DB_getTablaId(sTabla, &tIdTabla);
		// CheckError(iReturn,"No se encontro la tabla");
		//
		//	iReturn = DB_Leer_Tabla(tIdTabla,tTabla);
		//	CheckError(iReturn,"Hubo un error al leer los datos de la Tabla");

		// Recorro todas las tablas existentes en tablasis hasta que encuentro la que busco
		iReturn = DB_buscarTablaSisNombre(sTabla,tTabla, NULL);
		Macro_CheckError(iReturn,"Error al buscar la Tabla en la Tabla se Sistema");
		//DB_consultarCursor("tablasis",)

	}
	
	return iReturn;
}

static void DB_SetearValoresTabla(tipo_tabla_sistema *tTabla, char sTabla[DB_LONGITUD_NOMBRE_TABLA], char sNombreArch[DB_LONGITUD_ARCHIVO_TABLA], tipo_id tIdUltimo, bool bDuplicado, bool bFormatear) {
	tTabla->idUltimoRegistro = tIdUltimo;
	memset(tTabla->sTabla, 0, DB_LONGITUD_NOMBRE_TABLA);
	memcpy(tTabla->sTabla, sTabla, strlen(sTabla)+1);
	memset(tTabla->sNombreArch, 0, DB_LONGITUD_ARCHIVO_TABLA);
	memcpy(tTabla->sNombreArch, sNombreArch, strlen(sNombreArch)+1);
	tTabla->bDuplicado = bDuplicado;
	tTabla->pdbTabla = 0;
	tTabla->pdbcTabla = 0;
	tTabla->bIndiceAbierto = false;
	tTabla->iCantReg = 0;
	tTabla->bFormatear = bFormatear;
}

static int DB_crearTablas(char sTabla[DB_LONGITUD_NOMBRE_TABLA], char sNombreArch[DB_LONGITUD_ARCHIVO_TABLA], bool bDuplicado, bool bFormatear) {
	
	tipo_tabla_sistema tTabla;
	tipo_id tIdTablaSis;
	int iReturn = 0;

	/* Asigna los parametros a los valores de la estructura */
	DB_SetearValoresTabla(&tTabla,sTabla,sNombreArch,1,bDuplicado,bFormatear);
	/*
	tTabla.idUltimoRegistro = tIdUltimo;
	memset(tTabla.sTabla, 0, DB_LONGITUD_NOMBRE_TABLA);
	memcpy(tTabla.sTabla, sTabla, strlen(sTabla)+1);
	memset(tTabla.sNombreArch, 0, DB_LONGITUD_ARCHIVO_TABLA);
	memcpy(tTabla.sNombreArch, sNombreArch, strlen(sNombreArch)+1);
	tTabla.bDuplicado = cDuplicado;
	tTabla.pdbTabla = 0;
	tTabla.pdbcTabla = 0;
	tTabla.bIndiceAbierto = 0;
	tTabla.iCantReg = 0;
	*/

	/* Guarda la tabla con los datos en el Array en el primer lugar libre */
	//iReturn = DB_Agregar_Tabla(&tTabla);
	//CheckError(iReturn, "Error al Agregar la Tabla");
	iReturn = DB_Insertar("tablasis", &tTabla, sizeof(tTabla), &tIdTablaSis);
	Macro_CheckError(iReturn,"Error al Insertar Datos en la Tabla de Sistema");
	//iReturn = DB_synccloseTablas("tablasisNombres", SYNC);

	// --- Comentado por Julian 07/06/2015
	// Despues de Insertar tengo que actualizar la Tabla de Sistema
	// DB_TablaSis(&tTablaSis,NULL,false);
	// tTablaSis.idUltimoRegistro = tIdTablaSis+1;
	// tTablaSis.iCantReg++;
	// DB_TablaSis(&tTablaSis,NULL,true);
	// ---

	/* Abre la tabla */
	iReturn = DB_AbrirTabla(sTabla);
	Macro_CheckError(iReturn, "No se pudo Abrir la tabla");

	return iReturn;
}

static int DB_idxTablaSis(DB *pTabla, const DBT *pkey, const DBT *data, DBT *skey) {
	//char sNombreTabla[DB_LONGITUD_NOMBRE_TABLA];
	tipo_tabla_sistema *tTablaSis;
	tTablaSis = ((tipo_tabla_sistema *) data->data);
	//memset(sNombreTabla,0,DB_LONGITUD_NOMBRE_TABLA);
	//memcpy(sNombreTabla,tTablaSis->sTabla,strlen(tTablaSis->sTabla) + 1);
	memset(skey, 0, sizeof(DBT));
	skey->data = tTablaSis->sTabla;
	skey->size = strlen(tTablaSis->sTabla) +1;
	return 0;
}


static int DB_CrearTablaSis() {
	int iReturn = 0;

	tipo_tabla_sistema tTabla, tIndice;
	char sTabla[DB_LONGITUD_NOMBRE_TABLA] = "tablasis";
	char sNombreArch[DB_LONGITUD_ARCHIVO_TABLA] = "tablasis.db";

	/* Asigna los parametros a los valores de la estructura */
	DB_SetearValoresTabla(&tTabla,sTabla,sNombreArch,1,false,false);

	/* Define el puntero del entorno de datos, y le da un valor */
	DB_ENV *pdeFileSystem;

	iReturn = DB_CrearLeerEntorno(&pdeFileSystem);
	Macro_CheckError(iReturn,"No se puedo abrir el Entorno de Datos");

	/* Crea el puntero a la Tabla y lo asocia al entorno de datos
	 similar a la funcion malloc */
	iReturn = db_create(&tTabla.pdbTabla, pdeFileSystem, 0);
	Macro_CheckError(iReturn, "%s",db_strerror(iReturn));

	/* Abre el archivo y le asigna el puntero a la Tabla*/
	iReturn = tTabla.pdbTabla->open(tTabla.pdbTabla, NULL, tTabla.sNombreArch,
	//NULL, DB_BTREE, DB_CREATE | DB_THREAD | DB_AUTO_COMMIT, 0664); //DB_RDONLY para solo lectura
	NULL, DB_BTREE, DB_CREATE, 0664); //DB_RDONLY para solo lectura
	Macro_CheckError(iReturn, "%s",db_strerror(iReturn));

	char sTablaIdx[DB_LONGITUD_NOMBRE_TABLA] = "tablasisNombres";
	char sNombreArchIdx[DB_LONGITUD_ARCHIVO_TABLA] = "tablasis.idx";

	/* Asigna los parametros a los valores de la estructura */
	DB_SetearValoresTabla(&tIndice,sTablaIdx,sNombreArchIdx,1,true,false);

	/* Crea el puntero a la Tabla y lo asocia al entorno de datos
	 similar a la funcion malloc */
	iReturn = db_create(&tIndice.pdbTabla, pdeFileSystem, 0);
	Macro_CheckError(iReturn, "%s",db_strerror(iReturn));

	iReturn = tIndice.pdbTabla->set_flags(tIndice.pdbTabla, DB_DUPSORT);
	Macro_CheckError(iReturn, "%s",db_strerror(iReturn));

	/* Abre el archivo y le asigna el puntero a la Tabla*/
	iReturn = tIndice.pdbTabla->open(tIndice.pdbTabla, NULL, tIndice.sNombreArch,
	//NULL, DB_BTREE, DB_CREATE | DB_THREAD | DB_AUTO_COMMIT, 0664); //DB_RDONLY para solo lectura
	NULL, DB_BTREE, DB_CREATE , 0664); //DB_RDONLY para solo lectura
	Macro_CheckError(iReturn, "%s",db_strerror(iReturn));

	iReturn = tTabla.pdbTabla->associate(tTabla.pdbTabla, NULL, tIndice.pdbTabla, DB_idxTablaSis, DB_CREATE);
	Macro_CheckError(iReturn,"Error al asociar el indice de la Tabla de Sistema");

	// Guarda la Tabla en la variable estatica para ser accedida posteriormente
	DB_TablaSis(&tTabla, &tIndice, true);

	return iReturn;
}

/*
 * Funcion que inicializa el entorno de datos y las Tablas ( las crea en caso de no existir previamente)
 */
int DB_TablasSistema() {
	int iReturn = 0;
	tipo_tabla_sistema tTablas;
	bool CrearTablas = false; // Variable para controlar existencia de las Tablas

	/* Verifica que exista el archivo de tablas de sistemas conteniendo 
	 datos viejos (suponemos que si esta este archivo van a estar todos los demas) */
	if (0 != access(DB_RUTA_TABLA_SISTEMA, F_OK)) {
		if (ENOENT == errno) {
			/* Si el archivo no existe setea el flags */
			CrearTablas = true;
		}
	}

	/* Crea la primera tabla que es la que contiene todos los
	 datos de las demas Tablas. Le asigna un Ultimo Id=1 ya que es la misma
	 tabla Base del sistema. */
	iReturn = DB_CrearTablaSis();
	Macro_CheckError(iReturn,"Error al Crear Tabla de Sistema");

	/* Si no existen tablas, crea todas las tablas */
	if (CrearTablas == true) {
		//Lo siguiente es cuando creamos las tablas que realmente usamos nosotros.
		//Los archivos que terminan en ".db" (de "DataBase") son realmente los archivos que contienen los datos de las tablas
		//Los archivos que terminan en ".idx" (de "Index") contienen datos para poder recorrer los datos de las tabla de distinta manera, basicamente contienen los datos de los archivos ".db" pero ordenados cada ".idx" de una manera distinta.
		//Osea, sirven para recorrer la misma Tabla de distinta manera, de distinto orden (esto me facilita el uso de los datos en las funciones, por ejemplo si yo quiero buscar Dado un Directorio Padre sus Directorio Hijos voy a utilizar el indice "indpadre" en vez de "dirdup")

		iReturn = DB_crearTablas("dir", "dir.db", false, true);
		Macro_CheckError(iReturn,"Error al insertar datos en el tabla");
		//iReturn = DB_datosTablaSis("dirdup", "dir0.idx", 1, 1);
		//CheckError(iReturn,"Error al insertar datos en el tabla");

		//Aca creo el Indice (en memoria) para poder recorrer la Tabla, sino no podria recorrerla con este Indice
		//iReturn = DB_CrearIndice("dir", "dirdup", Duplicados);
	 	//CheckError(iReturn,"Error al crear Indice de Duplicados");
	 	// Aca no haria falta un "DB_consultarCursor" como en la funcion "DIR_AbrirIndices"? Tampoco entiendo porque lo Comentastes, no se supone que es necesario Crear el indice para Usarlo?

		iReturn = DB_crearTablas("dirindice", "dir1.idx", true, false);
		Macro_CheckError(iReturn,"Error al insertar datos en el tabla");
		//Aca no falta un "DB_CrearIndice" porque se hace en "DIR_AbrirIndices"
		iReturn = DB_crearTablas("indpadre", "dir2.idx", true, false);
		Macro_CheckError(iReturn,"Error al insertar datos en el tabla");
		//Aca no falta un "DB_CrearIndice" porque se hace en "DIR_AbrirIndices"

		iReturn = DB_crearTablas("file", "file.db", false, true);
		Macro_CheckError(iReturn,"Error al insertar datos en el tabla");
		//iReturn = DB_datosTablaSis("filedup", "file0.idx", 1, 1);
		//CheckError(iReturn,"Error al insertar datos en el tabla");

		//iReturn = DB_CrearIndice("file", "filedup", Duplicados);
		//CheckError(iReturn,"Error al insertar datos en el tabla");
		// Aca no haria falta un "DB_consultarCursor" como en la funcion "FILE_AbrirIndices"? Tampoco entiendo porque lo Comentastes, no se supone que es necesario Crear el indice para Usarlo?
		
		iReturn = DB_crearTablas("filemd5", "file1.idx", true, false);
		Macro_CheckError(iReturn,"Error al insertar datos en el tabla");
		//Aca no falta un "DB_CrearIndice" porque se hace en "FILE_AbrirIndices"
		iReturn = DB_crearTablas("filepadre", "file2.idx", true, false);
		Macro_CheckError(iReturn,"Error al insertar datos en el tabla");
		//Aca no falta un "DB_CrearIndice" porque se hace en "FILE_AbrirIndices"

		iReturn = DB_crearTablas("nodo", "nodo.db", false, false);
		Macro_CheckError(iReturn,"Error al insertar datos en el tabla");
		//iReturn = DB_datosTablaSis("nododup", "nodo0.idx", 1, 1);
		//CheckError(iReturn,"Error al insertar datos en el tabla");
		//iReturn = DB_CrearIndice("nodo", "nododup", Duplicados);
		//CheckError(iReturn,"Error al insertar datos en el tabla");
		// Aca no haria falta un "DB_consultarCursor" como en la funcion "NODO_AbrirIndices"?

		iReturn = DB_crearTablas("nodonombre", "nodo1.idx", true, false);
		Macro_CheckError(iReturn,"Error al insertar datos en el tabla");
		//Aca no falta un "DB_CrearIndice" porque se hace en "NODO_AbrirIndices"
		iReturn = DB_crearTablas("nodoespacio", "nodo2.idx", true, false);
		Macro_CheckError(iReturn,"Error al insertar datos en el tabla");
		//Aca no falta un "DB_CrearIndice" porque se hace en "NODO_AbrirIndices"

		iReturn = DB_crearTablas("bloques", "bloques.db", false, true);
		Macro_CheckError(iReturn,"Error al insertar datos en el tabla");
		//iReturn = DB_datosTablaSis("bloquesdup", "bloques0.idx", 1, 1);

		//CheckError(iReturn,"Error al insertar datos en el tabla");
		//iReturn = DB_CrearIndice("bloques", "bloquesdup", Duplicados);
		// Aca no haria falta un "DB_consultarCursor" como en la funcion "BLOQUES_AbrirIndices"? Tampoco entiendo porque lo Comentastes, no se supone que es necesario Crear el indice para Usarlo?

		//CheckError(iReturn,"Error al insertar datos en el tabla");
		iReturn = DB_crearTablas("bloquesnodo", "bloques1.idx", true, false);
		Macro_CheckError(iReturn,"Error al insertar datos en el tabla");
		//Aca no falta un "DB_CrearIndice" porque se hace en "BLOQUES_AbrirIndices"
		iReturn = DB_crearTablas("bloquesarch", "bloques2.idx", true, false);
		Macro_CheckError(iReturn,"Error al insertar datos en el tabla");
		iReturn = DB_crearTablas("bloquesespecifico", "bloques3.idx", true, false);
		Macro_CheckError(iReturn,"Error al insertar datos en el tabla");
		//Aca no falta un "DB_CrearIndice" porque se hace en "BLOQUES_AbrirIndices"

		// No se que hacen estas 2 lineas, primero obtiene los datos y luego los reinserta? eso no generaria un dubplicado? No entiendo como es que lo actualiza..
		// Actualiza tablasis con los datos de IdUltimo
		// --- Comentado por Julian 06/06/2015
		//iReturn = DB_getTabla("tablasis", &tTablas);
		//CheckError(iReturn,"Error al insertar datos en el tabla");
		//iReturn = DB_Modificar("tablasis", &tTablas, sizeof(tTablas), 1);
		//CheckError(iReturn,"Error al insertar datos en el tabla");
		// ---

	} else {
		//Las tablas ya existen, entonces no hace falta Crearlas nuevamente

		DBT tDatos;
		memset(&tDatos,0,sizeof(DBT));
		tipo_id tablaActual = 1;

		while (iReturn != DB_NOTFOUND) {
			iReturn = DB_Consultar("tablasis",tablaActual,&tDatos);
			if ((iReturn != DB_NOTFOUND) && (iReturn < 0)) {
				Macro_CheckError(iReturn,"No se pudo consultar en la Tabla de Sistema");
			} else if (iReturn == DB_NOTFOUND) {
				iReturn = 0;
				break;
			}
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
			tTablas = *((tipo_tabla_sistema *) tDatos.data);
#pragma GCC diagnostic pop
			tTablas.pdbTabla = NULL;
			tTablas.pdbcTabla = NULL;
			if (tTablas.bDuplicado == 1) {
				tTablas.bIndiceAbierto = false;
			}
			// Abre la Tabla
			iReturn = DB_Actualizar_Tabla(tTablas.sTabla,&tTablas);
			Macro_CheckError(iReturn,"Error al actualziar tablas de Sistema");
			iReturn = DB_AbrirTabla(tTablas.sTabla);
			Macro_CheckError(iReturn,"No se puedo abrir la tabla");
			tablaActual++;
		}

		DB_getTabla("tablasis",&tTablas);
		tTablas.iCantReg = tablaActual-1;
		tTablas.idUltimoRegistro = tablaActual;
		DB_TablaSis(&tTablas,NULL,true);

/*
		// Carga IdUltimo de tablasis
		if ((iReturn = DB_Consultar("tablasis", 1, &pDatos)) == 0) {
			tTablas = *((tipo_tabla_sistema*) pDatos.data);
			IdUltimo = tTablas.idUltimoRegistro;
			// Que Significa ese "1" hardcodeado? Por Favor Poner un Comentario
			// FIXED Comentame ESTA!!!
			//iReturn = DB_Leer_Tabla(1, &tTablas);
			CheckError(iReturn,"Error al leer datos de la Tabla");
			tTablas.idUltimoRegistro = IdUltimo;
			//iReturn = DB_Actualizar_Tabla(1, &tTablas);
			CheckError(iReturn,"Error al actualizar datos de la Tabla");
		} 
		CheckError(iReturn,"Error al consultar tablasis y cargar datos");

		// Carga el resto de las tablas
		// Que significa ese "2" hardCodeado?
		// Es el resultado de mi primer parcial y el del Ruso!
		for (i = 2; i < IdUltimo; i++){
			if (DB_Consultar("tablasis", i, &pDatos) == 0) {
				tTablas = *((tipo_tabla_sistema*) pDatos.data);
				//iReturn = DB_Actualizar_Tabla(i, &tTablas);
				CheckError(iReturn,"Error al actualizar datos de las tablas");
				iReturn = DB_AbrirTabla(tTablas.sTabla);
				CheckError(iReturn,"No se puedo abrir la tabla");
			}
			CheckError(iReturn,"Error al consultar tablasis");
		}
		*/

		/*
		iReturn = DB_CrearIndice("dir", "dirdup", Duplicados);
	 	CheckError(iReturn,"Error al crear Indice de Duplicados");
		iReturn = DB_CrearIndice("file", "filedup", Duplicados);
		CheckError(iReturn,"Error al insertar datos en el tabla");
		iReturn = DB_CrearIndice("nodo", "nododup", Duplicados);
		CheckError(iReturn,"Error al insertar datos en el tabla");
		iReturn = DB_CrearIndice("bloques", "bloquesdup", Duplicados);
		CheckError(iReturn,"Error al insertar datos en el tabla");
		*/
	}

	// --- Comentado por Julian 06/06/2015
	//iReturn = DB_synccloseTablas("tablasis", SYNC);
	//CheckError(iReturn,"Error al guardar datos en disco");
	// ---

	return iReturn;
}

/*
 * Funcion Interna que Abre una Tabla y le asigna un puntero
 *	sTabla: nombre de la Tabla
 *	cDup: permitir duplicados en la tabla
 */
static int DB_AbrirTabla(char sTabla[DB_LONGITUD_NOMBRE_TABLA]) {
	int iReturn = 0;

	/* Define el puntero del entorno de datos, y le da un valor */
	DB_ENV *pdeFileSystem;

	iReturn = DB_CrearLeerEntorno(&pdeFileSystem);
	Macro_CheckError(iReturn,"No se puedo abrir el Entorno de Datos");

	/* Busca la tabla en el Array y devuelve los datos */
	tipo_tabla_sistema tTabla;
	iReturn = DB_getTabla(sTabla, &tTabla);
	Macro_CheckError(iReturn,"Error al recuperar datos de la Tabla");

	/* Crea el puntero a la Tabla y lo asocia al entorno de datos 
	 similar a la funcion malloc */
	iReturn = db_create(&tTabla.pdbTabla, pdeFileSystem, 0);
	Macro_CheckError(iReturn, "%s",db_strerror(iReturn));

	/* Si la Tabla soporta registros duplicados, estable los flags para
	 que en caso de insertar duplicados no de error */
	if (tTabla.bDuplicado == true) {

		// Elimino los archivos de indices por si alguno tira error
		// TODO: Ver si se puede atrapar mejor el error -30972 en consultar cursor despues del get
		pdeFileSystem->dbremove(pdeFileSystem,NULL,tTabla.sNombreArch,NULL,0);
		iReturn = tTabla.pdbTabla->set_flags(tTabla.pdbTabla, DB_DUPSORT);
		Macro_CheckError(iReturn, "%s",db_strerror(iReturn));
	}

	//if (tTabla.cDuplicado != 1) {
	//tTabla.pdbTabla->set_bt_compare(tTabla.pdbTabla,CompKeys);
	//	tTabla.pdbTabla->set_dup_compare(tTabla.pdbTabla,CompDatos);
	//}

	char nombreFullArchivo[100];
	sprintf(nombreFullArchivo,"fsdb/%s",tTabla.sNombreArch);
	if (0 != access(nombreFullArchivo, F_OK)) {
		if (ENOENT == errno) {
			tTabla.idUltimoRegistro = 1;
			tTabla.iCantReg = 0;
		}
	}
	/* Abre el archivo y le asigna el puntero a la Tabla*/
	iReturn = tTabla.pdbTabla->open(tTabla.pdbTabla, NULL, tTabla.sNombreArch,
//	NULL, DB_BTREE, DB_CREATE | DB_THREAD | DB_AUTO_COMMIT , 0664); //DB_RDONLY para solo lectura
	NULL, DB_BTREE, DB_CREATE , 0664); //DB_RDONLY para solo lectura
	Macro_CheckError(iReturn, "%s",db_strerror(iReturn));


	/* Actuliza los datos del Array con el puntero asignado */
	// --- Comentado por Julian
	//iReturn = DB_Actualizar_Tabla(tTabla.idTabla, &tTabla);
	//CheckError(iReturn, "Error al actualizar datos");
	// ---
	iReturn = DB_Actualizar_Tabla(sTabla,&tTabla);
	Macro_CheckError(iReturn,"Error al Actualizar Datos en la Tabla de Sistema");

	return iReturn;
}

/*
 * Funcion Interna que abre el Enterno de datos o lo crea
 *	pdeFileSystem: puntero del Enterno de datos
 *	sDir: Carpeta donde se encuentra o crea el entorno de datos
 * return = 0 si es exitoso
 */
static int DB_AbrirEntorno(DB_ENV **pdeFileSystem, char* sDir) {
	/* Define variable locales */
	int iReturn = 0;

	/* Asigna el espacio de memoria para el Entorno de Datos */
	iReturn = db_env_create(pdeFileSystem, 0);
	Macro_CheckError(iReturn, "%s",db_strerror(iReturn));


	//Verifica que exista la Carpeta que contendra la Base de Datos, con ruta en "sDir". Si no existe la crea.
	//(normalmente nosotros le pasamos "FSDB" al "sDir")
	if (0 != access(sDir, F_OK)) {
		if (ENOENT == errno) {
			char sNewDir[50];
			sprintf(sNewDir, "./%s/", sDir);
			mkdir(sNewDir, 0775);
		}
	}
	/* if (ENOTDIR == errno) {
	 // no es un directorio
	 }
	 }*/

	// Realiza una primera apertura del Entorno de datos por si hay errores los corrige
	iReturn = (*pdeFileSystem)->open((*pdeFileSystem), sDir,
	DB_CREATE | DB_INIT_MPOOL | DB_RECOVER_FATAL | DB_INIT_TXN, 0664); //DB_FAILCHK
	Macro_CheckError(iReturn, "%s",db_strerror(iReturn));

	(*pdeFileSystem)->close((*pdeFileSystem),0);
	/* Abre el Entorno de Datos */
	iReturn = db_env_create(pdeFileSystem, 0);
	Macro_CheckError(iReturn, "%s",db_strerror(iReturn));
	/* Asigna la funcion para la Captura de Errores del Entorno de Datos */
	(*pdeFileSystem)->set_errcall((*pdeFileSystem), LogErrores);
	
	iReturn = (*pdeFileSystem)->open((*pdeFileSystem), sDir,
	//DB_CREATE | DB_INIT_MPOOL | DB_THREAD | DB_INIT_TXN | DB_INIT_LOCK | DB_INIT_LOG, 0664); //OJO! Aca como un 0 delante el GCC lo toma como el numero 436. Si queres el numero 664 sacale el 0 de delante asi no lo toma como OCTAL
	DB_CREATE | DB_INIT_MPOOL, 0664); //OJO! Aca como un 0 delante el GCC lo toma como el numero 436. Si queres el numero 664 sacale el 0 de delante asi no lo toma como OCTAL
	Macro_CheckError(iReturn, "%s",db_strerror(iReturn));
	
	return iReturn;
}

/*
 * Funcion para insertar datos en una Tabla
 * 	sTabla: nombre de la Tabla
 *	tDatos: tipo de datos abstracto, puede ser int, char, struct...
 *	iTamano: tamoño en bytes de los datos
 */
int DB_Insertar(char sTabla[DB_LONGITUD_NOMBRE_TABLA], void* tDatos, int iTamano, tipo_id *tId) {
	int iReturn = 0;
	tipo_id id_actual;
	DBT key, data;

	tipo_tabla_sistema tTabla;
	iReturn = DB_getTabla(sTabla, &tTabla);
	Macro_CheckError(iReturn,"No se encontro la tabla para insertar");

	memset(&key, 0, sizeof(key));
	memset(&data, 0, sizeof(data));

	/* Asigna los valores a los punteros DBT key y data */
	id_actual = tTabla.idUltimoRegistro;
	key.data = &id_actual;
	key.size = sizeof(id_actual);
	data.data = tDatos;
	data.size = iTamano;

	/*if (strcmp(sTabla, "tablasis") != 0) {
		iReturn = buscarDuplicado(sTabla, tDatos, iTamano);
		if (iReturn != DB_NOTFOUND) {
			return DB_KEYEXIST;
		} else {
			iReturn = 0;
		}
	}*/
	/* Guarda los datos en la Tabla */
	iReturn = tTabla.pdbTabla->put(tTabla.pdbTabla, NULL, &key, &data, 0);
	Macro_CheckError(iReturn, "%s",db_strerror(iReturn) );
	
	/*if (ret == DB_KEYEXIST) {
		 dbtable->err(dbtable, ret, "La clave: %f ya existe!", tId);
	 }*/

	// Actualiza el UltimoId, actualiza los datos en el Array de la Tabla y fuerza el guardar los datos en el disco
	if (tId != NULL) {
		*tId = id_actual;
	}

	tTabla.idUltimoRegistro++;
	tTabla.iCantReg++;

	// --- Comentado por Julian 06/06/2015
	// --- Esto ya no es necesario
	// iReturn = DB_Actualizar_Tabla(tTabla.idTabla, &tTabla);
	// CheckError(iReturn,"Error al actualizar Tabla");
	//
	//iReturn = DB_Modificar("tablasis", &tTabla, sizeof(tTabla), tTabla.idTabla);
	//CheckError(iReturn, "Error al modificar datos de la tablasis");

	iReturn = DB_synccloseTablas(sTabla, SYNC);
	Macro_CheckError(iReturn, "Error al guardar los datos en disco");

	iReturn = DB_Actualizar_Tabla(sTabla,&tTabla);
	Macro_CheckError(iReturn,"Error al Actualizar Datos en la Tabla de Sistema");

	return iReturn;
}

/*
 * Funcion para modificar datos en una Tabla
 * 	sTabla: nombre de la Tabla
 *	tDatos: tipo de datos abstracto, puede ser int, char, struct...
 *	iTamano: tamoño en bytes de los datos
 *	tId: Id del registro que se va a Actualizar
 */
int DB_Modificar(char sTabla[DB_LONGITUD_NOMBRE_TABLA], void* tDatos, int iTamano, tipo_id tId) {
	int iReturn = 0;
	DBT key, data;
	// Modificado 24-05-2015 por Julian Latasa
	
	// Definit una estructura de tabla y la llena con los datos
	tipo_tabla_sistema tTabla;
	iReturn = DB_getTabla(sTabla, &tTabla);
	Macro_CheckError(iReturn, "No se encontro la tabla a modificar");

	// Pone los punteros de datos en 0
	memset(&key, 0, sizeof(DBT));
	memset(&data,0, sizeof(DBT));

	// Asigna el Id al Puntero de datos
	key.data = &tId;
	key.size = sizeof(key.data);

	// Elimina el dato de la tabla y de los indices
	iReturn = tTabla.pdbTabla->del(tTabla.pdbTabla, NULL, &key, 0);
	Macro_CheckError(iReturn, "%s",db_strerror(iReturn));

	//iReturn = DB_synccloseTablas(sTabla, SYNC);
	//Macro_CheckError(iReturn,"Error al guardar datos");

	// Asigna nuevamente los datos para Insertar el registro modificado	
	key.data = &tId;
	key.size = sizeof(key.data);
	data.data = tDatos;
	data.size = iTamano;
	
	// Guarda los datos en la Tabla
	iReturn = tTabla.pdbTabla->put(tTabla.pdbTabla, NULL, &key, &data, 0);
	Macro_CheckError(iReturn, "%s",db_strerror(iReturn));


	iReturn = DB_synccloseTablas(sTabla, SYNC);
	Macro_CheckError(iReturn,"Error al guardar datos");

	return iReturn;
}

/*
 * Funcion para eliminar datos en una Tabla
 * 	sTabla: nombre de la Tabla
 *	tId: Id del registro que se va a Actualizar
 */
int DB_Eliminar(char sTabla[DB_LONGITUD_NOMBRE_TABLA], tipo_id tId) {
	int iReturn = 0;
	DBT key;

	tipo_tabla_sistema tTabla;
	iReturn = DB_getTabla(sTabla, &tTabla);
	Macro_CheckError(iReturn, "No se encontro la tabla al eliminar");

	memset(&key, 0, sizeof(DBT));

	key.data = &tId;
	key.size = sizeof(tId);

	iReturn = tTabla.pdbTabla->del(tTabla.pdbTabla, NULL, &key, 0);
	Macro_CheckError(iReturn, "%s",db_strerror(iReturn));

	tTabla.iCantReg--;
	//iReturn = DB_Actualizar_Tabla(tTabla.idTabla, &tTabla);
	//CheckError(iReturn,"No se pudo actualizar los datos de la Tabla");

	iReturn = DB_synccloseTablas(sTabla, SYNC);
	Macro_CheckError(iReturn,"No se puedieron guardar los datos");

	// --- Comentado por Julian 06/06/2015
	// No es necesario actualizar la tabla de sistema
	//iReturn = DB_Modificar("tablasis", &tTabla, sizeof(tTabla), tTabla.idTabla);
	//CheckError(iReturn,"No se pudo modificar la tablasis");
	// ---
	iReturn = DB_Actualizar_Tabla(sTabla,&tTabla);
	Macro_CheckError(iReturn,"Error al Actualizar Datos en la Tabla de Sistema");

	return iReturn;

}

/*
 * Funcion para consultar datos en una Tabla
 * 	sTabla: nombre de la Tabla
 *	tId: Id del registro que se va a buscar
 *	dbtDatos: puntero al registro tipo DBT que contiene los datos
 */
int DB_Consultar(char sTabla[DB_LONGITUD_NOMBRE_TABLA], tipo_id tId, DBT *dbtDatos) {
	int iReturn = 0;
	DBT key, data;

	tipo_tabla_sistema tTabla;
	iReturn = DB_getTabla(sTabla, &tTabla);
	Macro_CheckError(iReturn, "No se encontro la tabla a consultar");

	memset(&key, 0, sizeof(key));
	memset(&data, 0, sizeof(DBT));

	key.data = &tId;
	key.size = sizeof(tipo_id);
	//data.flags = DB_DBT_MALLOC;

	iReturn = tTabla.pdbTabla->get(tTabla.pdbTabla, NULL, &key, &data, 0);
	if ((iReturn < 0) && (iReturn != DB_NOTFOUND)) {
		Macro_CheckError(iReturn, "%s",db_strerror(iReturn));
	}

	if (data.data != 0) {
		*dbtDatos = data;
	}

	return iReturn;
}

/*
 * Funcion para asociar una tabla con otra y crear un indice
 *	sTabla: nombre de la tabla que contiene los datos
 *	sIndice: nombre de la tabla que va a ser indice
 *	(*callback): puntero a la funcion que va a procesar el indice
 */
int DB_CrearIndice(char sTabla[DB_LONGITUD_NOMBRE_TABLA], char sIndice[DB_LONGITUD_NOMBRE_TABLA],
		int (*callback)(DB *secondary, const DBT *key, const DBT *data, DBT *result)) {
	int iReturn = 0;

	tipo_tabla_sistema tTabla;
	iReturn = DB_getTabla(sTabla, &tTabla);
	Macro_CheckError(iReturn,"No se encontro la tabla para crear el indice");

	tipo_tabla_sistema tIndice;
	iReturn = DB_getTabla(sIndice, &tIndice);
	Macro_CheckError(iReturn,"No se encontro la tabla para crear el indice");
	// Verifica que la tabla de Indice no se encuentre ya abierta
	if (tIndice.bIndiceAbierto == false) {
		iReturn = tTabla.pdbTabla->associate(tTabla.pdbTabla, NULL, tIndice.pdbTabla, (*callback), DB_CREATE);
		Macro_CheckError(iReturn,"Error al asociar el indice");
		tIndice.bIndiceAbierto = true;
	} else {
		iReturn = -1;
		Macro_CheckError(iReturn,"El Indice ya esta en uso");
	}

	//iReturn = DB_Actualizar_Tabla(tIndice.idTabla, &tIndice );
	//CheckError(iReturn, "Error al actualizar la table en memoria");
	// Actualizat tabla de sistema Obteniendo ID
	iReturn = DB_Actualizar_Tabla(sIndice,&tIndice);
	Macro_CheckError(iReturn,"Error al Actualizar Datos en la Tabla de Sistema");

	return iReturn;
}

/* 
 * FUNCION DEPRECADA, NO SE RECOMIENTA USAR!!
 * Usar funcion DB_consultarCursor
 * Funcion para buscar en un indice
 * 	sIndice: nombre del indice
 *	tBusqueda: tipo de datos abstracto a buscar
 *	iTamano: Tamaño de los datos a buscar
 *	tId: Id del registro encontrado
 */
/*int DB_buscarIndice(char sIndice[DB_LONGITUD_NOMBRE_TABLA], void* tBusqueda, int iTamano, tipo_id *tId,
		u_int32_t iRegistro) {
	DBT data, pkey, skey;
	int iReturn = 0;
	db_recno_t tNumeroReg;

	tipo_tabla_sistema tIndice;
	iReturn = DB_getTabla(sIndice, &tIndice);
	CheckError(iReturn,"No se encontro la tabla al buscar indice");

	if ((iRegistro == 0)) {
		if (tIndice.pdbcTabla != 0) {
			iReturn = tIndice.pdbcTabla->c_close(tIndice.pdbcTabla);
			CheckError(iReturn,"Error al cerrar el cursor del indice");
			tIndice.pdbcTabla = 0;
			iReturn = DB_Actualizar_Tabla(tIndice.idTabla, &tIndice);
			CheckError(iReturn,"Error al actualizar la tabla");
		}
	}

	memset(&skey, 0, sizeof(skey));
	memset(&pkey, 0, sizeof(pkey));
	memset(&data, 0, sizeof(data));
	skey.data = tBusqueda;
	skey.size = iTamano;
	skey.flags = DB_DBT_MALLOC;
	pkey.flags = DB_DBT_MALLOC;
	data.flags = DB_DBT_MALLOC;

	if (iRegistro == 0) {
		iReturn = tIndice.pdbTabla->cursor(tIndice.pdbTabla, NULL, &tIndice.pdbcTabla, 0);
		CheckError(iReturn, "Error al crear el cursor");
		iReturn = DB_Actualizar_Tabla(tIndice.idTabla, &tIndice);
		CheckError(iReturn,"Error al actualizar la tabla");
		iReturn = tIndice.pdbcTabla->c_pget(tIndice.pdbcTabla, &skey, &pkey, &data, DB_SET);
		CheckError(iReturn,"Error al obtener el rango de datos");
	} else {
		switch (iRegistro) {
			case 2:
				iReturn = tIndice.pdbcTabla->c_pget(tIndice.pdbcTabla, &skey, &pkey, &data,
				DB_NEXT_DUP);
				break;
			case 3:
				iReturn = tIndice.pdbcTabla->c_pget(tIndice.pdbcTabla, &skey, &pkey, &data,
				DB_PREV_DUP);
				break;
		}
		if (iReturn != DB_NOTFOUND) {
			CheckError(iReturn,"Error al obtener los datos");
		}
	}


	if (pkey.data != 0) {
		*tId = *((int *) pkey.data);
	}

	iReturn = tIndice.pdbcTabla->c_count(tIndice.pdbcTabla, &tNumeroReg, 0);
	CheckError(iReturn,db_strerror(iReturn));
	
	return (int) tNumeroReg;
}*/

int DB_consultarCursor(char sTabla[DB_LONGITUD_NOMBRE_TABLA], tipo_id *tId, DBT *dbtDatos, t_consulta_datos iFlags, t_tipo_consulta cOpenClose, void *vFiltro, int iSize, int *iCantRegistros) {
	// Defino las variables locales
	DBT data, pkey, skey;
	int iReturn = 0;
	db_recno_t tNumeroReg = 0;

	// Defino un tipo tabla sistema y cargo los datos en funcion del nombre
	tipo_tabla_sistema datosDeTabla;
	iReturn = DB_getTabla(sTabla, &datosDeTabla);
	Macro_CheckError(iReturn, "No se encontro la tabla para consultar el cursor");

	// Solicito abrir un cursor
	if (cOpenClose == ABRIR) {
		// Verifico que no exita uno ya abierto y si esta abierto 
		// lo cierro, dejo la variable en 0 y guardo los datos
		// en la memoria dinamica
		if (datosDeTabla.pdbcTabla != 0) {
			iReturn = datosDeTabla.pdbcTabla->c_close(datosDeTabla.pdbcTabla);
			Macro_CheckError(iReturn, "%s",db_strerror(iReturn));
			datosDeTabla.pdbcTabla = 0;
			iReturn = DB_Actualizar_Tabla(sTabla,&datosDeTabla);
			Macro_CheckError(iReturn,"Error al Actualizar Datos en la Tabla de Sistema");
			//iReturn = DB_Modificar("tablasis", &tTablas, sizeof(tTablas), tablaActual);
			//CheckError(iReturn,"Error al Actualizar Datos en la Tabla de Sistema");
			//iReturn = DB_Actualizar_Tabla(datosDeTabla.idTabla, &datosDeTabla);
			//CheckError(iReturn,"Error al actualizar datos de la Tabla");
		}
		// Si no hubo errores, abro el cursor, y guardo los datos en 
		// la memoria dinamica
		if (iReturn >= 0) {
			iReturn = datosDeTabla.pdbTabla->cursor(datosDeTabla.pdbTabla, NULL, &datosDeTabla.pdbcTabla, 0);
			Macro_CheckError(iReturn, "%s",db_strerror(iReturn));
			iReturn = DB_Actualizar_Tabla(sTabla,&datosDeTabla);
			Macro_CheckError(iReturn,"Error al Actualizar Datos en la Tabla de Sistema");
			//iReturn = DB_Modificar("tablasis", &tTablas, sizeof(tTablas), tablaActual);
			//CheckError(iReturn,"Error al Actualizar Datos en la Tabla de Sistema");
			//iReturn = DB_Actualizar_Tabla(datosDeTabla.idTabla, &datosDeTabla);
			//CheckError(iReturn, "Error al actualizar datos de la Tabla");
		}
		// Si solicito cerrar un cursor abierto
	} else if (cOpenClose == CERRAR) {
		// Verifico que efectivamente este abierto
		if (datosDeTabla.pdbcTabla != 0) {
			// Cierro el cursor, dejo su variable en 0 y guardo
			// los datos en la memoria dinamica
			iReturn = datosDeTabla.pdbcTabla->c_close(datosDeTabla.pdbcTabla);
			Macro_CheckError(iReturn,"Error al cerrar el cursor");
			datosDeTabla.pdbcTabla = 0;
			iReturn = DB_Actualizar_Tabla(sTabla,&datosDeTabla);
			Macro_CheckError(iReturn,"Error al Actualizar Datos en la Tabla de Sistema");
			//iReturn = DB_Modificar("tablasis", &tTablas, sizeof(tTablas), tablaActual);
			//CheckError(iReturn,"Error al Actualizar Datos en la Tabla de Sistema");
			//iReturn = DB_Actualizar_Tabla(datosDeTabla.idTabla, &datosDeTabla);
			//CheckError(iReturn,"Error al actualizar datos de la Tabla");
		}
		// Si ejecuto una consulta
	} else if (cOpenClose == CONSULTAR) {
		// Dejo los espacios de memoria en 0 para elimnar basura
		memset(&skey, 0, sizeof(skey));
		memset(&pkey, 0, sizeof(pkey));
		memset(&data, 0, sizeof(data));
		// Defino que reserve los espacios de memoria
		//skey.flags = DB_DBT_MALLOC;
		//pkey.flags = DB_DBT_MALLOC;
		//data.flags = DB_DBT_MALLOC;
		// Si permite duplicado es un indice, entonces puede buscar con 
		// la funcion c_pget para indices, si no es indice busca con 
		// p_get que es para busqueda en tablas
		if ((iSize != 0) && (vFiltro != NULL) ) {
			// Si pase un parametro para filtrar una tabla indice cargo
			// los parametros para la busqueda
			if (datosDeTabla.bDuplicado == false) {
				iReturn = -1;
				Macro_CheckError(iReturn,"Se pasaron parametros de busqueda a una tabla que no es un indice");
			}
			skey.data = vFiltro;
			skey.size = iSize;

			iReturn = datosDeTabla.pdbcTabla->c_pget(datosDeTabla.pdbcTabla, &skey, &pkey, &data, iFlags);
		} else {
			iReturn = datosDeTabla.pdbcTabla->c_get(datosDeTabla.pdbcTabla, &pkey, &data, iFlags);
		}
		if (iReturn != DB_NOTFOUND) {
		//printf(db_strerror(iReturn));
			Macro_CheckError(iReturn,"Error al obtener datos del cursor");


			// Antes de Devolver por referencia reviso que tenga datos validos para devolver
			if ((pkey.data != 0) && (tId != NULL)) {
				*tId = *((int *) pkey.data);
			}

			// Antes de Devolver por referencia reviso que tenga datos validos para devolver
			if ((data.data != 0) && (dbtDatos != NULL)) {
				*dbtDatos = data;
			}

			// En caso de que se usen cursores con indices, devuelve la
			// cantidad de registros que se encontraron

			iReturn = datosDeTabla.pdbcTabla->c_count(datosDeTabla.pdbcTabla, &tNumeroReg, 0);
			Macro_CheckError(iReturn, "%s",db_strerror(iReturn));

			if (iCantRegistros != NULL) {
				*iCantRegistros = (int) tNumeroReg;
			}
		} else {
			// El registro no se encontro, entonces la cantidad de registros es 0
			//iReturn = 0;
			if (tId != NULL) {
				*tId = 0;
			}

			if (iCantRegistros != NULL) {
				*iCantRegistros = 0;
			}
		}

	}
	return iReturn;
}

int DB_consultarDosIndicesCursor(char sTabla[DB_LONGITUD_NOMBRE_TABLA],
								 char sIndice1[DB_LONGITUD_NOMBRE_TABLA],
								 char sIndice2[DB_LONGITUD_NOMBRE_TABLA],
								 tipo_id PrimerFiltro, int SegundoFiltro, tipo_id *tId, bool NuevaConsulta) {
	// Defino las variables locales
	DBT data, key;
	int iReturn = 0;

	// Defino al Cursos que va a contener los datos como Static para que no pierda la informacion
	static DBC *pcdbConsulta;
	if ((pcdbConsulta != NULL) && (NuevaConsulta == false)) {
		memset(&key,0,sizeof(key));
		memset(&data,0,sizeof(data));
		// continuo devolviendo el conjunto de resultados
		iReturn = pcdbConsulta->get(pcdbConsulta, &key, &data, 0);
	} else {
		// comienzo el proceso de consulta
		if (pcdbConsulta !=NULL) {
			iReturn = pcdbConsulta->c_close(pcdbConsulta);
			pcdbConsulta = NULL;
		}
		// Defino un tipo tabla sistema y cargo los datos en funcion del nombre
		// Necesito la Base de datos mas los 2 indices
		tipo_tabla_sistema datosDeTabla, datosDeIndice1, datosDeIndice2;
		iReturn = DB_getTabla(sTabla, &datosDeTabla);
		Macro_CheckError(iReturn, "No se encontro la tabla para consultar el cursor");
		iReturn = DB_getTabla(sIndice1, &datosDeIndice1);
		Macro_CheckError(iReturn, "No se encontro la tabla para consultar el cursor");
		iReturn = DB_getTabla(sIndice2, &datosDeIndice2);
		Macro_CheckError(iReturn, "No se encontro la tabla para consultar el cursor");

		// Defino los cursores y el array de cursores
		DBC *carray[3];

		// Limpio las variables de memoria DBT para que no tengan basura
		memset(&data,0,sizeof(data));
		memset(&key,0,sizeof(key));
		//key.flags = DB_DBT_MALLOC;
		//data.flags = DB_DBT_MALLOC;

		// TODO: IMPORTANTE!!! Pasar Size usando _Generic, si es int, puntero o string, si es string usar strlen+1
		// Asigno los campos de filtro
		key.data = &PrimerFiltro;
		key.size = sizeof(PrimerFiltro);

		// Posiciono el cursor en los registros que tengo que consultar
		iReturn = datosDeIndice1.pdbcTabla->get(datosDeIndice1.pdbcTabla, &key, &data, DB_SET);
		Macro_CheckError(iReturn,"No se pudo ejecutar la consulta sobre el Indice");

		memset(&key,0,sizeof(key));
		//key.flags = DB_DBT_MALLOC;

		key.data = &SegundoFiltro;
		key.size = sizeof(SegundoFiltro);

		iReturn = datosDeIndice2.pdbcTabla->get(datosDeIndice2.pdbcTabla, &key, &data, DB_SET);
		Macro_CheckError(iReturn,"No se pudo ejecutar la consulta sobre el cursor");

		// Setup de cursor Array, el ultimo valor es NULL
		carray[0] = datosDeIndice1.pdbcTabla;
		carray[1] = datosDeIndice2.pdbcTabla;
		carray[2] = NULL;

		// Creo el cursor de consultas cruzadas, se podria pasar el parametro DB_JOIN_NOSORT
		iReturn = datosDeTabla.pdbTabla->join(datosDeTabla.pdbTabla, carray, &pcdbConsulta, 0);
		Macro_CheckError(iReturn,"No se pudo crear el Curzor de consulta Join");

		memset(&key,0,sizeof(key));

		// Paso los datos para recorrer el cursor, el ultimo parametro para este cursor en particular de tipo Join siempre es 0
		// y devuelvo el primer resultado
		iReturn = pcdbConsulta->get(pcdbConsulta, &key, &data, 0);
	}

	if (iReturn == DB_NOTFOUND) {
		// se llego al ultimo registro entonces cierro el cursor
		// Cierro los cursores
		iReturn = pcdbConsulta->c_close(pcdbConsulta);
		Macro_CheckError(iReturn,"No se puede cerrar el cursor");
		iReturn = DB_NOTFOUND;
		pcdbConsulta = NULL;
		*tId = 0;
	} else {
		Macro_CheckError(iReturn,"Error al realizar la consulta sobre el Cursor");
		// Devulevo el Id encontrado
		if ((key.data != 0) && (tId != NULL)) {
			*tId = *((int*) key.data);
		}
	}
	return iReturn;
}


/*
 * Funcion rapida (sin mucho codigo) para formatear
 * requiere cerrar todas las tablas antes con syncclose
 * despues de llamarla hacer un TablasSistema y crear los indices nuevamente
 */
/*
 * Comentado por Julian 06/06/2015
 * Funcion Deprecada
int DB_QuickFormat() {
	int iReturn;
	// Define el puntero del entorno de datos, y le da un valor
	DB_ENV *pdeFileSystem;
	//pdeFileSystem = (DB_ENV *)malloc(sizeof(DB_ENV));
	iReturn = DB_CrearEntorno(&pdeFileSystem);
	if (iReturn != 0) {
		LogErrores(NULL, "", "No se pudo abrir el Entorno de Datos");
		return iReturn;
	}
	pdeFileSystem->close(pdeFileSystem, 0);
	unlink("filesys/__db.001");
	unlink("filesys/__db.002");
	unlink("filesys/__db.003");
	unlink("filesys/dir.db");
	unlink("filesys/dir.idx");
	unlink("filesys/dirpadre.idx");
	unlink("filesys/files.db");
	unlink("filesys/tablasis.db");
	unlink("filesys/files.idx");
	unlink("filesys/nodo.db");
	unlink("filesys/nodo.idx");
	return iReturn;
}
*/

/*
 * Funcion para formatear la base de datos
 * 	Tener en cuenta de armar indices de nuevo despues de formatear
 */
int DB_FormatFS() {
	tipo_tabla_sistema tTablas;
//	tipo_id tIdTabla;
	int iReturn = 0;
//	bool bCursorEstabaAbierto;

	int tablaActual, iTotalTablas;
	DBT tDatos;

	DB_getUltimoId("tablasis",&iTotalTablas);
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
	for (tablaActual = 1; tablaActual < iTotalTablas; tablaActual++) {
#pragma GCC diagnostic pop
//		bCursorEstabaAbierto = false;
		iReturn = DB_Consultar("tablasis",tablaActual,&tDatos);
		Macro_CheckError(iReturn,"No se pudo consultar en la Tabla de Sistema");
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
		tTablas = *((tipo_tabla_sistema *) tDatos.data);
#pragma GCC diagnostic pop

/*		if (tTablas.pdbcTabla != 0) {
			//Cerrar el Cursor
			iReturn = DB_consultarCursor(tTablas.sTabla, NULL, NULL, 0, 2, NULL, 0, NULL);
			CheckError(iReturn,"Error al consultar el cursor");
			//Pongo que el cursor estaba abierto
			bCursorEstabaAbierto = true;
		}
*/
		if (tTablas.bFormatear == true) {
			tTablas.iCantReg = 0;
			tTablas.idUltimoRegistro = 1;
			iReturn = tTablas.pdbTabla->truncate(tTablas.pdbTabla, NULL, NULL, DB_AUTO_COMMIT);
			Macro_CheckError(iReturn, "%s",db_strerror(iReturn));

/*		if (bCursorEstabaAbierto == true) {
			//Cerrar el Cursor
			iReturn = DB_consultarCursor(tTablas.sTabla, NULL, NULL, 0, 1, NULL, 0, NULL);
			CheckError(iReturn,"Error al consultar el cursor");
		}
*/
			iReturn = DB_Actualizar_Tabla(tTablas.sTabla,&tTablas);
			Macro_CheckError(iReturn,"Error al Actualizar Datos en la Tabla de Sistema");
			//DB_AbrirTabla(tTablas.sTabla);
		}
	}
	return iReturn;



	//---------------Leo Modifico Aca---------------

/*	tipo_tabla_sistema tTablas;
	int iReturn = 0;
	int tablaActual;

	u_int32_t iCantidadRegistrosEliminados = 0;
	bool bCursorEstabaAbierto = false;

	// Define el puntero del entorno de datos, y le da un valor
	DB_ENV *pdeFileSystem;
	//pdeFileSystem = (DB_ENV *)malloc(sizeof(DB_ENV));
	iReturn = DB_CrearLeerEntorno(&pdeFileSystem);
	CheckError(iReturn,"No se pudo abrir el entorno de datos");

	//Recorro Todas las Tablas del Sistema
	//Arranca la tabla 2 porque es la Tabla del Sistema
	for (tablaActual = 2; tablaActual <= MAXCTABLAS; tablaActual++) {

		//Hago que en mi estructura auxiliar tTablas cargue la tabla actual que estoy recorriendo
		// Cambiar Funcion
		//		iReturn = DB_Leer_Tabla(tablaActual, &tTablas);
		CheckError(iReturn,"Error al leer datos de las tablas");

		//Verifico si la tabla tiene nombre vacio (\0) , porque eso significa que es la ultima tabla del sistema
		if(tTablas.sTabla[0]=='\0'){
			//Salgo del For
			break;
		}


		//Reviso que sea Una Tabla  y No un Indice (cDuplicado = 0 significa Tabla, osea que no acepta duplicados, porque los indices SI aceptan duplicados)
		if (tTablas.cDuplicado == 0) {

			//Veo Si esta abiero el indice, para cerrarlo
			if (tTablas.pdbcTabla != 0) {
				//Cerrar el Cursor
				iReturn = DB_consultarCursor(tTablas.sTabla, NULL, NULL, 0, 2, NULL, 0, NULL);
				CheckError(iReturn,"Error al consultar el cursor");

				//Pongo que el cursor estaba abierto
				bCursorEstabaAbierto = true;

			} else {
				//Esta cerrado el cursor

				//iReturn = pdeFileSystem->dbremove(pdeFileSystem, NULL,tTablas.sNombreArch, "", 0);

				//Supuestamente Elimina los Datos de Tabla sin borrar el archivo ni modificar los indices
				iReturn = tTablas.pdbTabla->truncate(tTablas.pdbTabla, NULL, &iCantidadRegistrosEliminados, 0);
				CheckError(iReturn, db_strerror(iReturn));		
		
				//Vuelvo a poner el puntero de ultimo registro de la tabla en 1 (empieza en 1 no en 0 los registros de la tabla)
				tTablas.idUltimoRegistro = 1;

				//Actualizo en memoria la tabla que le dije el que vuelva al 1er registro
				//iReturn = DB_Actualizar_Tabla(tTablas.idTabla, &tTablas);
				CheckError(iReturn, "Error al actualizar la tabla");

				//Actualizo a disco lo que modifique
				iReturn = DB_Modificar("tablasis", &tTablas, sizeof(tTablas), tablaActual);
				CheckError(iReturn,"Error al modificar tablasis");

			}

			//Si el cursor estaba cerrado no debo reabrirlo, si estaba abierto (yo lo cerre) debo reabrirlo
			if (bCursorEstabaAbierto == true) {
				//Abro el Cursor a la tabla
				iReturn = DB_consultarCursor(tTablas.sTabla, NULL, NULL, 0, 1, NULL, 0, NULL);
				CheckError(iReturn,"Error con el cursor");
			}
		}
	}

	return 0;
	//---------------Termine de Modificar----------------
	 */
}

int DB_CerrarEntorno() {
	int iReturn;
	DB_ENV *pdeFileSystem;
	iReturn = DB_CrearLeerEntorno(&pdeFileSystem);
	Macro_CheckError(iReturn,"Error al obtener el entorno\n");
	pdeFileSystem->close(pdeFileSystem,0);
	return iReturn;
}
