#ifndef MARTA_SERVER_H_
#define MARTA_SERVER_H_

#include <semaphore.h>

#include "marta_configuration.h"
#include "../../Sockets/Biblioteca_Sockets.h"

typedef struct {
	int orden;
	tipo_nodo_marta * nodo;
	char* archivoResultado;
	sem_t* semaphore;
} tipo_orden_job_marta;

bool FileSystemActivo(const char IPFS[LONGITUD_CHAR_IP], const char PuertoFS[LONGITUD_CHAR_PUERTOS]);

//Recordar que debe enviarse la RutaYNombre del archivo al FS, no solo el nombre
//Ante Errores Devuelve NULL, Si el Archivo no esta Disponible Tambien devuelve NULL
t_list* FileSystemCopiasBloqueNodo(const char IPFS[LONGITUD_CHAR_IP], const char PuertoFS[LONGITUD_CHAR_PUERTOS],
		const char* ArchivoConRuta);

//Recordar que debe enviarse la RutaYNombre del archivo al FS, no solo el nombre
//En Caso de Ejecucion Correcta Devuelve Un Numero Positivo, Mayor a 0
//Ante Errores Devuelve -1, Si el Archivo no se Subio Tambien devuelve -1
int FileSystemSubirArchivoNodo(const char IPFS[LONGITUD_CHAR_IP], const char PuertoFS[LONGITUD_CHAR_PUERTOS],
		const char* ArchivoConRuta, const char NodoNombre[NODO_LONGITUD_NOMBRE]);

//Funcion para Crear un Thread en Marta.c (Marta Padre) para escuchar Jobs y generar nuevos Marta Hijos
//NOTA: debe pasarsele un Int* como puerto
void Marta_Servidor_Iniciar(void* puertoEscuchaMartaVoid);

//Funcion que se Encarga de Crear nuevos Marta Hijos para Atender a un JOB Particular que esta conectado por el Socket. (Luego de Recibir los Datos del JOB cierra ese Socket)
void Servidor_realizarOrden_AtenderJOB(tipo_socket* socketConectadoAlJOB, t_header headerRecibido);

//Se encarga de Conectarse al JOB par aenviarle la orden "EndJOB" con el mensaje de Terminacion como parametro. Recordar que en caso de terminacion correcta, se debe enviar “Terminacion Correcta” al JOB.
//Devuelve 0 En caso de Terminacion Correcta y -1 si hubo un Error. Igual no creo que sea importante Manejar el Error de no Envio, se encarga sola la misma funcion..
int Servidor_enviarOrden_EndJOB(const char job_IP[LONGITUD_CHAR_IP], const char job_Puerto[LONGITUD_CHAR_PUERTOS],
		const char* mensajeEnviar);

//Se encarga de conectarse al job para enviarle una orden de mapping
//Devuelve 1 si paso bien, -2 si hubo error del nodo, y -1 si el error fue del job
int Servidor_enviarOrden_MAP_JOB(const char job_IP[LONGITUD_CHAR_IP], const char job_Puerto[LONGITUD_CHAR_PUERTOS],
		tipo_nodo_marta* nodo, const char* archivoResultado);

//Se encarga de conectarse al job para enviarle una orden de reduce local, sea 1 archivo o varios
//Devuelve 1 si paso bien, -2 si hubo error del nodo, y -1 si el error fue del job
int Servidor_enviarOrden_REDUCE_LOCAL_JOB(const char job_IP[LONGITUD_CHAR_IP],
		const char job_Puerto[LONGITUD_CHAR_PUERTOS], tipo_nodo_marta* nodo, t_list* listaDeArchivos,
		const char* archivoResultado);

//Se encarga de conectarse al job para enviarle la orden de reduce final. Requiere que la lista de nodos sea de tipo Nodo_externo
//Devuelve 1 si paso bien, -2 si hubo error del nodo, y -1 si el error fue del job
int Servidor_enviarOrden_REDUCE_FINAL_JOB(const char job_IP[LONGITUD_CHAR_IP],
		const char job_Puerto[LONGITUD_CHAR_PUERTOS], tipo_nodo_marta* nodo, t_list* listaDeArchivos,
		t_list* listaNodosExternos, const char* archivoResultado);

#endif /* MARTA_SERVER_H_ */
