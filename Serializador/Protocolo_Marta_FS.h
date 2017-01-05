/*	NOTAS DE USO:
 * 		Las unicas funciones que deberian usarse desde el marta deberian ser "DesSerializar_listaCopiasBloqueNodo", "TriadaCopias_destruir" y TriadaCopias_obtenerCopia ; las demas no deberian usarse en Marta, son Internas o se usan en el File System
 */


#ifndef SERIALIZADOR_PROTOCOLO_MARTA_FS_H_
#define SERIALIZADOR_PROTOCOLO_MARTA_FS_H_

#include <stdint.h>
#include "../Biblioteca_Comun/Biblioteca_Comun.h"

#include "commons/collections/list.h"



//Tipo de Dato que contiene los datos necesarios sobre un bloque dentro de un nodo para un archivo determinado (no hace falta guardarlo, porque tanto el Marta como el FS lo conocen en el momento que se envia estos struct)
typedef struct __attribute__ ((__packed__)) {
	char nodoNombre[NODO_LONGITUD_NOMBRE];
	char nodoIP[LONGITUD_CHAR_IP];
	char nodoPuerto[LONGITUD_CHAR_PUERTOS];
	uint32_t numeroBloqueDentroNodo;
	// --- Julian 2015-06-21
	uint32_t numeroBloqueDeArchivo;
	size_t sizeArchivoDentroDelBloque;
} tipo_copia_bloqueNodo;


//Tipo de dato que contiene las 3 copias, No use un vector comun en vez del struct, porque nada me asegura que se escriban en memoria de forma contigua
typedef struct __attribute__ ((__packed__)) {
	int cantidadCopiasValidas;
	//Vector de 3 copias, para Recorrerlo a Mano va de 0 a 2
	//tipo_copia_bloqueNodo copia[3];
	// --- Julian 2015-06-21
	tipo_copia_bloqueNodo *copia;
} tipo_triadaCopias;


tipo_triadaCopias* TriadaCopias_crear();
//Crea con malloc e inicializa un puntero al "tipo_triadaCopias". Es necesario trabajar con punteros porque asi lo requieren las Listas Dinamicas de las Commons

void TriadaCopias_destruir(tipo_triadaCopias* triadaPorDestruir);
//Libera la memoria del "tipo_triadaCopias", puede usarse dentro del "list_destroy_and_destroy_elements" (y similares) de las Commons

void TriadaCopias_agregarCopia(tipo_triadaCopias* triadaDondeAgregar, const tipo_copia_bloqueNodo copiaPorAgregar, const int numeroDeCopia);
//Copia dentro de "triadaDondeAgregar" los datos que contiene "copiaPorAgregar" dentro del vector de copias en la posicion "numeroDeCopia"(valores validos: 1 a 3)

tipo_copia_bloqueNodo TriadaCopias_obtenerCopia(const tipo_triadaCopias* triada, const int numeroDeCopia);
//Dado la triada y el Numero de Copia te devuelve el "tipo_copia_bloqueNodo". Valores validos: 1 a 3


char* Serializar_listaCopiasBloqueNodo(const t_list* listaPorSerializar, uint32_t* tamanioSerializacion );
//Dada una Lista (de las Commons) que contengan elementos "tipo_triadaCopias" la serializa y devuelve un char*, tambien devuelve por referencia (&tamanioSerializacion) el tamanio de la Serializacion.
//Debido a que el tamaño es variable en tiempo de ejecucion, debi usar Malloc.
//Asi que DEBEN ASEGURARSE de hacer FREE al dejar de usarla y antes de llamar a otro "Serializar_listaCopiasBloqueNodo".

t_list* DesSerializar_listaCopiasBloqueNodo(const char* listaSerializada);
//Dado una lista serializada que contengan elementos "tipo_triadaCopias" vuelve a crear la lista
//RECUERDEN hacer un "list_destroy_and_destroy_elements(lista, (void*)&TriadaCopias_destruir);" al final (cuando la dejen de usar) para liberar la memoria
//Puede aplicarse cualquier funcion de las commons sobre esa lista (Altamente Recomendado Usarlas!)

char* serializar_rutaFinalYNodoContenedor(char* rutaFinal, char* nombreNodoContenedor, uint32_t* tamanioSerializacion);
//dada una ruta de archivo final, resultado de un job, y el nombre del nodo que tiene dicho archivo, serializa estos dos datos para que puedan ser enviados
//NO OLVIDAR HACER FREE AL CHAR* QUE DEVUELVE UNA VEZ USADO, YA QUE SE USO MALLOC Y PUEDE HABERR MEMORY LEAKS

void deserializar_rutaFinalYNodoContenedor(const char* datosSerializados, char** rutaFinal, char** nombreNodoContenedor);
//Dados los datos serializados de este mensaje, los deserializa para su uso posterior. Requiere pasar los 2 string  finales por referencia (&<Char*>) que se van a usar para devolverlos
//No hace falta inicializar antes los strings, los inicializo internamente
//NO OLVIDAR HACER FREE A DICHOS PUNTEROS UNA VEZ TERMINADOS DE USAR! USO MALLOC Y PUEDE HABERR MEMORY LEAKS


#endif /* SERIALIZADOR_PROTOCOLO_MARTA_FS_H_ */
