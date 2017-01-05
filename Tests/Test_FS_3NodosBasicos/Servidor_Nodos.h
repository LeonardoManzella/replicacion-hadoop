#ifndef SERVIDOR_NODOS_H_
#define SERVIDOR_NODOS_H_

	#include <stdio.h>
	#include <stdlib.h>
	#include <pthread.h>

	//Librerias para Semaforos
	#include <time.h>

	#include <signal.h> //Para el Manejo de Señales de C

	#include <commons/string.h>
	#include <commons/log.h>

	#include "../../Sockets/Biblioteca_Sockets.h"
	#include "../../Biblioteca_Comun/Biblioteca_Comun.h"
	#include "../../FileSystem/headers/Biblioteca_Bloques.h"




	#define INFINITO 1
	//Nombre del Archivo de Logs
	#define ARCHIVO_DE_LOG "ArchivoLOG.txt"

	#define PUERTO_FILE_SYSTEM "6000"
	#define IP_FILE_SYSTEM "127.0.0.1"

	#define ARCHIVOFINAL_TAMANIO 100
	#define ARCHIVOFINAL_CANTIDAD_BLOQUES 1
	#define ARCHIVOFINAL_MD5 "Un MD5 del Archivo Final-Relleno"
	#define ARCHIVOFINAL_CONTENIDO "Contenido Correcto del Archivo Final\n"

	#define BLOQUE_PEDIDO_CONTENIDO "Contenido Correcto del Bloque Pedido\n"


	//Thread Para los Nodos Dummys
	void threadNodo(char* sPuertoEscucha);

	//Rutina para Eliminar Procesos Zombies (se encarga automaticamente)
	void Handler_Eliminar_Zombies(int valorQueNoUso);

	//Funcion para el Thread que se crea por cada cliente
	void threadCreadoPorCliente(void* socket);

	//Funcion para Loguear(escribir) en el ARCHIVO_DE_LOG del File System
	/*	Nivles de Logueo Disponibles:
									LOG_LEVEL_TRACE
									LOG_LEVEL_DEBUG
									LOG_LEVEL_INFO
									LOG_LEVEL_WARNING
									LOG_LEVEL_ERROR
	 */
	void Servidor_loguear(const char* nombreTareaFuncion, const char* textoPorLoguear, t_log_level nivelDeLogueo );

	void Servidor_realizarOrden_envioUbicacionBloqueParaGuardar(tipo_socket* socketConectadoAlCliente, t_header headerRecibido);

	void Servidor_realizarOrden_pedirSubirArchivoFinal(tipo_socket* socketConectadoAlCliente, t_header headerRecibido);

	void Servidor_realizarOrden_pedirBloque(tipo_socket* socketConectadoAlCliente, t_header headerRecibido);

	void Servidor_realizarOrden_copiarBloqueANodo(tipo_socket* socketConectadoAlCliente, t_header headerRecibido);

#endif /* SERVIDOR_NODOS_H_ */
