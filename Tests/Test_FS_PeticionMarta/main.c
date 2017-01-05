#include <stdio.h>
#include <commons/string.h>

#include "../../Sockets/Biblioteca_Sockets.h"
#include "../../Serializador/Protocolo_Marta_FS.h"

#define RUTA_ARCHIVO_POR_PEDIR "/Prueba.txt"

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
		return 0;
	}

	//Envio Peticion para Que me de Bloques de un Archivo
	char* peticion = package_create(RUTA_ARCHIVO_POR_PEDIR , string_length(RUTA_ARCHIVO_POR_PEDIR)+1 , "pedirBloquesArchivo" , MARTA);
	iNumeroError = Sockets_enviar_datos(socketConectadoAlMarta , peticion);
	if (iNumeroError <= 0) {
		printf("ERROR: No puedo enviar peiticion al FS\n");
		return 0;
	}

	//Espero La lista
	t_header headerRecibido;
	iNumeroError = Sockets_recibir_Header(socketConectadoAlMarta , &headerRecibido);
	if (iNumeroError <= 0) {
		printf("ERROR: No puedo recibir la Lista, error al recibir Header desde FS\n");
		return 0;
	}

	char* listaSerializada;
	iNumeroError = Sockets_recibir_Datos(socketConectadoAlMarta , &listaSerializada , headerRecibido);
	if (iNumeroError < 0) {
		printf("ERROR: Error al recibir la lista Serializada desde FS\n");
		return 0;
	}

	//Verifico si me dijo que Esta Disponible o NO el Archivo viendo la orden que llego
	if (header_esOrden(headerRecibido , "​cancelacionPedirArchivo")) {
		printf("El FS me dijo que el Archivo No esta Disponible: %s\n" , headerRecibido.order);
		return 0;
	} else if (!header_esOrden(headerRecibido , "respuestaBloquesArchivo")) {
		printf("Llego cualquier cosa como orden desde el FS: %s\n" , headerRecibido.order);
		return 0;
	}

	//Des-Serializo la Lista
	t_list* listaBloques = DesSerializar_listaCopiasBloqueNodo(listaSerializada);
	if (listaBloques == NULL) {
		printf("Llego cualquier cosa como Lista desde el FS, fijate que la 'listaSerializada' empieza con \\0 O algo asi..\n");
		return 0;
	}

	//Como es Correcto voy a Imprimir los Datos con forma de Tabla, Primero Imprimo la "Cabezera" De la Tabla
	printf("%50s %6s %5s %17s %5s \n" , "Nombre Nodo (Por ahora es la IP)" , "Bloque" , "Copia" , "IP" , "Puerto");

	//Recorro la lista Imprimiendo las Triadas
	int elementoActual;
	for (elementoActual = 0; elementoActual < list_size(listaBloques); elementoActual++) {
		//FIXME NO estoy seguro si debe ir "<" o "<=", porque no se como devuelve bien el "list_size". Pareceria ser que va "<" por los tests de las commons (ver https://github.com/sisoputnfrba/so-commons-library/blob/master/tests/unit-tests/test_list.c#L116-L141 )

		//tipo_triadaCopias* triada = TriadaCopias_crear();

		//char* datosLista;
		tipo_triadaCopias* datosLista = list_get((t_list*) listaBloques , elementoActual);
		//int cantidadCopias;
		//memcpy(&cantidadCopias,datosLista,sizeof(int));
		//triada->cantidadCopiasValidas = cantidadCopias;

		//Por cada Triada Recorro las 3 Copias
		int copiaActual;
		for (copiaActual = 0; copiaActual < datosLista->cantidadCopiasValidas; copiaActual++) {
			printf("%50s %6d %5d %17s %5s \n" , datosLista->copia[copiaActual].nodoNombre , datosLista->copia[copiaActual].numeroBloqueDentroNodo , copiaActual+1 , datosLista->copia[copiaActual].nodoIP , datosLista->copia[copiaActual].nodoPuerto);
		}

		printf("\n");     //Para Separar entre Triadas
	}

	printf("\n\n");

	//Si llego hasta aca, es que el Test Es Correcto
	printf("TEST CORRECTO <OK> \n");
	return 1;
}
