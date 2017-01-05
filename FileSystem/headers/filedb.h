//Definicion de funciones y estructuras para manejo de Archivos
//Deben ser publicas ya que se utilizan externamente

#ifndef FILEDB_H_
#define FILEDB_H_

#include <commons/bitarray.h>

#include "fsdb.h"

#include "../../Biblioteca_Comun/Biblioteca_Comun.h"	//Para las Constantes

typedef struct  __attribute__ ((__packed__)) {
	char 		sNombre[FILE_LONGITUD_NOMBRE];
	tipo_id 	dirPadre;
	char 		md5[MD5_LENGTH];
	bool		disponible;
	t_bitarray*	tBloquesActivos; // Tipo de datos BitSet, con un bitset para la cantidad de bloques del archivo, asi verifico si estan todos en 1 que el archivo esta disponible, sin importarme el nodo.
	uint 		iCantBloques;
	ulong 		tamanioArchivo;
} tipo_datos_file;

int FILE_Crear(tipo_datos_file tArch, tipo_id *tId);

int FILE_Renombrar(tipo_id idArchivoActual, char sNombreArchivoNuevo[FILE_LONGITUD_NOMBRE]);

int FILE_Eliminar(char sNombreArchivo[FILE_LONGITUD_NOMBRE]);
int FILE_EliminarId(tipo_id tIdArchivo);

int FILE_Mover(tipo_id idArchivoPorMover, tipo_id tIdCarpetaNueva);

int FILE_ConsultarId(tipo_id tIdArchivo, tipo_datos_file *tArchivo);
int FILE_Copiar(char sNombreArchivo[FILE_LONGITUD_NOMBRE], char sNombreArchivoCopiado[FILE_LONGITUD_NOMBRE],
		tipo_id tIdCarpetaNueva);

//Abre los Indices Necesarios para Trabajar (en la Base de Datos) con Archivos
//Devuelve Numero Positivo o 0 en Exito. Devuelve un Numero Negativo en Error
int FILE_AbrirIndices();
int FILE_CerrarIndices();

int FILE_BuscarNombre(char sNombreArchivo[FILE_LONGITUD_NOMBRE], tipo_id *tId, int iRegistro, int *iCantRegistros);
int FILE_BuscarPorPadre(tipo_id tIdPadre, tipo_id *tId, int iRegistro, int *iCantRegistros);

//Dado el Nombre del Archivo, se fija si esta Disponible o no y devuelve un Booleano.
bool FILE_estaDisponibleArchivo(char* archivoNombre);

int FILE_PonerNoDisponible(tipo_id tIdArch);

int FILE_PonerDisponible(tipo_id tIdArch, t_bitarray *tBloquesActivos, int iCantBloques);

int FILE_Modificar(tipo_id tIdArch, tipo_datos_file tDatosArch);

#endif
//FILEDB_H_

