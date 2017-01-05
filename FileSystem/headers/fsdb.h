#ifndef FSDB_H_
#define FSDB_H_

#include "../../lib/include/db.h"
#include <stdbool.h>

//Maxima cantidad posible de tablas
//#define MAXCTABLAS 32
#define DB_LONGITUD_NOMBRE_TABLA 20
#define DB_LONGITUD_ARCHIVO_TABLA 25

 //Enums para el manejo de Id
typedef int tipo_id;

typedef enum {
	CURRENT = DB_CURRENT,
	NEXT = DB_NEXT,
	PREV = DB_PREV,
	LAST = DB_LAST,
	FIRST = DB_FIRST,
	NEXT_IN_RANGE = DB_NEXT_DUP,
	PREV_IN_RANGE = DB_PREV_DUP
} t_consulta_datos;

//Enum para las Opciones en "DB_consultarCursor"
typedef enum {
	CONSULTAR = 0,
	ABRIR = 1,
	CERRAR = 2
} t_tipo_consulta;

//Enum para las Opciones en "DB_synccloseTablas"
typedef enum {
	SYNC = 0,
	CLOSE = 1
} t_tipo_syncclose;
/*
 * Funcion que escribe los datos en el disco o cierra la Tabla
 *      sTabla: nombre de la tabla
 *      cClose: 0 escribe datos, 1 cierra la tabla
*/
int DB_synccloseTablas(char sTabla[DB_LONGITUD_NOMBRE_TABLA],t_tipo_syncclose cClose);

/*
 * Funcion que inicializa el entorno de datos y las Tablas o las crea de
 * no existir previamente
*/
int DB_TablasSistema();

/*
 * Funcion para insertar datos en una Tabla
 *      sTabla: nombre de la Tabla
 *      tDatos: tipo de datos abstracto, puede ser int, char, struct...
 *      iTamano: tamoño en bytes de los datos
*/
int DB_Insertar(char sTabla[DB_LONGITUD_NOMBRE_TABLA], void* tDatos, int iTamano, tipo_id *tId);

/*
 * Funcion para modificar datos en una Tabla
 *      sTabla: nombre de la Tabla
 *      tDatos: tipo de datos abstracto, puede ser int, char, struct...
 *      iTamano: tamoño en bytes de los datos
 *      tId: Id del registro que se va a Actualizar
*/
int DB_Modificar(char sTabla[DB_LONGITUD_NOMBRE_TABLA], void* tDatos, int iTamano, tipo_id tId);

/*
 * Funcion para eliminar datos en una Tabla
 *      sTabla: nombre de la Tabla
 *      tId: Id del registro que se va a Actualizar
*/
int DB_Eliminar(char sTabla[DB_LONGITUD_NOMBRE_TABLA], tipo_id tId);

/*
 * Funcion para consultar datos en una Tabla
 *      sTabla: nombre de la Tabla
 *      tId: Id del registro que se va a buscar
 *      dbtDatos: puntero al registro tipo DBT que contiene los datos
*/
int DB_Consultar(char sTabla[DB_LONGITUD_NOMBRE_TABLA], tipo_id tId, DBT* dbtDatos);

/*
 * Funcion para asociar una tabla con otra y crear un indice
 *      sTabla: nombre de la tabla que contiene los datos
 *      sIndice: nombre de la tabla que va a ser indice
 *      (*callback): puntero a la funcion que va a procesar el indice
*/
int DB_CrearIndice(char sTabla[DB_LONGITUD_NOMBRE_TABLA], char sIndice[DB_LONGITUD_NOMBRE_TABLA], int (*callback)(DB *secondary,const DBT *key, const DBT *data, DBT *result));

/* 
 * Funcion para buscar en un indice
 *      sIndice: nombre del indice
 *      tBusqueda: tipo de datos abstracto a buscar
 *      iTamano: Tamaño de los datos a buscar
 *      tId: Id del registro encontrado
 * 	iRegistro: 0 una busqueda nueva, 1 primero, 2 siguiente, 3 anterior
 *		   4 ultimo
 *	devuelve < 0 error, 0 exitoso, >0 cantidad de registros
*/
//int DB_buscarIndice(char sIndice[DB_LONGITUD_NOMBRE_TABLA], void* tBusqueda, int iTamano, tipo_id *tId, u_int32_t iRegistro);

int DB_getUltimoId(char sTabla[DB_LONGITUD_NOMBRE_TABLA], tipo_id *tIdUltimo);
int DB_NumeroRegistros(char sTabla[DB_LONGITUD_NOMBRE_TABLA]);

/*
 * Funcion para consultar datos en una Tabla mediante un cursor
 *      sTabla: nombre de la Tabla o Indice
 *      tId: Id del registro que se va a devolver, puede ser 0
 *      dbtDatos: puntero al registro tipo DBT que contiene los datos, puede ser NULL
 *      iFlags: Flags de la base Berkeley pueden ser
 *		DB_SET: busca un registro en indice o tabla,si el registro esta duplicado, busca el primer duplicado
 *		DB_FIRST: devuelve el primer registro, no importa si hay filtro
 *		DB_LAST: devuelve el ultimo registro, no importa si hay filtro
 *		DB_NEXT: devuelve el siguiente registro, sin filtrar
 *		DB_PREV: devuelve el registro anterior, sin filtrar
 *		DB_NEXT_DUP: devuelve el siguiente registro del RANGE
 *		DB_PREV_DUP: devuelve el registro anterior del RANGE
 *		DB_CURRENT: trabaja sobre el registro actual
 *	cOpenClose: 0 para consulta, 1 para abrir, 2 para cerrar
 *	vFiltro: Campo de cualquier tipo para buscar
 *	iSize: Tamaño del campo para buscar
 * return: Numero de Registros si se aplica RANGE, 0 si no hay error
 *	   < 0 con errores
*/
//int DB_consultarCursor(char sTabla[DB_LONGITUD_NOMBRE_TABLA], tipo_id *tId, DBT *dbtDatos, u_int32_t iFlags, char cOpenClose, void *vFiltro, int iSize);
int DB_consultarCursor(char sTabla[DB_LONGITUD_NOMBRE_TABLA], tipo_id *tId, DBT *dbtDatos, t_consulta_datos iFlags, t_tipo_consulta cOpenClose, void *vFiltro, int iSize, int *iCantRegistros);

int DB_FormatFS();

int DB_consultarDosIndicesCursor(char sTabla[DB_LONGITUD_NOMBRE_TABLA],
		char sIndice1[DB_LONGITUD_NOMBRE_TABLA],
		char sIndice2[DB_LONGITUD_NOMBRE_TABLA], tipo_id PrimerFiltro,
		int SegundoFiltro, tipo_id* tId, bool NuevaConsulta);

int DB_CerrarEntorno();
#endif
//FSDB_H_
