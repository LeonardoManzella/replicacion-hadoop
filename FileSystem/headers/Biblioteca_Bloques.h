#ifndef BIBLIOTECA_BLOQUES_H_
#define BIBLIOTECA_BLOQUES_H_

	#include <stdint.h> //Para el tipo uint32_t
	#include<sys/types.h>

	//Tamaño del Bloque, 20mb = 20*1024*1024 = 20971520 bytes. Y como 1 byte = 1char, son 20971520 chars.
	#define TAMANIO_BLOQUE 20971520
//para debug
//#define TAMANIO_BLOQUE 4096

	//PARA DEBUG
	//#define TAMANIO_BLOQUE 500


typedef struct __attribute__ ((__packed__)) {
	//NOTA: Le pongo los Modificadores Completos para que sea mas portable y evite errores.
	signed long int 	tamanioBloque;							//Si hay problemas (Poco Probable), cambiarlo a uint32_t
	char 				contenidoBloque[TAMANIO_BLOQUE+1];		//El "+1" es por el caracter de Fin de String, sino da Segmentations Fault al usar el String.
} tipo_bloque;





	tipo_bloque* Bloques_crear_nuevo();
		//Crea (dinamicamente) un bloque nuevo, inicializando el  "contenidoBloque" con Ceros y el "tamanioBloque" a 0.

		

	void Bloques_destruir(tipo_bloque* tBloque);
		//Destruye/Libera la memoria que ocupa un Bloque.


		
	tipo_bloque* Bloques_obtener_desde_archivo_texto( const char *sRutaArchivo, uint32_t *piPosicionLecturaArchivo );
		/*
		 * Abre el Archivo para Lectura y devuelva un puntero al Bloque Leido ( de 20971520 bytes, 20mb), listo para cargar a un Nodo, debe recibirse en un "tipo_bloque" y al final realizarsele "Bloques_destruir". Tambien modifica la Posicion de Lectura del Archivo, estableciendo la posicion de lectura del proximo bloque. Al final Cierra el archivo.
		 * La funcion YA Se encarga de imprimir con "perror" para cualquier tipo de error que haya.
		 * En caso de Error, corta su ejecucion. Porque considero que no se pueda leer los bloques de un archivo es un error muy grave y debe parar la ejecucion.
		 * 		Si hay error devuelve NULL (en el Puntero).
		NOTA: Los Bloques siempre son de maximo 20mb, si hay menos datos se rellena con Ceros hasta los 20mb (para que no haya basura dentro del bloque).
		*/


		
	uint32_t Bloques_obtener_tamanio_archivo( const char* sRutaArchivo );
		//Dada la ruta (dentro de Linux) de un archivo, usa la funcion "stat" para obtener el tamaño en Bytes del archivo.
		//En caso de no poder obtenerse el tamaño, devuelve -1


	uint32_t Bloques_obtener_cantidad_bloques_archivo(  const char* sRutaArchivo  );
		//Dado la Ruta del Archivo se Encarga de Calcular la Cantidad de Bloques.
		//NOTA: La Cantidad de Bloques NO siempre es "Tamaño Archivo/20mb", por eso la existencia de esta Funcion.
		//En caso de no poder obtenerse el tamaño, devuelve -1


	char* Bloques_serializar(const tipo_bloque* punteroBloque, uint32_t* tamanioSerializacion);
		//Dado un puntero a bloque, serializa el tipo_bloque y lo devuelve serializado
		//NOTA: Debido a que estamos trabajando con Chars de 20mb, debi usar Malloc. ASEGURENSE de hacer un Free al "char*" cada vez despues de usar el "package_create"..

	tipo_bloque* Bloques_des_serializar(const char* bloqueSerializado);
	///Dado un bloque serializado crea un nuevo tipo_bloque y devuelve un puntero a el.
	//NOTA: Recordar usar "Bloques_destruir" cuando no lo uses mas.
	//NOTA: Asegurense de que antes de volver a llamar a "Bloques_des_serializar" hagan un "Bloques_destruir", para evitar memory leaks
	size_t Bloques_obtener_tamanio_bloque_en_memoria( char **punteroBloqueMemoria);
#endif 
//BIBLIOTECA_BLOQUES_H_

