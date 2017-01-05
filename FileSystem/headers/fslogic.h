//Definicion de funciones para manejo del File System. Permiten Manejar tanto Archivos,Directorios/Carpetas,Nodos y Bloques dentro de los Nodos
//Deben ser publicas ya que se utilizan externamente

#ifndef FSLOGIC_H_
#define FSLOGIC_H_

#include "filedb.h"
#include "dirdb.h"
#include "nododb.h"
#include "bloqdb.h"

#include "Biblioteca_Bloques.h"	  //Para la funciones de Manejo del "tipo_bloque"
#include "../../Biblioteca_Comun/Biblioteca_Comun.h"
/*
typedef enum {
	FS_NODO_RESET = -1,
	FS_NODO_LISTAR = 0,
	FS_NODO_AGREGAR = 1,
	FS_NODO_ELIMINAR = 2,
	FS_NODO_CONTAR = 3
} t_operacion_nodo;
*/

#define CANTIDAD_COPIAS_MINIMAS 3


/* Dada la ruta (dentro de Linux) de un archivo, se encarga de leerlo y cargar sus bloques (y cargar el MD5) al File System en el Directorio Indicado.
En caso de ejecucion correcta devuelve 0.
En caso de un error al leer los bloques, para la ejecucion y devuelve el un numero de error negativo.
La funcion YA se encarga de imprimir con "perror" para cualquier tipo de error que haya (en realidad lo hace la funcion "Bloques_obtener_desde_archivo_texto" que la estoy usando dentro de esta funcion).
	Los numeros de error que devuelven son:
		-1 si el archivo esta vacio (tamaño 0).
		-2 si hubo un error al leer los bloques. */
int FS_CargarCompletoArchivo( const char* sRutaArchivo,  const char* md5, const tipo_id tIdDir, bool showProgress );
int FS_CargarBloqueDeArchivo2(tipo_id tIdArch, tipo_id *IdNodos, tipo_bloque* punteroBloque, int iNumeroBloqArch, int iCantCopias, char md5[MD5_LENGTH]);


/*	Carga el Bloque Indicado del Archivo en el FS con la Cantidad de Copias Indicadas
 * 	Se encarga de Enviarlo a los Nodos y Controlar que haya tantas copias del bloque como lo indicado
 *	return: -1 error, 0 exito */
//int FS_CargarBloqueDeArchivo(tipo_id tIdArch, int iNumeroBloque, tipo_bloque* punteroBloque, int iCantCopias) ;

/*	Crea un Nuevo Archivo en la Base de Datos, pasandole los Datos necesarios y Devolviendo por Referencia el ID del archivo creado
 * 	Devuelve Numero Positivo o 0 en Exito. Devuelve un Numero Negativo en Error */
int FS_CrearIdArchivo(const char sNombre[FILE_LONGITUD_NOMBRE], const char*	md5, const tipo_id idDirectorioDondeSubirArchivo, const size_t tamanioArchivo, tipo_id* idArchivoCreado);

/*	Dado el ID de un Archivo, elimina Primero todos los Bloques Asociados al Archivo y luego el mismo archivo
 * 	Devuelve Numero Positivo o 0 en Exito. Devuelve un Numero Negativo en Error */
int FS_EliminarArchivo(tipo_id tIdArchivo);

/*	Dado el Nombre del Nodo Se encarga de Activar (Marcar como Disponible) al mismo Nodo, Todos Sus Bloques y tambien se Fija si algun Archivo se Reactivo.
 * 	Devuelve Numero Positivo o 0 en Exito. Devuelve un Numero Negativo en Error */
int FS_ActivarNodo(char sNombre[NODO_LONGITUD_NOMBRE]);

/*	Abre todos los indices de la base de datos
 * 	Devuelve 0 en caso que se abrieran todos
 * 	Devuelve -1 en caso de error e imprime por pantalla que paso */
int FS_abrir_indices();

//Dado el nombre del archivo , el numero de bloque dentro de ese archivo y el numero de copia; obtiene el "tipo_datos_bloques" del bloque
//Devuelve Numero Positivo o 0 en Exito. Devuelve un Numero Negativo en Error
int FS_ObtenerCopiaBloqueArchivo(tipo_id tIdArchivo, int iBloqueArch, int iCopia, tipo_datos_bloques  *tBloqueArch);

//Dado el nombre del archivo y el numero de Bloque, me devuelve por referencia la Cantidad de Copias que hay de ese Bloque
//Devuelve Numero Positivo o 0 en Exito. Devuelve un Numero Negativo en Error
int FS_CantidadCopiasBloqueArchivo(tipo_id tIdArchivo, int numeroBloqueDentroArchivo,int *iCantCopias);

//Dado un nombre de archivo y un numero de bloque del archivo que elimina sus copias (marque esos bloques como Vacios y actualiza los Nodos)
//Devuelve Numero Positivo o 0 en Exito. Devuelve un Numero Negativo en Error
int FS_EliminarBloqueArchivo(tipo_id tIdArchivo, int iBloqueArch);

//Dado el Numero de Bloque y el ID del Nodo, me Traiga el ID del Bloque
//Devuelve Numero Positivo o 0 en Exito. Devuelve un Numero Negativo en Error
int FS_ObtenerBloqueNodo(tipo_id tIdNodo, int iNumeroBloqueNodo, tipo_id *tIdBloque);

//Dado el Nombre del Nodo lo marca como NO Disponible.
int FS_DesactivarNodo(char sNombre[NODO_LONGITUD_NOMBRE]);

//Permite Reservar un Bloque para que no sea usado por Otro.
//Devuelve Numero Positivo o 0 en Exito. Devuelve un Numero Negativo en Error
int FS_ReservarBloque(tipo_id tIdNodo, int numeroBloqueNodo, char numeroCopia, int numeroBloqueArch, tipo_id tIdArch, tipo_id *tIdBloque);

//Permite Liberar un Bloque que Reservamos Anteriomente.
//Devuelve Numero Positivo o 0 en Exito. Devuelve un Numero Negativo en Error
int FS_LiberarBloque(tipo_id tIdBloque);

//Devuelve por Referencia la Cantidad de Nodos Disponibles/Activos en este momento dentro del File System.
//Devuelve Numero Positivo o 0 en Exito. Devuelve un Numero Negativo en Error
int FS_ContarNodosActivos(int *iCant);

//Valida que los Nodos Guardados en la Base estan Activos o No. Y tambien Des/Activa los Archivos correspondientes
//Devuelve Numero Positivo o 0 en Exito. Devuelve un Numero Negativo en Error
int FS_CargarNodos();

//int FS_AgregarNodoActivo(tipo_id *pIdNodo);
//int FS_NodosActivos(tipo_id *ptIdNodo, t_operacion_nodo estado, int iNumNodo, int *iTotNodos);

//Esa funcion dado un ID de Nodo Busca el Primer Bloque Libre del Nodo
//Devuelve Numero Positivo o 0 en Exito. Devuelve un Numero Negativo en Error
int FS_ObtenerNumeroBloquesNodo(tipo_id tIdNodo, int *iBloqueNodo);

//Dado un Bloque que se Cayo (se pasa el ID del Bloque) se fija si ese bloque caido desactiva el Archivo proque no hay mas copias
//Devuelve Numero Positivo o 0 en Exito. Devuelve un Numero Negativo en Error
int FS_DesActivarArchivo(tipo_id tIdBloque);


int FS_GuardarBloque(tipo_id tIdBloque, size_t lSize, char md5[MD5_LENGTH]);

//Valida que exista el archivo, que exista la ruta y que el archivo este exactamente dentro de esa ruta
//De forma correcta devuelve el ID del Archivo (es un Int), sino devuelve un numero Negativo e imprime por pantalla los errores
//Numeros de Error: -1 si no existe la Ruta, -2 si no hay ningun archivo con ese nombre, -3 si no hay ningun archivo con ese nombre que coincida que esta JUSTO en la ruta indicada, -4 si no se pueden acceder a los datos de algun archivo con ese nombre y -5 si exploto en la Base de Datos al buscar archivo
tipo_id FS_Validar_Archivo_Ruta(char*archivoNombre, char* archivoRuta);

bool FS_VerificarEspacioParaCopias(int iCantBloqueArchivo, int iCantCopias, tipo_id ***Reservado);
int FS_LiberarEspacioReservado(int iCantBloqueArchivo, int iCantCopias, tipo_id **Reservado);

//Dado un Puntero al bloque y el Id del bloque asociado al bloque se encarga de enviarlo al Nodo Correspondiente
//Devuelve 0 en Exito. Devuelve -1 en Error
int FS_EnviarBloque(tipo_bloque* punteroBloque, tipo_id tIdBloque);

bool FS_ReasignarEspacioParaCopias(int iCantBloqueArchivo, int BloqueDesde, int iCantCopias, tipo_id ***NodosReservados);

#endif
//FSLOGIC_H_
