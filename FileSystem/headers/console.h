#ifndef CONSOLE_H_
#define CONSOLE_H_

#include "fslogic.h"





//Muestra el texto de ayuda por consola
void mostarAyuda( char* queMostrar);

//Formatea el File System
void console_formatearFS();


void console_listarNodos();

//Chequea los inputs ingresados y posteriormente procede a crear la carpeta
void console_crearCarpeta(char* directoryName, char* ruta);

//Mueve una carpeta de origen a destino
void console_moverCarpeta(char* directoryName, char* rutaOrigen, char* rutaDestino);

//Renombra la Carpeta "nombreActual" a "nombreNuevo" que se encuentra dentro del File System en "rutaUbicacion"
void console_renombrarCarpeta(char* rutaUbicacion, char*nombreActual, char* nombreNuevo);

//Borra la carpeta indicada si existe en la ruta indicada
void console_borrarCarpeta(char* directoryName, char* ruta);





//Carga el Archivo al File System, Primero intenta tratar de Subirlo a la Base de Datos y luego de Leer los Nodos del Archivo.
void console_cargarArchivo(char* rutaLinux, char* rutaFileSystem);

//Pide el md5 de un archivo a la base de datos
void console_requestMd5(char* fileName, char* ruta);

//Mueve el archivo de la ruta de origen a la ruta de destino
void console_moverArchivo(char* nombreArchivo, char* rutaOrigen, char* rutaDestino);

//Renombra el Archivo "nombreActual" a "nombreNuevo" que se encuentra dentro del File System en
//"rutaUbicacion", si existen y son validos estos 3 argumentos.
void console_renombrarArchivo(char* rutaUbicacion, char*nombreActual, char* nombreNuevo);

//Si hay un error al crear el archivo, corta la ejecucion y avisa que paso.
//Si hay algun error durante la escritura de bloques, corta la ejecucion (el archivo lo cierra y lo deja creado en el linux, para poder ver donde paso el error, que contenido tenia el bloque, etc).
void console_descargarArchivo(char* rutaLinux, char* rutaFileSystem); //Descarga el Archivo desde el File System al Linux. Le pide al File System bloque por bloque y los va escribiendo en un archivo nuevo (si ya existiese lo sobrescribe) ubicado en "rutaLinux".

//Borra el archivo de la ruta especificada
void console_borrarArchivo(char* nombreArchivo, char* rutaOrigen);






//Guarda el contenido del Bloque en un Archivo del cual se indica la Ruta
void console_verBloqueNodo(char* nodoNombre, char* numeroBloque, char* rutaArchivoPorGenerar);

//copia un bloque del nodo origen a el nodo destino
void console_copiarBloqueANodo(char*nodoOrigenNombre, char* nodoOrigenNumeroBloque, char* nodoDestinoNombre, char* nodoDestinoNumeroBloque);

//Borra un Bloque de Un Nodo especificado
void console_borrarBloqueNodo(char* nodoNombre,char* numeroBloque);



//Agrega un nodo a la lista de nodos disponibles
void console_agregarNodo(char* name);

//Elimina un nodo de la lista de nodos disponibles
void console_eliminarNodo(char* name);





//lista todos los directorios y archivos contenidos en la ruta
void console_listarContenido(char* ruta);

//Crea todos las carpetas de una ruta
void console_crearRuta(char* ruta);

//Muestra la lista de los bloques para el archivo pedido
void console_mostrarBloquesDeArchivo(char* archivo, char* ruta);

//Borra un bloque de la lista de bloques disponibles para un archivo
void console_borrarBloqueDeArchivo(char* archivo, char* ruta, char* blockNumber);



//Esto borra las subcarpetas y archivos de la carpeta, pero sin visibilidad del usuario, el usuario solo ve el resultado final
//Siempre Primero Empieza Eliminando Carpetas y luego archivos, en cuanto detecta una para eliminar no la elimina hasta que halla eliminado sus carpetas y archivos hijos. osea que Nunca va a quedar una carpeta o archivo hijo sin eliminar, se asegura de borrar todos o dar error.
int console_recursive_delete(int idCarpetaPadrePorBorrar);

//Valida que exista el archivo y que el archivo contenga ese bloque
//De forma correcta devuelve 1, sino devuelve un numero Negativo e imprime por pantalla los errores
int console_Validar_Archivo_Bloque(char* archivoNombre, uint32_t archivoNumeroBloque);

//Dado el archivoNombre y archivoNumeroBloque, busca Un Nodo cualquiera que tenga una copia valida del bloque y se lo pide. Ante Errores devuelve NULL, sino devuelve el Bloque.
tipo_bloque* console_PedirFSBloqueArchivo(char* archivoNombre, uint32_t archivoNumeroBloque);

//Dado el ID del Nodo y el Numero de Bloque dentro de Ese Nodo, le pedi el bloque al nodo y te lo devuelve. En caso de Error Devuelve NULL
tipo_bloque* console_PedirNodoBloque(tipo_id nodoID, uint32_t nodoNumeroBloque, size_t sizeArchivoDentroDelBloque);

#endif
//CONSOLE_H_
