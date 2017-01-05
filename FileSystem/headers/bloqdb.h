//Definicion de funciones y estructuras para manejo de Bloques contenidos dentro de Nodos
//Deben ser publicas ya que se utilizan externamente

#ifndef BLOQDB_H_
#define BLOQDB_H_

#include "stdbool.h"

#include "../headers/fsdb.h"
#include "../../Biblioteca_Comun/Biblioteca_Comun.h"

typedef struct  __attribute__ ((__packed__)) {
	tipo_id		tIdArchivo;
	int			numeroBloqueDentroArchivo;
	char		copia;
	tipo_id		tIdNodo;
	int			numeroBloqueDentroNodo;
	bool		disponible;
	size_t		tamano; 	//Tamanio del Bloque Cargado (Tamanio REAL de los Datos Utiles dentro de los 20mb, No lo usamos para Nada pero es bueno tenerlo guardado)
	char		md5[MD5_LENGTH];
	//Key = tIdArchivo+tIdNodo+numeroBloqueDentroArchivo
} tipo_datos_bloques;

//Dado el ID del Bloque y los Datos del Bloque, busca y actualiza en la Base de Datos con estos Datos
//Devuelve Numero Positivo o 0 en Exito. Devuelve un Numero Negativo en Error
int BLOQUES_Modificar(tipo_id tId, tipo_datos_bloques tBloque);

//Devuelve por Referencia la estructura de un Bloque conociendo el Id de este
//Devuelve Numero Positivo o 0 en Exito. Devuelve un Numero Negativo en Error
int BLOQUES_Consultar(tipo_id tId,tipo_datos_bloques* tBloques);

//Crea un Bloque Dentro de un Nodo y devuelve por Referencia el ID del Bloque Creado
//Devuelve Numero Positivo o 0 en Exito. Devuelve un Numero Negativo en Error
int BLOQUES_Nuevo(tipo_datos_bloques tBloque, tipo_id* tId);

//Dado el ID del Bloque lo elimina de la Base de Datos
//Devuelve Numero Positivo o 0 en Exito. Devuelve un Numero Negativo en Error
int BLOQUES_Eliminar(tipo_id tIdBloque);

//Abre los Indices Necesarios para Trabajar (en la Base de Datos) con Bloques dentro de Nodos
//Devuelve Numero Positivo o 0 en Exito. Devuelve un Numero Negativo en Error
int BLOQUES_AbrirIndices();

//Cierra todos los Indices relacionados Bloques
//Devuelve Numero Positivo o 0 en Exito. Devuelve un Numero Negativo en Error
int BLOQUES_CerrarIndices();

/*	Busca un Bloque dado el ID del Nodo que lo contiene, devuelve por referencia el ID del Bloque y la Cantidad de Bloques encontrados para ese Nodo
 *	iRegistro: Parametro para recorrer en caso de Multiples Bloques para el Mismo Nodo,
 *	los valores posibles son: DB_SET (Obtener el primero que haya)   DB_NEXT_DUP (Despues de usar DB_SET, obtiene el siguiente Bloque que haya)
 *	Devuelve Numero Positivo o 0 en Exito. Devuelve un Numero Negativo en Error */
int BLOQUES_buscarBloqueNodo(tipo_id tIdNodo, tipo_id *tId, int iRegistro, int *iCantRegistros);

/*	Busca un Bloque dado el ID del Archivo que lo contiene, devuelve por referencia el ID del Bloque y la Cantidad de Bloques encontrados para ese Nodo
 *	iRegistro: Parametro para recorrer en caso de Multiples Bloques para el Mismo Nodo,
 *	los valores posibles son: DB_SET (Obtener el primero que haya)   DB_NEXT_DUP (Despues de usar DB_SET, obtiene el siguiente Bloque que haya)
 *	Devuelve Numero Positivo o 0 en Exito. Devuelve un Numero Negativo en Error */
int BLOQUES_buscarBloqueArchivo(tipo_id idArchivo, tipo_id* idBloqueBuscado, int iRegistro, int *iCantRegistros);

/*	Busca un Bloque dado el ID del Archivo que lo contiene y El Numero de Bloque buscado, devuelve por referencia el ID del Bloque y la Cantidad de Bloques encontrados para ese Nodo
 *	iRegistro: Parametro para recorrer en caso de Multiples Bloques para el Mismo Nodo,
 *	los valores posibles son: DB_SET (Obtener el primero que haya)   DB_NEXT_DUP (Despues de usar DB_SET, obtiene el siguiente Bloque que haya)
 *	Devuelve Numero Positivo o 0 en Exito. Devuelve un Numero Negativo en Error */
int BLOQUES_buscarBloqueEspecificoArchivo(tipo_id idArchivo, int numeroBloqueDentroArchivo, tipo_id* idBloqueBuscado, bool NuevaConsulta);

#endif
//BLOQDB_H_

