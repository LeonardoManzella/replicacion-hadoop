#ifndef SERIALIZADOR_PROTOCOLO_MARTA_JOB_NODO_H_
#define SERIALIZADOR_PROTOCOLO_MARTA_JOB_NODO_H_

	#include <stdint.h>
	#include <stdbool.h>

	#include "commons/collections/list.h"

	#include "../Biblioteca_Comun/Biblioteca_Comun.h"


	typedef enum {
		MAPPING = 1,
		REDUCEconCOMBINER = 2,
		REDUCEsinCOMBINER = 3,
	} tipo_enum_orden;



	typedef struct __attribute__ ((__packed__)) {
		char 	jobPuerto[LONGITUD_CHAR_PUERTOS];
		bool 	soportaCombiner;
		char 	rutaYnombreArchivoFinal[RUTA_LONGITUD_APROX];
	} tipo_Datos_Conexion_Inicial;



	#define TERMINACION_CORRECTA 	 0
	#define TERMINACION_FALLIDA 	-1
	#define NODO_DESCONECTADO 		-2

	typedef struct __attribute__ ((__packed__)) {
		char 			nodoIP[LONGITUD_CHAR_IP];
		char 			nodoPuerto[LONGITUD_CHAR_PUERTOS];
		tipo_enum_orden tipoDeOrden;
		int 			terminacionCorrecta;		//Los Defines de Arriba ban
	} tipo_Datos_RespuestaMarta;


	typedef struct __attribute__ ((__packed__)) {
		char 			nodoIP[LONGITUD_CHAR_IP];
		char 			nodoPuerto[LONGITUD_CHAR_PUERTOS];
		char			nombreArchivo[FILE_LONGITUD_NOMBRE];
	} tipo_nodoExterno;




	char* Serializar_DatosConexionInicial(const tipo_Datos_Conexion_Inicial datosPorSerializar, uint32_t* tamanioSerializacion );
	//Como use Malloc, deben hacer un Comun_LiberarMemoria cuando se Deje de usar el Char* Serializado

	tipo_Datos_Conexion_Inicial DesSerializar_DatosConexionInicial(const char* datosSerializados);



	char* Serializar_listaArchivos(const t_list* listaPorSerializar, uint32_t* tamanioSerializacion );
	//Dada una Lista (de las Commons) que contengan elementos "char*s" la serializa y devuelve un char*, tambien devuelve por referencia (&tamanioSerializacion) el tamanio de la Serializacion.
	//Debido a que el tamaño es variable en tiempo de ejecucion, debi usar Malloc.
	//Asi que DEBEN ASEGURARSE de hacer FREE al dejar de usarla

	t_list* DesSerializar_listaArchivos(const char* listaSerializada);
	//Dado una lista serializada que contengan elementos "char*s" vuelve a crear la lista
	//RECUERDEN hacer un "list_destroy_and_destroy_elements(lista, (void*)&destructor_elementoListaArchivo);" al final (cuando la dejen de usar) para liberar la memoria
	//Puede aplicarse cualquier funcion de las commons sobre esa lista (Altamente Recomendado Usarlas!)


	void destructor_elementoListaArchivo(char* stringPorLiberar);
	//Como los elementos de la Lista deben ser Char*s con Malloc, van a necesitar este destructor para liberar la memoria al destruir la lista




	char* Serializar_RespuestaMarta(const tipo_Datos_RespuestaMarta datosPorSerializar, uint32_t* tamanioSerializacion );
	//Como use Malloc, deben hacer un Comun_LiberarMemoria cuando se Deje de usar el Char* Serializado

	tipo_Datos_RespuestaMarta DesSerializar_RespuestaMarta(const char* datosSerializados);




	char* Serializar_DatosNodo(const char nodoIP[LONGITUD_CHAR_IP], const char nodoPuerto[LONGITUD_CHAR_PUERTOS],  uint32_t* tamanioSerializacion );
	//Como use Malloc, deben hacer un Comun_LiberarMemoria cuando se Deje de usar el Char* Serializado

	void DesSerializar_DatosNodo(const char* datosSerializados, char** nodoIP, char** nodoPuerto);
	//Deben pasar los char de Ip y Puerto por referencia
	//Como use Funciones de las Commons, deben hacer un "Comun_LiberarMemoria" cuando dejen de usar ambos chars




	char* Serializar_DatosMapping(const uint32_t numeroBloquePorTrabajar, const uint32_t tamanioBloquePorTrabajar, const char* nombreArchivoResultado, uint32_t* tamanioSerializacion );
	//Como use Malloc, deben hacer un Comun_LiberarMemoria cuando se Deje de usar el Char* Serializado

	void DesSerializar_DatosMapping(const char* datosSerializados, uint32_t* numeroBloquePorTrabajar, uint32_t* tamanioBloquePorTrabajar, char** nombreArchivoResultado);
	//Deben pasar por referencia el "numeroBloquePorTrabajar" y el "nombreArchivoResultado", como use Funciones de las Commons, deben hacer un "Comun_LiberarMemoria" cuando dejen de usar el "nombreArchivoResultado"





	char* Serializar_DatosReduceConCombiner(const t_list* listaArchivos, const char* nombreArchivoResultado, uint32_t* tamanioSerializacion );
	//Como use Malloc, deben hacer un Comun_LiberarMemoria cuando se Deje de usar el Char* Serializado
	//La lista de archivos contiene los archivos locales al nodo que seran usados en este Reduce

	void DesSerializar_DatosReduceConCombiner(const char* datosSerializados, t_list** listaArchivos, char** nombreArchivoResultado);
	//Deben pasar por referencia la Lista y el char*.
	//Como use Funciones de las Commons, deben hacer un "Comun_LiberarMemoria" cuando dejen de usar el char*
	//Y cuando dejen de usar la lista hagan un "list_destroy_and_destroy_elements(lista, (void*)&destructor_elementoListaArchivo);"




	char* Serializar_DatosReduceSinCombiner(const t_list* listaArchivosLocales, const t_list* listaNodosExternos, const char* nombreArchivoResultado, uint32_t* tamanioSerializacion );
	//Como use Malloc, deben hacer un Comun_LiberarMemoria cuando se Deje de usar el Char* Serializado
	//La lista de archivos contiene los archivos locales al nodo que seran usados en este Reduce
	//la lista de nodos contiene elementos IP-Puerto-Archivo, los usara un Nodo para pedirle los archivos a los otros nodos


	void DesSerializar_DatosReduceSinCombiner(const char* datosSerializados, t_list** listaArchivos, t_list** listaNodosExternos, char** nombreArchivoResultado);
	//Deben pasar por referencia las Listas y el char*.
	//Como use Funciones de las Commons, deben hacer un "Comun_LiberarMemoria" cuando dejen de usar el char*
	//Y cuando dejen de usar la lista de archivos hagan un "list_destroy_and_destroy_elements(lista, (void*)&destructor_elementoListaArchivo);"
	//Y cuando dejen de usar la lista de nodos hagan un "list_destroy_and_destroy_elements(lista, (void*)&destructor_elementoListaNodo);"

	tipo_nodoExterno* CrearNodoExterno(char nodoIP[LONGITUD_CHAR_IP], char nodoPuerto[LONGITUD_CHAR_PUERTOS], char nombreArchivo[FILE_LONGITUD_NOMBRE]);
	//Como para Llenar la Lista es Necesario que los elementos sean Dinamicos con Malloc, este Constructor se encarga de eso.

	void destructor_elementoListaNodo(tipo_nodoExterno* nodoPorLiberar);
	//Como los elementos de la Lista deben ser "tipo_nodoExterno" con Malloc, van a necesitar este destructor para liberar la memoria al destruir la lista



#endif /* SERIALIZADOR_PROTOCOLO_MARTA_JOB_NODO_H_ */

