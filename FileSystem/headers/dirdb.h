//Definicion de funciones y estructuras para manejo de Directorios/Carpetas
//Deben ser publicas ya que se utilizan externamente

#ifndef DIRDB_H_
#define DIRDB_H_

#include "fsdb.h"


#include "../../Biblioteca_Comun/Biblioteca_Comun.h"	//Para las Constantes



//Struct Necesario para Manejar Directorios/Carpetas en la Base de Datos
typedef struct  __attribute__ ((__packed__)) {
        char    sNombre[DIR_LONGITUD_NOMBRE]; 	// Nombre del directorio
        int     iPadre; 						// id del Padre, 0 si esta en la raiz
} tipo_datos_dir;

//Devuelve por Referencia la estructura de un Directorio conociendo el Id de este
//Devuelve Numero Positivo o 0 en Exito. Devuelve un Numero Negativo en Error
int DIR_Consultar(tipo_id tId,tipo_datos_dir* tDir);


/*	Crea un Directorio
 * 	sDir: Nombre del directorio a crear
 *	sPadre: Nombre del padre donde se crea, "" para la raiz
 *	Devuelve Numero Positivo o 0 en Exito. Devuelve un Numero Negativo en Error */
int DIR_Crear(char sDir[DIR_LONGITUD_NOMBRE], char sPadre[DIR_LONGITUD_NOMBRE]);

/* 	Crea un Directorio (pero con el ID del padre en vez del nombre)
 * 	sDir: Nombre del directorio a crear
 *	padre: ID del padre donde se crear, 0 para la raiz
 *	tId: puntero en el cual devuelve el id recien creado
 *	Devuelve Numero Positivo o 0 en Exito. Devuelve un Numero Negativo en Error */
int DIR_CrearConId(char sDirName[DIR_LONGITUD_NOMBRE], int padre, tipo_id* tId);

/*	Funcion para cambiar el nombre de un directorio (sin cambiar la ubicacion)
 * 	sDirViejo: Nombre del directorio a cambiar
 *	sDirNuevo: Nuevo nombre del directorio
 *	Devuelve Numero Positivo o 0 en Exito. Devuelve un Numero Negativo en Error */
int DIR_Renombrar(char sDirViejo[DIR_LONGITUD_NOMBRE], char sDirNuevo[DIR_LONGITUD_NOMBRE]);

/*	Funcion para cambiar el nombre de un directorio (sin cambiar la ubicacion) a partir del id
 * 	idCarpeta: id del directorio a cambiar
 *	sDirNuevo: Nuevo nombre del directorio
 *	Devuelve Numero Positivo o 0 en Exito. Devuelve un Numero Negativo en Error */
int DIR_RenombrarConId(tipo_id idCarpeta, char sDirNuevo[DIR_LONGITUD_NOMBRE]);

/*	Funcion para cambiar la ubicacion de un directorio (cambia el padre)
 * 	sDir: Nombre del directorio
 *	sNuevoPadre: String con el nombre del nuevo Padre
 *	Devuelve Numero Positivo o 0 en Exito. Devuelve un Numero Negativo en Error */
int DIR_Mover(char sDir[DIR_LONGITUD_NOMBRE], char sNuevoPadre[DIR_LONGITUD_NOMBRE]);

/*	Funcion para cambiar la ubicacion de un directorio (cambia el padre) a partir del id
 * 	aMoverId: id del directorio a mover
 *	detinoId: id del nuevo Padre
 *	Devuelve Numero Positivo o 0 en Exito. Devuelve un Numero Negativo en Error */
int DIR_MoverConId(tipo_id aMoverId, tipo_id detinoId);

//Funcion para eliminar un directorio pasando su ID
//Devuelve Numero Positivo o 0 en Exito. Devuelve un Numero Negativo en Error
int DIR_EliminarConId(int id);

//Funcion para eliminar un directorio pasando su Nombre
//Devuelve Numero Positivo o 0 en Exito. Devuelve un Numero Negativo en Error
int DIR_Eliminar(char sDir[DIR_LONGITUD_NOMBRE]);

//Abre los Indices Necesarios para Trabajar (en la Base de Datos) con Directorios
//Devuelve Numero Positivo o 0 en Exito. Devuelve un Numero Negativo en Error
int DIR_AbrirIndices();

//Cierra todos los Indices relacionados Directorios
//Devuelve Numero Positivo o 0 en Exito. Devuelve un Numero Negativo en Error
int DIR_CerrarIndices();

/*	Busca un Directorio dado el Nombre, devuelve por referencia el ID del Directorio y la Cantidad de Directorios encontrados con el Mismo Nombre
 *	iRegistro: Parametro para recorrer en caso de Multiples Directorios para el Mismo Nombre,
 *	los valores posibles son: DB_SET (Obtener el primero que haya)   DB_NEXT_DUP (Despues de usar DB_SET, obtiene el siguiente Bloque que haya)
 *	Devuelve Numero Positivo o 0 en Exito. Devuelve un Numero Negativo en Error */
int DIR_buscarId(char sDir[DIR_LONGITUD_NOMBRE],tipo_id *tId, int iRegistro, int *iCantRegistros);

/*	Busca un Directorio Hijo dado el ID su Directorio Padre, devuelve por referencia el ID del Directorio Hijo y la Cantidad de Directorios Hijos encontrados para ese Padre
 *	iRegistro: Parametro para recorrer en caso de Multiples Directorios Hijos para el Mismo Directorio Padre,
 *	los valores posibles son: DB_SET (Obtener el primero que haya)   DB_NEXT_DUP (Despues de usar DB_SET, obtiene el siguiente Bloque que haya)
 *	Devuelve Numero Positivo o 0 en Exito. Devuelve un Numero Negativo en Error */
int DIR_buscarIdPadre(tipo_id tIdPadre, tipo_id *tId, int iRegistro, int *iCantRegistros);



//Verifica que los componentes de una ruta existan, devuelve el id del ultimo componente o -1 si algun
//   directorio en la ruta no existe. Verifica a partir del StardID (0 para root "/", que seria el valor por defecto si no queres buscar usando StartID)
int DIR_validarRuta(char* ruta, int startID);

#endif
//DIRDB_H_

