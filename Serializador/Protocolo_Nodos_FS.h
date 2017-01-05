#ifndef SERIALIZADOR_PROTOCOLO_NODOS_FS_H_
#define SERIALIZADOR_PROTOCOLO_NODOS_FS_H_

	#include <stdint.h>
	#include "../Biblioteca_Comun/Biblioteca_Comun.h" //Para las Constantes

	//defino paquete de datos de archivo final
	typedef struct __attribute__ ((__packed__)) {
		uint32_t 	tamanio; 			//tamaño del archivo final
		uint32_t 	cantidad_bloques;  	//cantidad de bloques
		char 		md5[MD5_LENGTH]; 	//string con el md5 del archivo final
	} t_datos_archivo_final;


	//defino paquete de datos de nodo
	typedef struct __attribute__ ((__packed__)) {
		char 		nombre_nodo[NODO_LONGITUD_NOMBRE]; 		//nombre del nodo
		char 		ip[LONGITUD_CHAR_IP]; 					//ip del nodo (max 255.255.255.255)
		uint32_t 	puerto; 								//puerto
		uint32_t 	numero_bloques;  						//número de bloques del bin
	} t_datos_nodo;


	//defino paquete de datos para la orden de Copiar Bloques entre nodos
	typedef struct __attribute__ ((__packed__)) {
		uint32_t	nodoOrigenNumeroBloque;				//Numero del Bloque que el Nodo debe copiar a otro Nodo
		char		nodoDestinoIP[LONGITUD_CHAR_IP];
		uint32_t	nodoDestinoPuerto;
		uint32_t	nodoDestinoNumeroBloque;			//Numero de Bloque donde se copiara en el otro Nodo
		size_t		tamanioBloque;
	} t_datos_orden_copiar_bloque;

	typedef struct __attribute__ ((__packed__)) {
		uint32_t	nodoDestinoNumeroBloque;			//Numero de Bloque donde se copiara en el otro Nodo
		size_t		tamanioBloque;
	} t_datos_orden_leer_bloque;

	typedef struct __attribute__ ((__packed__)) {
		uint32_t	nodoDestinoNumeroBloque;			//Numero de Bloque donde se copiara en el otro Nodo
		size_t		tamanioBloque;
	} t_datos_orden_pedir_md5_bloque;


	/*	Serializador de datos archivo final...
	 * 	DEBE HACERSE un "datos_archivo_final_liberar_recursos" del char* cuando deje de usarse  */
	char* serializar_datos_archivo_final(t_datos_archivo_final datos_archivo_final, uint32_t *tamanioSerializacion);

	// Deserializador de datos archivo final...
	t_datos_archivo_final deserializar_datos_archivo_final(const char* datos_archivo_final_serializados);

	// Libera los recursos de datos archivo final serializados
	void datos_archivo_final_liberar_recursos(char* datos_archivo_final_serializados);



	/*	Serializador de datos de nodo...
	 *	DEBE HACERSE un "datos_nodo_liberar_recursos" del char* cuando deje de usarse.
	 * 	NOTA: el tamaño del char* resultante tiene que ser sí o sí el sizeof(t_daos_archivo_final). Si da otro nro entonces está mal el código  */
	char* serializar_datos_nodo(t_datos_nodo datos_nodo);

	// Deserializador de datos de nodo...
	t_datos_nodo deserializar_datos_nodo(const char* datos_nodo_serializados);

	// Libera los recursos de datos nodo serializados
	void datos_nodo_liberar_recursos(char* datos_nodo_serializados);



	//DEBE HACERSE un "orden_copiar_liberar_recursos" del char* cuando deje de usarse.
	char* serializar_orden_copiar(t_datos_orden_copiar_bloque datos, uint32_t* tamanioSerializacion);

	t_datos_orden_copiar_bloque deserializar_orden_copiar(const char* datosSerializados);

	// Libera los recursos
	void orden_copiar_liberar_recursos(char* datosSerializados);


	// Serializador para la orden pedirBloqueMD5 en el nodo
	char* serializarNodoPedirBloqueMD5(char* md5Bloque, uint32_t* tamanioSerializacion);

	// Deserializador de la orden pedirBloqueMD5 en el nodo
	t_datos_orden_pedir_md5_bloque deserializarNodoPedirBloqueMD5(const char* ordenSerializada);

	// Serializador para la orden pedirBloqueMD5 en FS
	char* serializarFSPedirBloqueMD5(const t_datos_orden_pedir_md5_bloque datosBloque, uint32_t* tamanioSerializacion);

	// Deserializador de la orden pedirBloqueMD5 en FS
//	char* deserializarFSPedirBloqueMD5(const char* bloqueMD5Serializado);

	char* serializar_orden_leer_bloque(t_datos_orden_leer_bloque datos_orden_leer_bloque, uint32_t *tamanioSerializacion);


	t_datos_orden_leer_bloque deserializar_orden_leer_bloque(const char* datos_orden_leer_bloque_serializados);

	void datos_orden_leer_bloque_liberar_recursos(char* datos_orden_leer_bloque_serializados);

#endif /* SERIALIZADOR_PROTOCOLO_NODOS_FS_H_ */
