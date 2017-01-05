#include <stdio.h>

#include <commons/string.h>
#include <stdlib.h>

#include "../../Sockets/Biblioteca_Sockets.h"
#include "../../Serializador/Protocolo_Marta_FS.h"

#define RUTA_ARCHIVO_POR_SUBIR "/ArchivoFinal.txt"
#define NOMBRE_NODO_CONTIENE_ARCHIVO "7001"

#define PUERTO_FILE_SYSTEM "6000"
#define IP_FILE_SYSTEM "127.0.0.1"

int main() {
	//Variable para Manejo de Errores
	int iNumeroError = 0;

	printf("Iniciando Test...\n");

	//Me conecto al FS
	tipo_socket* socketConectadoAlMarta = Sockets_conectar_servidor(IP_FILE_SYSTEM , PUERTO_FILE_SYSTEM);
	if (socketConectadoAlMarta == NULL) {
		printf("ERROR: No puedo conectar al FS\n");
		printf("TEST Fallido <MAL>\n");
		return -1;
	}

	//Serializo la Peticion
	uint32_t tamanioSerializacion = -1;
	char* payloadSerializado = serializar_rutaFinalYNodoContenedor(RUTA_ARCHIVO_POR_SUBIR, NOMBRE_NODO_CONTIENE_ARCHIVO, &tamanioSerializacion);

	//Envio Peticion para Que me de Bloques de un Archivo
	char* peticion = package_create(payloadSerializado , string_length("OJO CON LO QUE PONGO ACA, QUE EL DES-SERIALIZADOR DEVUELVA VARIABLE TAMANIO SERIALIZACION")+1 , "pedirSubirArchivoFinal" , MARTA);
	iNumeroError = Sockets_enviar_datos(socketConectadoAlMarta , peticion);
	if (iNumeroError <= 0) {
		printf("ERROR: No puedo enviar peticion al FS\n");
		printf("TEST Fallido <MAL>\n");
		return -1;
	}

	//Espero La repuesta
	t_header headerRecibido;
	iNumeroError = Sockets_recibir_Header(socketConectadoAlMarta , &headerRecibido);
	if (iNumeroError <= 0) {
		Macro_ImprimirParaDebugConDatos("ERROR: No se pudo recibir el resultado de subir el archivo, error al recibir Header desde FS\n");
		printf("TEST Fallido <MAL>\n");
		return iNumeroError;
	}

	//Verifico si me dijo que se pudo el archivo
	if (header_esOrden(headerRecibido , "​cancelacionSubirArchivo")) {
		Macro_ImprimirParaDebugConDatos("El FS me dijo que el Archivo No se pudo subir: %s.\n Puede no ser algo Incorrecto si Ya esta Subido el Archivo" , headerRecibido.order);
		iNumeroError = -1;
	} else if (!header_esOrden(headerRecibido , "respuestaArchivoSubido")) {
		Macro_ImprimirParaDebugConDatos("Llego cualquier cosa como orden desde el FS: %s\n" , headerRecibido.order);
		iNumeroError = -1;
	}

	free(payloadSerializado);
	free(peticion);


	Sockets_cerrar_desconectar(socketConectadoAlMarta);

	if( iNumeroError == -1 ){
		printf("TEST Fallido <MAL>\n");
		return -1;
	}


	//Si llego hasta aca, es que el Test Es Correcto
	printf("TEST CORRECTO <OK> Recomiendo Correr el 'Test_FS_PeticionMarta' (cambiando el nombre del archivo que pide) para verificar que se subio el archivo en los Bloques. \n");
	return 1;
}
