#ifndef FILESYSTEM_SERVIDOR_FILESYSTEM_H_
#define FILESYSTEM_SERVIDOR_FILESYSTEM_H_

	#include <commons/log.h>

	#include "../../Sockets/Biblioteca_Sockets.h"

	#define RUTA_ARCHIVO_CONFIG_FS "FSconfig.cfg"

	#define INFINITO 1
	//Nombre del Archivo de Logs de File System
	#define ARCHIVO_DE_LOG "LogFileSystem.txt"





	//Funcion para el Thread Base del Servidor (que crea otros Threads adentro)
	void threadBaseServidor(char* sPuertoEscucha);


	//Rutina para Eliminar Procesos Zombies (se encarga automaticamente)
	void Handler_Eliminar_Zombies(int valorQueNoUso);


	//Funcion para el Thread que se crea por cada cliente
	void threadCreadoPorCliente(void* socket);


	/*	Funcion para Loguear(escribir) en el ARCHIVO_DE_LOG del File System
			Nivles de Logueo Disponibles:
										LOG_LEVEL_TRACE
										LOG_LEVEL_DEBUG
										LOG_LEVEL_INFO
										LOG_LEVEL_WARNING
										LOG_LEVEL_ERROR
		NOTA: Permite pasar argumentos %s al estilo Printf  */
	void Servidor_loguear(t_log_level nivelDeLogueo,  const char* nombreTareaFuncion,  const char* textoPorLoguear,  ... );


	void Servidor_realizarOrden_FSDisponible(tipo_socket* socketConectadoAlMarta, t_header headerRecibido);

	//Funcion para realizar la orden de "pedirBloquesArchivo"
	//Dentro de la funcion se encarga de manejar errores y loguearlos
	void Servidor_realizarOrden_pedirBloquesArchivo(tipo_socket* socketConectadoAlMarta, t_header headerRecibido);


	//Funcion para realizar la orden de "pedirSubirArchivoFinal"
	//Dentro de la funcion se encarga de manejar errores y loguearlos
	void Servidor_realizarOrden_pedirSubirArchivoFinal(tipo_socket* socketConectadoAlMarta, t_header headerRecibido);


	//Funcion para realizar la orden de "avisarNodoConectado" (cuando un Nodo se me conecta)
	//Dentro de la funcion se encarga de manejar errores y loguearlos
	//NOTA: Tuve que hacer que devuelva el Nombre del nodo, proque es Necesario para Main_File_System. Si hay un Error devuelve NULL
	char* Servidor_realizarOrden_avisarNodoConectado(tipo_socket* socketConectadoAlNodo, t_header headerRecibido);

#endif
// FILESYSTEM_SERVIDOR_FILESYSTEM_H_
