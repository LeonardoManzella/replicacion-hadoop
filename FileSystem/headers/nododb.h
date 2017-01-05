//Definicion de funciones y estructuras para manejo de Nodos
//Deben ser publicas ya que se utilizan externamente

#ifndef NODODB_H_
#define NODODB_H_

#include "fsdb.h"
#include <commons/bitarray.h>

#include "../../Biblioteca_Comun/Biblioteca_Comun.h" //Para las Constantes


/* 
 * Definicion de estructura de Directorio 
 * Debe ser publica ya que despues se van a necesitar crear variables con ella
*/

typedef struct __attribute__ ((__packed__))  {
	char			sNombre[NODO_LONGITUD_NOMBRE]; 		// Nombre del directorio
	char			nodoIP[LONGITUD_CHAR_IP]; 						// Direccion IP para Socket
	char			nodoPuerto[LONGITUD_CHAR_PUERTOS];
	bool			activo; 															// Indica si el nodo esta activo
	t_bitarray* tBloquesNodo;
	int			bloques_totales;
	int			bloques_usados;
	int			bloques_libres;
	int			bloques_reservados;
} tipo_datos_nodo;

/*
 * Funcion para devolver la estructura de un directorio conociendo el Id
 * 	tId: Id del directorio
 *	tDir: puntero de estructura de directorio donde se devuelve los datos
*/
int NODO_Consultar(tipo_id tId,tipo_datos_nodo* tNodo);

/*
 * Funcion para crear un Nodo
 * 	sNodo: Nombre del Nodo a crear
 *	nodoIP: IP del Nodo
 *	nodoPuerto: Puerto del Nodo
*/
int NODO_Nuevo(char sNodo[NODO_LONGITUD_NOMBRE], char* nodoIP, char* nodoPuerto, int iBloquesTot);

/*
 * Funcion para cambiar la ubicacion de un directorio (el padre)
 * 	sDir: Nombre del directorio
 *	sNuevoPadre: String con el nombre del nuevo Padre
*/

int NODO_Eliminar(char sNodo[NODO_LONGITUD_NOMBRE]);

//Abre los Indices Necesarios para Trabajar (en la Base de Datos) con Nodos
int NODO_AbrirIndices();
int NODO_CerrarIndices();

/*
 * Funcion para buscar por nombre un directorio (el campo a buscar esta
 * 	definido en el indice, y devuelve el Id
 *	sDir: strin con el nombre del directorio a buscar
 *	tId: puntero para que devuelva el Id del Directorio
 *	iRegistro: Parametro para recorrer en caso de duplicados, los valores
 *		posibles son 0,1,2,3,4 (NUEVO, PRIM, SIG, ANT, ULT)
*/
int NODO_BuscarNombre(char sNodo[NODO_LONGITUD_NOMBRE],tipo_id *tId, int iRegistro, int *iCantRegistros);

int NODO_BuscarEspacio(tipo_id *tId, int iIdAnt);

int NODO_Modificar(tipo_id tIdNodo, tipo_datos_nodo tNodo);

#endif
//NODODB_H_

