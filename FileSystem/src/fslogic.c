#include "../headers/fslogic.h"

//#include <sys/types.h>
//#include <errno.h>
//#include <stdio.h>
//#include <db.h>
#include <string.h>
//#include <unistd.h>
#include <stdlib.h>
#include <libgen.h> //Para  "basename" y "dirname"

#include <commons/string.h>
#include "../../Biblioteca_Comun/Biblioteca_Comun.h"
#include "../../Sockets/Biblioteca_Sockets.h"
#include <fcntl.h>


int FS_DesMarcarBloquesResevadosNodo(tipo_id IdNodo);
int FS_MarcarBloquesResevadosNodo(tipo_id IdNodo);
int FS_crearArrayNodosDisp(int **Espacio, tipo_id **IdNodos, int *TamanioArray);
int FS_ObtenerNodosDondeCopiar(int *Espacio, tipo_id *IdNodos, int TamanoArray, int iCantCopias, tipo_id **NodosDes);
int FS_EnviarBloque_Progreso(tipo_bloque* punteroBloque, tipo_id tIdBloque, char *mensajecopia);



int FS_ObtenerNumeroBloquesNodo(tipo_id tIdNodo, int *iBloqueNodo) {
	int iReturn = 0;
		// Recorro todos los datos de los bloques del Nodo
		//int iBloqAct;
		tipo_datos_nodo tNodo;
		//tipo_id tIdBloque;
		iReturn = NODO_Consultar(tIdNodo,&tNodo);
		Macro_CheckError(iReturn,"Error al consultar Nodo");

		int iBloqueActual, iTotalBloques;
		iTotalBloques = tNodo.bloques_totales;
		for (iBloqueActual = 0; iBloqueActual < iTotalBloques; iBloqueActual++){
			if (bitarray_test_bit(tNodo.tBloquesNodo,iBloqueActual) == false) {
				break;
			}
		}

		/*
		// No necesariamente devuelve los bloques en orden
		iReturn = BLOQUES_buscarBloqueNodo(tIdNodo,&tIdBloque,DB_SET, NULL);
		CheckError(iReturn, "Error al buscar Bloque en el Nodo");
		for (iBloqAct = 1; iBloqAct <= tNodo.bloques_totales; iBloqAct++) {
			// Consulto el bloque, indexado por tIdNodo, en DB_NOTFOUND paro
			if (iReturn == DB_NOTFOUND) {
				break;
			}
			iReturn = BLOQUES_buscarBloqueNodo(tIdNodo,&tIdBloque,DB_NEXT_DUP, NULL);
			CheckError(iReturn,"Error al buscar Bloque en el Nodo");
		}
		*iBloqueNodo = iBloqAct;
		 */
		*iBloqueNodo = iBloqueActual;
		return iReturn;
}

int FS_ObtenerBloqueNodo(tipo_id tIdNodo, int iNumeroBloqueNodo, tipo_id *tIdBloque) {
	int iReturn = 0;
	tipo_datos_nodo tNodo;
	tipo_datos_bloques datosBloqueEncontrado;

	iReturn = NODO_Consultar(tIdNodo,&tNodo);
	Macro_CheckError(iReturn,"Error al consultar Nodo");

	//Busco el Primer Bloque
	iReturn = BLOQUES_buscarBloqueNodo(tIdNodo,tIdBloque,DB_SET, NULL);
	Macro_CheckError(iReturn,"Error al buscar Bloque en el Nodo");

	//Busco los Demas y Veo si es el que quiero
	int iBloqAct;
	for (iBloqAct = 1; iBloqAct <= tNodo.bloques_totales; iBloqAct++) {

		//Obtengo los Datos del Bloque
		iReturn = BLOQUES_Consultar(*tIdBloque, &datosBloqueEncontrado );
		if (iReturn != DB_NOTFOUND) {
			Macro_CheckError(iReturn,"Error al Consultar los datos del Bloque");
			//Si es el Numero de Bloque que Buscaba, paro y devuelvo por referencia su ID
			if(datosBloqueEncontrado.numeroBloqueDentroNodo==iNumeroBloqueNodo){
				//No hace falta Modificarlo a "tIdBloque" porque se modifica Siempre que busco, entonces se devuelve Bien Por Referencia..
				//Cambiamos para que no Genere Error
				//iReturn = 0;
				break;
			}
		}
		//Siguiente Bloque
		iReturn = BLOQUES_buscarBloqueNodo(tIdNodo,tIdBloque,DB_NEXT_DUP, NULL);
		if ((iReturn == DB_NOTFOUND)) {
			break;
		} else {
			Macro_CheckError(iReturn,"Error al buscar Bloque en el Nodo");
		}
	}
	return iReturn;
}

int FS_ReservarBloque(tipo_id tIdNodo, int numeroBloqueNodo, char numeroCopia, int numeroBloqueArch, tipo_id tIdArch, tipo_id *tIdBloque) {
	int iReturn = 0;
	tipo_datos_bloques tBloque;
	// Variables para sacar basura pero que no se fijan
	tBloque.disponible = 0;
	tBloque.tamano = 0;

	// Variables que se asignan
	tBloque.tIdNodo = tIdNodo;
	tBloque.numeroBloqueDentroArchivo = numeroBloqueArch;
	tBloque.numeroBloqueDentroNodo = numeroBloqueNodo;
	tBloque.copia = numeroCopia;
	tBloque.tIdArchivo = tIdArch;
	iReturn = BLOQUES_Nuevo(tBloque,tIdBloque);
	Macro_CheckError(iReturn,"Error al crear un nuevo bloque");

	return iReturn;
}

int FS_LiberarBloque(tipo_id bloqueID) {
	int iReturn = 0;

	//Antes de Eliminar el Bloque, debo Actualizar en el Nodo que hay un Bloque Libre Mas
	tipo_datos_bloques bloqueDatos;
	iReturn = BLOQUES_Consultar(bloqueID, &bloqueDatos);
	Macro_CheckError(iReturn,"Error al obtener datos del bloque");

	tipo_datos_nodo nodoDatos;
	iReturn = NODO_Consultar(bloqueDatos.tIdNodo ,&nodoDatos);
	Macro_CheckError(iReturn,"Error al obtener datos del Nodo al cual Pertenece el Bloque");

	//nodoDatos.bloques_libres++;
	//nodoDatos.bloques_usados--;

	iReturn = NODO_Modificar(bloqueDatos.tIdNodo, nodoDatos);
	Macro_CheckError(iReturn,"Error al Actualizar los datos del Nodo al cual Pertenece el Bloque");

	//Ahora si Elimino al Bloque
	iReturn = BLOQUES_Eliminar(bloqueID);
	Macro_CheckError(iReturn,"Error al eliminar el bloque");

	return iReturn;
}

int FS_CrearIdArchivo(const char sNombre[FILE_LONGITUD_NOMBRE], const char md5[MD5_LENGTH], const tipo_id idDirectorioDondeSubirArchivo, const size_t tamanioArchivo, tipo_id* idArchivoCreado){
	int iReturn = 0;

	tipo_datos_file tArch;
	memset(tArch.sNombre,0,FILE_LONGITUD_NOMBRE);
	memcpy(tArch.sNombre,sNombre,strlen(sNombre)+1);
	memset(tArch.md5,0,MD5_LENGTH);
	memcpy(tArch.md5,md5,strlen(md5)+1);
	tArch.dirPadre = idDirectorioDondeSubirArchivo;
	tArch.disponible = 0;
	tArch.tamanioArchivo = tamanioArchivo;
	tArch.iCantBloques = 0;
	tArch.tBloquesActivos = NULL;
	iReturn = FILE_Crear(tArch, idArchivoCreado);
	Macro_CheckError(iReturn,"Error al crear el archivo");

	return iReturn;
}

int FS_GuardarBloque(tipo_id tIdBloque, size_t lSize, char md5[MD5_LENGTH]) {
	int iReturn = 0;

	tipo_datos_bloques tBloques;
	tipo_datos_nodo tNodo;
	iReturn = BLOQUES_Consultar(tIdBloque, &tBloques);
	Macro_CheckError(iReturn, "Error al consultar datos del Bloque");

	tBloques.tamano = lSize;
	tBloques.disponible = true;
	memcpy(tBloques.md5, md5, MD5_LENGTH);
	iReturn = BLOQUES_Modificar(tIdBloque, tBloques);
	Macro_CheckError(iReturn,"Error al modificar datos del Bloque");


	iReturn = NODO_Consultar(tBloques.tIdNodo,&tNodo);
	Macro_CheckError(iReturn,"Error al consultar Nodo");
	if (tNodo.activo == true) {
		if (tNodo.tBloquesNodo != NULL)
			bitarray_set_bit(tNodo.tBloquesNodo,tBloques.numeroBloqueDentroNodo);
		tNodo.bloques_libres--;
		tNodo.bloques_usados++;
	}
	iReturn = NODO_Modificar(tBloques.tIdNodo,tNodo);
	Macro_CheckError(iReturn,"Error al actualziar datos del Nodo");

	return iReturn;
}

/*
 * Eliminar bloques y archivo por Id Archivo
*/
int FS_EliminarArchivo(tipo_id tIdArchivo) {
	int iReturn = 0;
	tipo_id bloqueID;
	tipo_datos_bloques datosDelBloque;
	tipo_datos_nodo datosDelNodo;
	tipo_datos_file datosArchivo;
	int iCantBloques, iBloqActual;

	iReturn = FILE_ConsultarId(tIdArchivo, &datosArchivo);
	Macro_CheckError(iReturn,"Error al buscar Datos del Archivo");
	iReturn = BLOQUES_buscarBloqueArchivo(tIdArchivo,&bloqueID,DB_SET, &iCantBloques);
	if (iReturn == DB_NOTFOUND) {
		// No se encontraron bloque del archivo
		// Esto puede pasar cuando el archivo se carga mal
		iCantBloques = 0;
	} else {
		Macro_CheckError(iReturn,"Error al buscar Bloques del Archivo");
	}

	for (iBloqActual = 1; iBloqActual <= iCantBloques; iBloqActual++) {
		iReturn = BLOQUES_Consultar(bloqueID,&datosDelBloque);
		Macro_CheckError(iReturn,"Error al consultar los datos del Bloque");

		iReturn = NODO_Consultar(datosDelBloque.tIdNodo, &datosDelNodo);
		Macro_CheckError(iReturn, "Error al consultar los datos del Nodo");

		// Como la rutina puede correr y el nodo no estar cargado, aca puede explotar
		// FIXME: El nood puede no estar activo y provoca un Segment Fault porque no existe la direccion
		// El problema principal es al cargar los nodos de 0 despues de un error del FS y sale mal
		if ((datosDelNodo.activo == true) && (datosDelNodo.tBloquesNodo != NULL)) {
			bitarray_clean_bit(datosDelNodo.tBloquesNodo, datosDelBloque.numeroBloqueDentroNodo);
		}

		//Antes de Eliminar el Bloque, debo Actualizar en el Nodo que hay un Bloque Libre Mas
		datosDelNodo.bloques_libres++;
		datosDelNodo.bloques_usados--;

		iReturn = NODO_Modificar(datosDelBloque.tIdNodo, datosDelNodo);
		Macro_CheckError(iReturn,"Error al Actualizar los datos del Nodo al cual Pertenece el Bloque");

		//Ahora si Elimino al Bloque
		iReturn = BLOQUES_Eliminar(bloqueID);
		Macro_CheckError(iReturn,"Error al eliminar Bloques");

		iReturn = BLOQUES_buscarBloqueArchivo(tIdArchivo,&bloqueID,DB_NEXT_DUP, NULL);
		if (iReturn == DB_NOTFOUND) {
			break;
		} else {
			Macro_CheckError(iReturn,"Error al buscar Bloques del Archivo");
		}
	}

	if (datosArchivo.tBloquesActivos != NULL) {
		free(datosArchivo.tBloquesActivos->bitarray);
		bitarray_destroy(datosArchivo.tBloquesActivos);
		datosArchivo.tBloquesActivos=NULL;
	}
	iReturn = FILE_EliminarId(tIdArchivo);
	Macro_CheckError(iReturn,"Error al eliminar el Archivo");

	return iReturn;	
}

int FS_EnviarBloque(tipo_bloque* punteroBloque, tipo_id tIdBloque) {
	int 				iReturn = 0;		//ariable par amanejar errores por defecto es 0 que significa sin errores
	tipo_socket* 		socketConectadoNodoEnviarBloque = NULL;
	bool				llegueConectarNodo = false;
	tipo_datos_nodo 	tNodo;
	tipo_datos_bloques 	tBloque;
	char* 				MensajeBloque = NULL;
	char* 				MensajeUbicBloque = NULL;
	char* 				MensajeRecibido = NULL;
	t_header 			headerRecibido;

	Macro_ImprimirParaDebug("Voy a Enviar una Copia de un Bloque a un Nodo\n");


	iReturn = BLOQUES_Consultar(tIdBloque, &tBloque);
	Macro_Check_And_Handle_Error(iReturn < 0,"Error al consultar datos del Bloque");

	iReturn = NODO_Consultar(tBloque.tIdNodo, &tNodo);
	Macro_Check_And_Handle_Error(iReturn < 0,"Error al consultar datos del Nodo");

	

	// Conecto al Nodo
	llegueConectarNodo = true;
	socketConectadoNodoEnviarBloque = Sockets_conectar_servidor(tNodo.nodoIP, tNodo.nodoPuerto);
	Macro_Check_And_Handle_Error(socketConectadoNodoEnviarBloque == NULL, "No pude conectarme al nodo");
	
	Macro_ImprimirParaDebug("PudeConectarme al Nodo\n");

	// Creo los paquetes para los mensajes
	char* numeroBloqueNodo = string_itoa(tBloque.numeroBloqueDentroNodo);
	MensajeUbicBloque = package_create(numeroBloqueNodo,string_length(numeroBloqueNodo)+1, "envioUbicacionBloqueParaGuardar", FILESYSTEM);
	
	//Envio Datos
	iReturn = Sockets_enviar_datos(socketConectadoNodoEnviarBloque, MensajeUbicBloque);
	Macro_Check_And_Handle_Error(iReturn <= 0, "No pude enviar la Orden de Guardar el Bloque");

	Macro_ImprimirParaDebug("Pude Enviar el la Orden de Guardar Bloque\n");

	Comun_LiberarMemoria((void**)&MensajeUbicBloque);
	
	//Serializo el Bloque
	uint32_t tamanioSerializacion = 0;
	char* bloqueSerializado = Bloques_serializar(punteroBloque,&tamanioSerializacion);

	MensajeBloque = package_create(bloqueSerializado ,tamanioSerializacion , "envioBloqueSerializadoParaGuardar", FILESYSTEM);

	//Libero la Memoria de la Serializacion del Bloque
	Comun_LiberarMemoria((void**)&bloqueSerializado);

	//Envio Datos
	iReturn = Sockets_enviar_datos(socketConectadoNodoEnviarBloque, MensajeBloque);
	Macro_Check_And_Handle_Error(iReturn <= 0,"No pude Enviar el Bloque Serializado al Nodo");

	Macro_ImprimirParaDebug("Pude Enviar el Bloque Serializado\n");


	//Libero la memoria del Bloque Enviado
	Comun_LiberarMemoria((void**)&MensajeBloque);

	//Ahora voy a Recibir una Confirmacion de que se Recibio y Guardo el Bloque

	Macro_ImprimirParaDebug("Ahora voy a Recibir una Confirmacion de que se Recibio y Guardo el Bloque\n");

	//Recibo el Header
	iReturn = Sockets_recibir_Header(socketConectadoNodoEnviarBloque, &headerRecibido);
	Macro_Check_And_Handle_Error(iReturn <= 0, "No pude Recibir Header Confirmacion");


	//Recibo Mensaje/Payload
	iReturn = Sockets_recibir_Datos(socketConectadoNodoEnviarBloque, &MensajeRecibido, headerRecibido);
	Macro_Check_And_Handle_Error(iReturn <= 0, "No pude Recibir Datos Confirmacion");

	Macro_ImprimirParaDebug("Pude Recibir la Confirmacion, Procedo a Compararla\n");

	//Verificamos que el Bloque se recibio Bien
	if(!string_equals_ignore_case(MensajeRecibido,"Bloque Guardado Correctamente")){
		Macro_ImprimirParaDebug("El Bloque No se Guardo Correctamente\n");

		//Como no hay error sockets hay que cerrarlo a mano
		Sockets_cerrar_desconectar(socketConectadoNodoEnviarBloque);

		Macro_Check_And_Handle_Error(true, "El Bloque No se Guardo Correctamente");
		//No hace Falta Return, va Siempre al Error handler
	}

	Macro_ImprimirParaDebug("Confirmacion Correcta. Se envio y guardo el Bloque en el Nodo Indicado\n");

	//Como lo dejo de usar, libero la memoria del payload
	Comun_LiberarMemoria((void**)&MensajeRecibido);

	Sockets_cerrar_desconectar(socketConectadoNodoEnviarBloque);

	return 0;



Error_Handler:
	Macro_Imprimir_Error("No se pudo Enviar un Bloque al Nodo:'%s'", tNodo.sNombre);

	//Si llegue a Almenos intentar conectar al Nodo, eso significa que exploto por el Nodo y debo Desactivarlo
	if( llegueConectarNodo == true ){
		Macro_ImprimirParaDebug("Se cayo el nodo %s. Procedo a Desactivarlo\n", tNodo.sNombre);

		iReturn = FS_DesactivarNodo(tNodo.sNombre);
		if (iReturn < 0) {
			Macro_ImprimirParaDebug("Error al Desactivar al Nodo: %s\n", tNodo.sNombre);
			//Esta bien que no haya return aca
		}
	}

	//Libero Memoria usada
	Comun_LiberarMemoria((void**)&MensajeUbicBloque);
	Comun_LiberarMemoria((void**)&MensajeRecibido);
	// FIXME: Leo, esta explotando aca despues de matar un nodo!
	Comun_LiberarMemoria((void**)&bloqueSerializado);
	Comun_LiberarMemoria((void**)&MensajeBloque);

	return -1;
}

int FS_EnviarBloque_Progreso(tipo_bloque* punteroBloque, tipo_id tIdBloque, char *mensajecopia) {



	int 				iReturn = 0;		//ariable par amanejar errores por defecto es 0 que significa sin errores
	uint32_t bytesPorEnviar = 0, bytesEnviados = 0;
	ssize_t bytesRetorno = 0;
	tipo_socket* 		socketConectadoNodoEnviarBloque = NULL;
	bool				llegueConectarNodo = false;
	tipo_datos_nodo 	tNodo;
	tipo_datos_bloques 	tBloque;
	char* 				MensajeBloque = NULL;
	char* 				MensajeUbicBloque = NULL;
	char* 				MensajeRecibido = NULL;
	t_header 			headerRecibido;
	char* bloqueSerializado = NULL;

	Macro_ImprimirParaDebug("Voy a Enviar una Copia de un Bloque a un Nodo\n");


	iReturn = BLOQUES_Consultar(tIdBloque, &tBloque);
	Macro_Check_And_Handle_Error(iReturn < 0,"Error al consultar datos del Bloque");

	iReturn = NODO_Consultar(tBloque.tIdNodo, &tNodo);
	Macro_Check_And_Handle_Error(iReturn < 0,"Error al consultar datos del Nodo");



	// Conecto al Nodo
	llegueConectarNodo = true;
	socketConectadoNodoEnviarBloque = Sockets_conectar_servidor(tNodo.nodoIP, tNodo.nodoPuerto);
	Macro_Check_And_Handle_Error(socketConectadoNodoEnviarBloque == NULL, "No pude conectarme al nodo");

	Macro_ImprimirParaDebug("PudeConectarme al Nodo\n");

	// Creo los paquetes para los mensajes
	// FIXME: memory leak
	char* numeroBloqueNodo = string_itoa(tBloque.numeroBloqueDentroNodo);
	MensajeUbicBloque = package_create(numeroBloqueNodo,string_length(numeroBloqueNodo)+1, "envioUbicacionBloqueParaGuardar", FILESYSTEM);

	//Envio Datos
	iReturn = Sockets_enviar_datos(socketConectadoNodoEnviarBloque, MensajeUbicBloque);
	Macro_Check_And_Handle_Error(iReturn <= 0, "No pude enviar la Orden de Guardar el Bloque");

	Macro_ImprimirParaDebug("Pude Enviar el la Orden de Guardar Bloque\n");

	Comun_LiberarMemoria((void**)&MensajeUbicBloque);

	//Serializo el Bloque
	uint32_t tamanioSerializacion = 0;
	bloqueSerializado = Bloques_serializar(punteroBloque,&tamanioSerializacion);

	MensajeBloque = package_create(bloqueSerializado ,tamanioSerializacion , "envioBloqueSerializadoParaGuardar", FILESYSTEM);

	//Libero la Memoria de la Serializacion del Bloque
	Comun_LiberarMemoria((void**)&bloqueSerializado);

	//Obtengo del header de "datosSerializadosConHeader" los bytes a enviar
	memcpy(&bytesPorEnviar, MensajeBloque, sizeof(uint32_t));
	//Controlo que no enviaron cosas raras
	if (bytesPorEnviar <= 0) {
		Macro_ImprimirParaDebug("Error al Enviar, fijate que en el header de los datos serializdos (2do argumento) me pasaste un tamaño 0 o negativo.\n Tamanio detectado: %d", (int)bytesPorEnviar );
		return -1;
	}

	// El Socket asyncronico NO FUNCIONA
//	int optSocket = fcntl(socketConectadoNodoEnviarBloque->descriptorSocket,F_GETFL);
//	fcntl(socketConectadoNodoEnviarBloque->descriptorSocket,F_SETFL, O_NONBLOCK);

	Comun_ImprimirProgreso(mensajecopia,bytesPorEnviar, 0);

	int retry = 0;
	uint32_t bytesPorEnviarParcial = bytesPorEnviar / 100;
	int BloqueActual = 1;
	bytesEnviados = 0;

	while (bytesEnviados < bytesPorEnviar) {
		if (bytesPorEnviar > 65535) {
			if (BloqueActual >99)
				bytesPorEnviarParcial = (bytesPorEnviar-bytesEnviados) + HEADER_SIZE;
			bytesRetorno = Sockets_funcion_enviar_datos_directos(socketConectadoNodoEnviarBloque, (MensajeBloque+bytesEnviados), bytesPorEnviarParcial);
		} else {
			bytesPorEnviar += HEADER_SIZE;
			bytesRetorno = Sockets_funcion_enviar_datos_directos(socketConectadoNodoEnviarBloque, (MensajeBloque+bytesEnviados), bytesPorEnviar);
		}
		if ((bytesRetorno <= 0) && (retry == 9) ) {
			//fcntl(socketConectadoNodoEnviarBloque->descriptorSocket,F_SETFL, optSocket);
			Macro_Check_And_Handle_Error(bytesRetorno <= 0,"No pude Enviar el Bloque Serializado al Nodo");
		} else if (bytesRetorno <= 0) {
			retry++;
		} else {
			bytesEnviados += bytesRetorno;
			Comun_ImprimirProgreso(mensajecopia, bytesPorEnviar, bytesRetorno);
			retry = 0;
			BloqueActual++;
		}
		//printf("Enviados %d | Porcentaje: %02d\r",(bytesEnviados-HEADER_SIZE),(((bytesEnviados-HEADER_SIZE)*100)/bytesPorEnviar));
	}
	printf("\n");
		//fcntl(socketConectadoNodoEnviarBloque->descriptorSocket,F_SETFL, optSocket);

	//Comun_ImprimirProgreso(mensajecopia, bytesPorEnviar, bytesEnviados);
	//printf("Enviados %d\n",bytesEnviados);

	Macro_ImprimirParaDebug("Pude Enviar el Bloque Serializado\n");


	//Libero la memoria del Bloque Enviado
	Comun_LiberarMemoria((void**)&MensajeBloque);

	//Ahora voy a Recibir una Confirmacion de que se Recibio y Guardo el Bloque

	Macro_ImprimirParaDebug("Ahora voy a Recibir una Confirmacion de que se Recibio y Guardo el Bloque\n");

	//Recibo el Header
	iReturn = Sockets_recibir_Header(socketConectadoNodoEnviarBloque, &headerRecibido);
	Macro_Check_And_Handle_Error(iReturn <= 0, "No pude Recibir Header Confirmacion");


	//Recibo Mensaje/Payload
	iReturn = Sockets_recibir_Datos(socketConectadoNodoEnviarBloque, &MensajeRecibido, headerRecibido);
	Macro_Check_And_Handle_Error(iReturn <= 0, "No pude Recibir Datos Confirmacion");

	Macro_ImprimirParaDebug("Pude Recibir la Confirmacion, Procedo a Compararla\n");

	//Verificamos que el Bloque se recibio Bien
	if(!string_equals_ignore_case(MensajeRecibido,"Bloque Guardado Correctamente")){
		Macro_ImprimirParaDebug("El Bloque No se Guardo Correctamente\n");

		//Como no hay error sockets hay que cerrarlo a mano
		Sockets_cerrar_desconectar(socketConectadoNodoEnviarBloque);

		Macro_Check_And_Handle_Error(true, "El Bloque No se Guardo Correctamente");
		//No hace Falta Return, va Siempre al Error handler
	}

	Macro_ImprimirParaDebug("Confirmacion Correcta. Se envio y guardo el Bloque en el Nodo Indicado\n");

	//Como lo dejo de usar, libero la memoria del payload
	Comun_LiberarMemoria((void**)&MensajeRecibido);

	Sockets_cerrar_desconectar(socketConectadoNodoEnviarBloque);

	return 0;



Error_Handler:
	Macro_Imprimir_Error("No se pudo Enviar un Bloque al Nodo:'%s'", tNodo.sNombre);

	//Si llegue a Almenos intentar conectar al Nodo, eso significa que exploto por el Nodo y debo Desactivarlo
	if( llegueConectarNodo == true ){
		Macro_ImprimirParaDebug("Se cayo el nodo %s. Procedo a Desactivarlo\n", tNodo.sNombre);

		iReturn = FS_DesactivarNodo(tNodo.sNombre);
		if (iReturn < 0) {
			Macro_ImprimirParaDebug("Error al Desactivar al Nodo: %s\n", tNodo.sNombre);
			//Esta bien que no haya return aca
		}
	}

	//Libero Memoria usada
	Comun_LiberarMemoria((void**)&MensajeUbicBloque);
	Comun_LiberarMemoria((void**)&MensajeRecibido);
	Comun_LiberarMemoria((void**)&bloqueSerializado);
	Comun_LiberarMemoria((void**)&MensajeBloque);

	return -1;


}


/*
 * CargarArchivo
 * 	tIdArch: recibe el id del archivo ya creado
*/

/*
// Considerar que se puede caer un nodo en el proceso de copia
int FS_CargarBloqueDeArchivo(tipo_id tIdArch, int iNumeroBloqArch, tipo_bloque* punteroBloque, int iCantCopias) {
	int iReturn = 0;

	//Debe Ser Static para que se Inicialize una Unica vez y Sea el Mismo para todos los Threads
	static pthread_mutex_t mutex_interno_eleccion_nodo = PTHREAD_MUTEX_INITIALIZER;

	// Numero actual del Bloque en el Nodo
	int iNumeroBloqueNodo;
	bool mutexAdquirido = false;

	// Defino los tipo_id
	tipo_id *iNodo = NULL; // Para guardar los nodos ya usados
	tipo_id tIdNodo; // Para guardar el nodo actual
	tipo_id tIdBloque; // Para guardar el bloque actual


	iNodo = malloc(sizeof(tipo_id));
	memset(iNodo,0,sizeof(tipo_id));

	// Obtener tamaño de los datos
	long lSize = punteroBloque->tamanioBloque;

	// Establezco por cada bloque de archivo los nodos en 0
	iNodo[0] = 0;
		
	// Establezco por cada bloque que arranque con la copia 1
	int iCopiaActual = 1;
	// Llevo la cuenta de las copias fallidas a los nodos
	int iCopiasFallidads = 0;
	
	// Establezco por cada bloque que el nodo a buscar comience
	// por el primer nodo con mayor espacio libre
	int iNodoAnterior, iNumeroNodo;
	bool NodoEncontrado = false;
	do {
		Macro_ImprimirParaDebug("Voy a Generar la Copia %d en los Nodos \n", iCopiaActual);


		// Buco el nodo con mayor espacio libre, y segun orden
		iNodoAnterior = 1;
		NodoEncontrado = false;
		do {
			//Esto me busca el Nodo con mas espacio libre, pero no me garantiza que este disponible

			iReturn = NODO_BuscarEspacio(&tIdNodo,iNodoAnterior);
			Macro_Check_And_Handle_Error(iReturn < 0,"Error al Buscar Espacio en el Nodo");

			//Verifica que el Nodo no se haya usado
			for (iNumeroNodo=1;iNumeroNodo<=iCopiaActual;iNumeroNodo++) {
				if (iNodo[iNumeroNodo-1] != tIdNodo) {
					iNodo[iCopiaActual-1] = tIdNodo;
					NodoEncontrado = true;
					break;
				} else {
					//Si el Nodo se uso va a buscar el proximo disponible
					iNodoAnterior++;
					iReturn = NODO_BuscarEspacio(&tIdNodo,iNodoAnterior);
					Macro_Check_And_Handle_Error(iReturn < 0,"Error al Buscar Espacio en el Nodo");
					if (iReturn == DB_NOTFOUND) {
						iReturn = -1;
						break;
					}
				}
			}
		} while (NodoEncontrado != true);
		// Verifico que el nodo no se repita con los anteriores
		if (NodoEncontrado == true) {

			Macro_ImprimirParaDebug("Encontre al Nodo de ID %d \n", tIdNodo);

			// Obtengo el primer bloque disponible para el nodo encontrado
			pthread_mutex_lock(&mutex_interno_eleccion_nodo);
			mutexAdquirido = true;


			iReturn = FS_ObtenerNumeroBloquesNodo(tIdNodo,&iNumeroBloqueNodo);
			Macro_Check_And_Handle_Error(iReturn < 0,"Error al Obtener el Bloque del Nodo");

			Macro_ImprimirParaDebug("Encontre que el Bloque %d del Nodo estaba Libre\n", iNumeroBloqueNodo);

			// Reservo el bloque para que nadie mas lo ocupe
			iReturn = FS_ReservarBloque(tIdNodo,iNumeroBloqueNodo,(iCopiaActual-iCopiasFallidads),iNumeroBloqArch,&tIdBloque);
			Macro_Check_And_Handle_Error(iReturn < 0,"Error al Reservar el Bloque de Datos del Nodo");


			pthread_mutex_unlock(&mutex_interno_eleccion_nodo);
			mutexAdquirido = false;
			// Este mutex lo que hace es que dos thread no traten de escribir el mismo bloque al mismo tiempo.
			// Despues de esto pueden seguir trabajando tranquilos y el Id del Bloque va a ser Unico para cada uno
			
			Macro_ImprimirParaDebug("Pude Reservar Ese Bloque\n");

			iReturn = FS_EnviarBloque(punteroBloque,tIdBloque);
			if (iReturn < 0) { 
				Macro_ImprimirParaDebug("Error al enviar el Bloque al Nodo");
				// En caso de error libero el bloque reservado
				iReturn = FS_LiberarBloque(tIdBloque);
				Macro_Check_And_Handle_Error(iReturn < 0,"Error al Liberar el Bloque");

				// Agrego una copia mas para buscar otro Nodo
				iCopiasFallidads++;
				iCantCopias++;
			} else {
				// Aca deberia ver si imprimo un espacion para x cantidad de bloques
				// Como ir moviendome entre las 3 copias simulaneas en las barras
				// Puedo recibir un parametro que sea imprimirprogreso cuando corresponda

				Macro_ImprimirParaDebug("Se pudo Enviar Correctamente el Bloque\n");

				iReturn = FS_GuardarBloque(tIdArch, tIdBloque, lSize, "");
				Macro_Check_And_Handle_Error(iReturn < 0,"Error al Guardar los Datos del Bloque");

				Macro_ImprimirParaDebug("Pude Guardar Correctamente el Bloque\n");
			}
				
			//Guardo El Nodo Que acabo de Usar para guardar la Copia
			//iNodo[iCopiaActual-1] = tIdNodo;
			iCopiaActual++;		//Se agrego una Nueva Copia

			//Agrando el iNodo para poder guardar Una Copia Mas (Guardo el Nodo donde guarde la Copia)
			iNodo = realloc(iNodo,sizeof(tipo_id)*iCopiaActual);
			//Inicializo a 0 esa proxima Copia
			iNodo[iCopiaActual-1] = 0;
		} 
	} while (iCopiaActual <= iCantCopias);


	Macro_ImprimirParaDebug("Pude Generar las Copias del Bloque. Este Bloque ya esta\n");
	Comun_LiberarMemoria((void**) &iNodo);

	//Si llegamos aca es que se termino correctamente, devuelvo 0
	return 0;

Error_Handler:
	//No debe Retornar de la Funcion sin Liberar El Mutex
	if( mutexAdquirido ==  true ){
			pthread_mutex_unlock(&mutex_interno_eleccion_nodo);
			mutexAdquirido = false;
	}

	Comun_LiberarMemoria((void**) &iNodo);

	//Si llegamos aca es que se termino Mal, devuelvo -1
	return -1;
}

*/



int FS_CargarBloqueDeArchivo2(tipo_id tIdArch, tipo_id *IdNodos, tipo_bloque* punteroBloque, int iNumeroBloqArch, int iCantCopias, char md5[MD5_LENGTH]) {
	int iReturn = 0;
	bool bTodoCopiado = false;
	//bool BloquesCopias[iCantCopias];
	// Numero actual del Bloque en el Nodo
	int iNumeroBloqueNodo[iCantCopias];
	// Defino los tipo_id
	tipo_id tIdBloque[iCantCopias]; // Para guardar el bloque actual
	int CopiaActual;
	// Obtener tamaño de los datos
	long lSize = punteroBloque->tamanioBloque;

	for (CopiaActual = 0; CopiaActual < iCantCopias; CopiaActual++) {
		iReturn = FS_ObtenerNumeroBloquesNodo(IdNodos[CopiaActual],&iNumeroBloqueNodo[CopiaActual]);
		Macro_CheckError(iReturn,"Error al Obtener el Bloque del Nodo\n");
		iReturn = FS_ReservarBloque(IdNodos[CopiaActual],iNumeroBloqueNodo[CopiaActual],(CopiaActual+1),iNumeroBloqArch, tIdArch, &tIdBloque[CopiaActual]);
		Macro_CheckError(iReturn,"Error al Reservar el Bloque de Datos del Nodo\n");
		iReturn = FS_DesMarcarBloquesResevadosNodo(IdNodos[CopiaActual]);
		Macro_CheckError(iReturn,"Error al desmarcar los bloques reservados para el Nodo\n");
	}

	char mensajecopia[40];
	for (CopiaActual = 0; CopiaActual < iCantCopias; CopiaActual++) {

		sprintf(mensajecopia, "Copia %d",CopiaActual);
		iReturn = FS_EnviarBloque_Progreso(punteroBloque,tIdBloque[CopiaActual], mensajecopia);
		// Funcion para enviar el bloque entero
//		iReturn = FS_EnviarBloque(punteroBloque,tIdBloque[CopiaActual]);
	
		if (iReturn < 0) {
			Macro_ImprimirParaDebug("Error al enviar el Bloque al Nodo\n");
			bTodoCopiado = false;
			//iCantCopias = CopiaActual;
			break;
		} else {
			bTodoCopiado = true;
			Macro_ImprimirParaDebug("Se pudo Enviar Correctamente el Bloque\n");
		}
	
	}

	for (CopiaActual = 0; CopiaActual < iCantCopias; CopiaActual++) {
		if (bTodoCopiado == false) {
			// En caso de error libero el bloque reservado
			iReturn = FS_LiberarBloque(tIdBloque[CopiaActual]);
			Macro_CheckError(iReturn,"Error al Liberar el Bloque\n");
		} else {
			iReturn = FS_GuardarBloque(tIdBloque[CopiaActual], lSize, md5);
			Macro_CheckError(iReturn,"Error al Guardar los Datos del Bloque\n");
			// TODO: En caso de error aca, tengo que eliminar los bloques guardados y los reservados para poder replanificar
			Macro_ImprimirParaDebug("Pude Guardar Correctamente el Bloque\n");
		}
	}

	if (bTodoCopiado == false) {
		return -1;
	}

	return iReturn;
}


int FS_CargarCompletoArchivo( const char* sRutaArchivo,  const char* md5, const tipo_id tIdDir, bool showProgress ){
	//Creo una variable para la posicion de lectura, que inicializo en 0 porque empiezo a leer el archivo desde el principio.
	uint32_t iPunteroPosicionPorLeer = 0;
	uint32_t iPunteroPosicionPorLeerAnterior = 0;
	//Variable para el ID de Archivo
	tipo_id tIdArch;

	//Variable para chequear errores
	int iNumeroError = 0;

	int iNumeroBloquesArchivo = Bloques_obtener_cantidad_bloques_archivo( sRutaArchivo );
	if (iNumeroBloquesArchivo == -1) {
		printf(ANSI_COLOR_RED "No se pudo Obtener Cuantos Bloques Tenia el Archivo" ANSI_COLOR_RESET "\nPodria Ser un problema de Archivo Invalido. Por Ejemplo si no Contiene ningun '\\n' en un Bloque o no lo tiene al Final del Archivo.\n");
		return -1;
	}

	long lTamanioArchivo = Bloques_obtener_tamanio_archivo( sRutaArchivo );
	//Veo que el archivo no este Vacio. Sino paro la ejecucion de la funcion.
	if( lTamanioArchivo==0 ){
		printf("Error: El archivo Tiene Tamaño 0, revisar que no este vacio.\n");
		return -1;
	}
	// Funcion para tener solo el nombre del archivo sin ruta
		// incluir <libgen.h>
	char* sNombreArchLocal = (char*) basename((char*) sRutaArchivo);

	// Creo el Archivo para obtener el Id con disponible -1 para que sea temporal
	iNumeroError = FS_CrearIdArchivo(sNombreArchLocal, md5,  tIdDir, lTamanioArchivo, &tIdArch);
	//Chequeo Errores
	if (iNumeroError < 0) {
		Macro_ImprimirParaDebug("No se pudo crear el Archivo en la Base de Datos\n");
		return -1;
	}

	//printf("\nSubiendo Bloques \n ");
	printf("\n");

//	int iCopiaActual;
//	for (iCopiaActual=1;iCopiaActual<=CANTIDAD_COPIAS_MINIMAS;iCopiaActual++) {
		//printf("Copia %2d: [\n",iCopiaActual);
//	}

	tipo_id **NodosReservados;
	int iBloqueActualArchivo;
	bool errorAlCopiar = false;
	FS_VerificarEspacioParaCopias(iNumeroBloquesArchivo,CANTIDAD_COPIAS_MINIMAS,&NodosReservados);
	for (iBloqueActualArchivo = 1; iBloqueActualArchivo <= iNumeroBloquesArchivo; iBloqueActualArchivo++) {
		tipo_bloque* punteroBloque = NULL;
		if (errorAlCopiar == true) {
			errorAlCopiar = false;
			//iBloqueActualArchivo--;
			iPunteroPosicionPorLeer = iPunteroPosicionPorLeerAnterior;
		} else {
			iPunteroPosicionPorLeerAnterior = iPunteroPosicionPorLeer;
		}
		punteroBloque = Bloques_obtener_desde_archivo_texto( sRutaArchivo, &iPunteroPosicionPorLeer );
		if ( punteroBloque==NULL ){
			return -2;
		}
		//Imprime un Punto "■" (Cuadradito) por Bloque, es como un "Cargando".
		//printf("■ ");
		char mensajecopia[80];
		sprintf(mensajecopia,"Copiando bloque %d de %d (%d copias de cada uno)",iBloqueActualArchivo, iNumeroBloquesArchivo, CANTIDAD_COPIAS_MINIMAS);
		Comun_ImprimirMensajeConBarras(mensajecopia);
		printf("\n");
		char* md5bloque = Comun_obtener_MD5_Bloque(punteroBloque,true);
		printf("MD5 del bloque: %s\n", md5bloque);
		iNumeroError = FS_CargarBloqueDeArchivo2(tIdArch, NodosReservados[iBloqueActualArchivo-1], punteroBloque, iBloqueActualArchivo, CANTIDAD_COPIAS_MINIMAS, md5bloque);
		if (iNumeroError < 0) {
			bool bResultado;
			bResultado = FS_ReasignarEspacioParaCopias(iNumeroBloquesArchivo, iBloqueActualArchivo, CANTIDAD_COPIAS_MINIMAS, &NodosReservados);
			printf("Intento reasignar los bloques\n");
			if (bResultado == false) {
				printf("Hubo un Error al copiar los bloques a los Nodos\n Borro el Archivo\n");
				/*for (iBloqueActualArchivo = 1; iBloqueActualArchivo <= iNumeroBloquesArchivo; iBloqueActualArchivo++) {
					if (NodosReservados[iBloqueActualArchivo-1] != NULL)
						free(NodosReservados[iBloqueActualArchivo-1]);
				}
				if (NodosReservados != NULL)
					free(NodosReservados);*/
				iNumeroError = FS_EliminarArchivo(tIdArch);
					//Chequeo que se elimine correctamente el Archivo y sus Bloques asociados
				if( iNumeroError < 0 ){
					Macro_ImprimirParaDebug("Hubo un Error al Borrar el archivo y todos sus bloques asociados\n");
				}
				// Ocurrio un error irrecuperable, no hay espacio para seguir copiando el archivo
				return -3;
				//break;
			} else {
				iNumeroError = 0;
				errorAlCopiar = true;
				iBloqueActualArchivo--;
			}
		}
		//Libero la Memoria usada para el bloque (20mb)
		printf("\n");
		Bloques_destruir(punteroBloque);
	}

	for (iBloqueActualArchivo = 1; iBloqueActualArchivo <= iNumeroBloquesArchivo; iBloqueActualArchivo++) {
		free(NodosReservados[iBloqueActualArchivo-1]);
	}
	free(NodosReservados);
	//Llegado a este punto se copiaron en los nodos todos los bloques, entonces el archivo esta listo para ser usado

	// Creo un bit array con la cantidad de bloques del archivo y los pongo todos en 1
	t_bitarray *tBloquesActivos;
	char *cBloquesActivos;
	int iCantBytesBitArray = (iNumeroBloquesArchivo/8)+1;
	cBloquesActivos = malloc(sizeof(char)*iCantBytesBitArray);
	memset(cBloquesActivos,1,sizeof(char)*iCantBytesBitArray);
	// FIXME: memory leak
	tBloquesActivos = bitarray_create(cBloquesActivos,sizeof(char)*iCantBytesBitArray);

	iNumeroError = FILE_PonerDisponible(tIdArch, tBloquesActivos, iNumeroBloquesArchivo);
	if (iNumeroError < 0) {
		// En caso de error debo eliminar el archivo y los bloques ya guardados
		Macro_ImprimirParaDebug("Error al poner disponible el archivo");
		bitarray_destroy(tBloquesActivos);
		// TODO: Aca no estoy informando nada
		//Queres Informar por Pantalla al usuario? Por mi no hay problema
		iNumeroError = FS_EliminarArchivo(tIdArch);
		Macro_CheckError(iNumeroError ,"Error al Borrar el Archivo y todos sus Bloques" );
		return iNumeroError;
	}

	//Aca deberia asegurarme de estar en la ultima barra con una linea mas
	//Asegurado!
	printf("  Todos los Bloques Subidos\n");

	return iNumeroError;
}


int FS_ActivarArchivo(tipo_id tIdArchivo) {
	// Busco el archivo si disponible = 0
	// Verifico si estan todos los bloques del archivo
	// y lo pongo en 1, si ya esta en 1, no me interesa
	int iReturn = 0;
	tipo_datos_file tArchivo;
	tipo_id tIdBloque;
	tipo_datos_bloques tBloqueArch;
	int iBloquesDisponiblesTotales, iBloqueActual;


	iReturn = FILE_ConsultarId(tIdArchivo, &tArchivo);
	Macro_CheckError(iReturn,"Error al consultar Datos del Archivo");
	// Verifico si el archivo no es basura de una version mal cargada

// --- Modificado Julian 2015-07-21
// Este procedimiento puede borrar archivos en curso, solo se va a ejecutar al cargar el FS
// por el caso de que se matara o muera el FS mientras carga un archivo
//	if (tArchivo.iCantBloques == 0) {
//		FS_EliminarArchivo(tIdArchivo);
//		return iReturn;
//	}

	if (tArchivo.disponible == 0) {


		// Iniciamos bitarray
		t_bitarray *tBloquesArchivo;
		char *cbitarrayBloquesArch;
		int iSizeBitArray;

		//Dado la cantidad de bloques del archivo obtengo cuantas bytes necesita el bitarray para reprentar esa cantidad de bloques.
		//Ese +1 es para que no nos quedemos corto por el dividir entero, ej 7/8 nos daria 0.
		iSizeBitArray = (tArchivo.iCantBloques/8)+1;

		// FIXME: memory leak
		//Armo un char* de todos 0 para pasarselo al bittarray_create
		cbitarrayBloquesArch = malloc(sizeof(char)*iSizeBitArray);
		memset(cbitarrayBloquesArch,0,sizeof(char)*iSizeBitArray);
		//Creo el BitArray
		tBloquesArchivo = bitarray_create(cbitarrayBloquesArch,sizeof(char)*iSizeBitArray);

		//Busco el Total de Bloques del Archivo, para recorrerlos
		iReturn = BLOQUES_buscarBloqueArchivo(tIdArchivo, &tIdBloque, DB_SET, &iBloquesDisponiblesTotales);
		Macro_CheckError(iReturn,"Error al Buscar el Bloque del Archivo");


		for (iBloqueActual=0; iBloqueActual < iBloquesDisponiblesTotales; iBloqueActual++) {
			//Obtengo los Datos del Bloque
			iReturn = BLOQUES_Consultar(tIdBloque,&tBloqueArch);
			Macro_CheckError(iReturn,"Error al al consultar los datos del Bloque");

			//Si el Bloque esta Disponible, actualizo el BitArray en la posicion del Bloque
			if (tBloqueArch.disponible == true ) {
				// Le resto 1 porque los Bloques de Archivo Arrancan en 1 y el BitArray arranca en 0
				if (bitarray_test_bit(tBloquesArchivo,tBloqueArch.numeroBloqueDentroArchivo-1) == false) {
					bitarray_set_bit(tBloquesArchivo,tBloqueArch.numeroBloqueDentroArchivo-1);
				}
			}
			//Obtengo el Siguiente Bloque del archivo
			iReturn = BLOQUES_buscarBloqueArchivo(tIdArchivo, &tIdBloque, DB_NEXT_DUP, NULL);
			if (iReturn != DB_NOTFOUND) {
				Macro_CheckError(iReturn,"Error al buscar el Bloque de Archivo");
			} else {
				iReturn = 0;
			}
		}

		int iArchDisp = 1;	//Variable Bandera para saber si el archivo tiene TODOS sus bloques disponibles

		//Ahora voy a Revisar si me quedo algun Bloque No Disponible
		for (iBloqueActual = 0; iBloqueActual < tArchivo.iCantBloques; iBloqueActual++) {
			if (bitarray_test_bit(tBloquesArchivo,iBloqueActual) == false) {
				iArchDisp = 0;
			}
		}

		//Si tiene todos los bloques disponibles es que el archivo esta disponible
		if (iArchDisp == 1) {
			iReturn = FILE_PonerDisponible(tIdArchivo, tBloquesArchivo, tArchivo.iCantBloques);
			Macro_CheckError(iReturn, "No se pudo marcar el archivo como disponible");
		} else {
			// Detruyo y libero memoria, N
			free(cbitarrayBloquesArch);
			bitarray_destroy(tBloquesArchivo);
			//Aca Habia Doble FREE
			//free(tBloquesArchivo);
		}
	}

	return iReturn;
}

int FS_DesActivarArchivo(tipo_id tIdBloque) {
	int iReturn = 0;
	tipo_datos_bloques tBloque, tOtroBloqueArchivo;
	tipo_datos_file tArchivo;
	tipo_id tIdBloqueArchivo;
	bool BloqueArchivoActivo = false;	//Asumo que se va a desactivar el Archivo

	iReturn = BLOQUES_Consultar(tIdBloque,&tBloque);
	Macro_CheckError(iReturn, "Error al consultar los datos del Bloque");

	// Consulto el archivo y veo si esta disponible
	iReturn = FILE_ConsultarId(tBloque.tIdArchivo, &tArchivo);
	Macro_CheckError(iReturn,"Error al consultar los datos del archivo");

	//Si el archivo ya estaba NO Disponible, entonces no hago nada y devuelvo un 1 por terminacion correcta
	if (tArchivo.disponible == false) {
		return 1;
	}

	// Va a buscar si el bloque esta disponible en otro Nodo
	iReturn = BLOQUES_buscarBloqueEspecificoArchivo(tBloque.tIdArchivo, tBloque.numeroBloqueDentroArchivo, &tIdBloqueArchivo, true);
	Macro_CheckError(iReturn,"No se pudo buscar el bloque del archivo");

	//Busco si alguna de las Copias del Bloque esta Activa, para no desactivar el Bloque
	do {
		iReturn = BLOQUES_Consultar(tIdBloqueArchivo,&tOtroBloqueArchivo);
		Macro_CheckError(iReturn,"No se pudo consultar los datos del Bloque");
		if (tOtroBloqueArchivo.disponible == true) {
			BloqueArchivoActivo = true;
			break;
		}
		iReturn = BLOQUES_buscarBloqueEspecificoArchivo(tBloque.tIdArchivo, tBloque.numeroBloqueDentroArchivo, &tIdBloqueArchivo, false);
		if (iReturn == DB_NOTFOUND) {
			break;
		} else {
			Macro_CheckError(iReturn,"No se pudo buscar el bloque del archivo");
		}
	} while (iReturn != DB_NOTFOUND);

	// No encontro otra Copia del Bloque, entonces Ese bloque no esta disponible y debo desactivar el archivo
	if (BloqueArchivoActivo == false) {
		// No es necesario modificar la estructura de tArchivo porque se modifican los datos de la memoria
		// Necesito retar 1 el numero de bloque de archivo, ya que arranca en 1 y el bitarray en 0
		bitarray_clean_bit(tArchivo.tBloquesActivos,tBloque.numeroBloqueDentroArchivo-1);

		// Si el archivo estaba disponible, al faltar el bloque se pone como no disponible
		if (tArchivo.disponible == true) {
			iReturn = FILE_PonerNoDisponible(tBloque.tIdArchivo);
			Macro_CheckError(iReturn, "No se pudo marcar el archivo como NO disponible");
		}
	}

	//Si llegamos hasta Aca es que se Ejecuto Correctamente
	return 1;
}

/*
 * Conexion Nodo
*/
int FS_ActivarNodo(char sNombre[NODO_LONGITUD_NOMBRE]) {
	tipo_id tIdNodo;
	tipo_id tIdBloque;
	tipo_datos_nodo tNodo;
	tipo_datos_bloques tBloques;
	int iReturn = 0;
	int iTotBloques, iBloqAct;

	iReturn = NODO_BuscarNombre(sNombre, &tIdNodo, DB_SET, NULL);
	Macro_CheckError(iReturn,"Error al buscar el Nombre del Nodo");

	// Doy de alta el nodo en el sistema
	// --- Comentado por Julian 06062015
	//iReturn = FS_AgregarNodoActivo(&tIdNodo);
	//CheckError(iReturn,"Error al guardar Datos del Nodo en Memoria");
	// ---

	// Consulto los datos del Nodo
	iReturn = NODO_Consultar(tIdNodo, &tNodo);
	Macro_CheckError(iReturn,"Error al consultar datos del Nodo");
	
	//Si no existe su BitArray lo Recreo
	if (tNodo.tBloquesNodo==NULL) {
		// Inicio el BitArray con todos 0 para contar bloques del Nodo
		char *cBloquesNodo;
		int iSizeNodo;
		iSizeNodo = (tNodo.bloques_totales / 8) + 1;
		cBloquesNodo = malloc(sizeof(char) * iSizeNodo);
		memset(cBloquesNodo , 0 , sizeof(char) * iSizeNodo);

		// FIXME: memory leak
		//tNodo.tBloquesNodo = bitarray_create(cBloquesNodo,tNodo.bloques_totales);
		tNodo.tBloquesNodo = bitarray_create(cBloquesNodo , sizeof(char) * iSizeNodo);

		tNodo.activo = true;
		tNodo.bloques_reservados = 0;

		iReturn = NODO_Modificar(tIdNodo, tNodo);
		Macro_CheckError(iReturn,"Error al modificar el estado del Nodo");
	}

	// Buscar todos los bloques que tienen el nodo
	iReturn = BLOQUES_buscarBloqueNodo(tIdNodo, &tIdBloque, DB_SET, &iTotBloques);
	if (iTotBloques > 0) {
		Macro_CheckError(iReturn,"Error al Buscar los Bloques en el Nodo");
		for (iBloqAct = 0; iBloqAct < iTotBloques; iBloqAct++) {
			//Obtengo los Datos del Bloque
			iReturn = BLOQUES_Consultar(tIdBloque, &tBloques);
			Macro_CheckError(iReturn,"Error al consultar los datos del Bloque");
			// Verifico que el bloque tenga datos validos, por si se corto la comunicacion antes de que el bloque pase entero y quedo basura
// --- Modificado Julian 2015-07-21
// Todos los bloques tienen IdArchivo != 0, asi sean fallidos
//			if (tBloques.tIdArchivo == 0) {
//				iReturn = BLOQUES_Eliminar(tIdBloque);
//				Macro_CheckError(iReturn,"Error al consultar los datos del Bloque");
//			} else {

				bitarray_set_bit(tNodo.tBloquesNodo, tBloques.numeroBloqueDentroNodo);

				//Como estamos activando al Nodo porque esta Disponible, debemos marcar como disponible todos sus nodos
				tBloques.disponible = true;
				//Actualizo la Modificacion
				iReturn = BLOQUES_Modificar(tIdBloque, tBloques);
				Macro_CheckError(iReturn,"Error al al modificar los Datos del Bloque");

				//Reviso para Activar el Archivo del cual es parte el Bloque
				iReturn = FS_ActivarArchivo(tBloques.tIdArchivo );
				Macro_CheckError(iReturn,"Error al Activar el Archivo");
//			}
			//Busco el Siguiente Bloque del Nodo
			iReturn = BLOQUES_buscarBloqueNodo(tIdNodo, &tIdBloque, DB_NEXT_DUP, NULL);
			if (iReturn == DB_NOTFOUND) {
				break;
			} else {
				Macro_CheckError(iReturn,"Error al buscar los datos del Bloque en el Nodo");
			}
		}
	} else if (iReturn != DB_NOTFOUND) {
		Macro_CheckError(iReturn,"Error al Buscar los Bloques en el Nodo");
	}

	tNodo.activo = true;
	tNodo.bloques_reservados = 0;
	tNodo.bloques_usados = iTotBloques;
	tNodo.bloques_libres = tNodo.bloques_totales - tNodo.bloques_usados;

	iReturn = NODO_Modificar(tIdNodo, tNodo);
	Macro_CheckError(iReturn,"Error al modificar el estado del Nodo");

	//Si llegamos hasta aca es que se Ejecuto Bien, asi que devuelvo 1.
	return 1;
}



int FS_DesactivarNodo(char sNombre[NODO_LONGITUD_NOMBRE]) {
	int iReturn = 0;
	tipo_id tIdNodo;
	tipo_datos_nodo tNodo;
	tipo_id tIdBloque;
	int iTotBloques, iBloqAct;

	iReturn = NODO_BuscarNombre(sNombre, &tIdNodo, DB_SET, NULL);
	Macro_CheckError(iReturn,"Error al buscar el Nombre del Nodo");

	// --- Comentado por Julian 06062015
	// Doy de baja el nodo en el sistema
	//iReturn = FS_EliminarNodoActivo(&tIdNodo);
	//CheckError(iReturn,"Error al guardar Datos del Nodo en Memoria");
	// ---
	iReturn = NODO_Consultar(tIdNodo,&tNodo);
	Macro_CheckError(iReturn,"Error al consultar los datos del Nodo");

	tipo_datos_bloques tBloques;
	// Desactivo todos los bloques del nodo
	// Busco todos los bloques que tienen el nodo. Primero Buscamos al primer Bloque
	iReturn = BLOQUES_buscarBloqueNodo(tIdNodo, &tIdBloque, DB_SET, &iTotBloques);

	//Solo si tiene bloques debemos desactivarlos. Puede darse que un Nodo no tenga Bloques porque recien se conecto
	if(iReturn != DB_NOTFOUND){
		Macro_CheckError(iReturn,"Error al Buscar los Bloques en el Nodo");

		for (iBloqAct = 1; iBloqAct <= iTotBloques; iBloqAct++) {
			iReturn = BLOQUES_Consultar(tIdBloque , &tBloques);
			Macro_CheckError(iReturn , "Error al consultar los datos del Bloque");

			//Desactivamos el Bloque del Nodo
			tBloques.disponible = false;
			iReturn = BLOQUES_Modificar(tIdBloque , tBloques);
			Macro_CheckError(iReturn , "Error al al modificar los Datos del Bloque");

			// Verificar si algun archivo que contenia ese bloque se desactivo
			// y puede pasaar que la rutina sea llamada con una copia invalida mal subida
			// por lo que verifico que no pase
			if (tBloques.tIdArchivo != 0) {
				iReturn = FS_DesActivarArchivo(tIdBloque);
				Macro_CheckError(iReturn , "Error al Activar el Archivo");
			}
			// Buscar Archivo de tBloques.tIdArchivo si disponible = 0
			// Verifico si estan todos los bloques del archivo
			// y lo pongo en 1, si ya esta en 1, no me interesa
			iReturn = BLOQUES_buscarBloqueNodo(tIdNodo , &tIdBloque , DB_NEXT_DUP , NULL);

			//Cuando dejo de Encontrar Bloques Paro de Desactivarlos
			if (iReturn == DB_NOTFOUND) {
				break;
			}
			Macro_CheckError(iReturn , "Error al buscar los datos del Bloque en el Nodo");

			//Volvemos al FOR parae l Proximo Bloque
		}
	}
	//Si no tiene Bloques salteamos lo de Desactivar Bloques


	//Pongo al Nodo como Desactivado y destruyo su BitArray si fue creado (sino da error en COMMONS)
	tNodo.activo = false;
	if (tNodo.tBloquesNodo != NULL) {
		bitarray_destroy(tNodo.tBloquesNodo);
		tNodo.tBloquesNodo=NULL;
	}

	//Actualizo en la Base de Datos
	iReturn = NODO_Modificar(tIdNodo, tNodo);
	Macro_CheckError(iReturn,"Error al modificar el estado del Nodo");


	//Si llegamos hasta aca es que se Ejecuto Bien, asi que devuelvo 1.
	return iReturn;
}

/*
 * Eliminar todos los bloques de un archivo, necesito IdArchivo y Bloque del Archivo
*/ 

int FS_EliminarBloqueArchivo(tipo_id tIdArchivo, int iBloqueArch) {


	// Se borran todos los bloques del archivo, o se borra un bloque de un nodo
	int iReturn = 0;
	tipo_datos_bloques tBloque;
	tipo_id bloqueID;
	int  iCantBloques, iBloqActual;
	
	//Busco la Cantidad de Bloques del Archivo
	iReturn = BLOQUES_buscarBloqueArchivo(tIdArchivo , &bloqueID , DB_SET , &iCantBloques);
	Macro_CheckError(iReturn , "Error al buscar los Bloques del Archivo");

	//Recorro los Bloques
	for (iBloqActual = 1; iBloqActual <= iCantBloques; iBloqActual++) {
		iReturn = BLOQUES_Consultar(bloqueID , &tBloque);
		Macro_CheckError(iReturn , "Error al Consutlar los datos del Bloque");

		//Me fijo si es El numero de Bloque que buscaba, en ese caso lo elimino
		if ( tBloque.numeroBloqueDentroArchivo == iBloqueArch) {

			//Antes de Eliminar el Bloque, debo Actualizar en el Nodo que hay un Bloque Libre Mas
			tipo_datos_bloques bloqueDatos;
			iReturn = BLOQUES_Consultar(bloqueID, &bloqueDatos);
			Macro_CheckError(iReturn,"Error al obtener datos del bloque");

			tipo_datos_nodo nodoDatos;
			iReturn = NODO_Consultar(bloqueDatos.tIdNodo ,&nodoDatos);
			Macro_CheckError(iReturn,"Error al obtener datos del Nodo al cual Pertenece el Bloque");

			nodoDatos.bloques_libres++;
			nodoDatos.bloques_usados--;

			bitarray_clean_bit(nodoDatos.tBloquesNodo, bloqueDatos.numeroBloqueDentroNodo);

			iReturn = NODO_Modificar(bloqueDatos.tIdNodo, nodoDatos);
			Macro_CheckError(iReturn,"Error al Actualizar los datos del Nodo al cual Pertenece el Bloque");

			//Ahora si Elimino al Bloque

			iReturn = BLOQUES_Eliminar(bloqueID);
			Macro_CheckError(iReturn,"Error al Eliminar el Bloque");
		}

		//Busco el Siguiente Bloque del Archivo
		iReturn = BLOQUES_buscarBloqueArchivo(tIdArchivo , &bloqueID , DB_NEXT_DUP , NULL);
		Macro_CheckError(iReturn , "Error al Buscar los Bloques del Archivo");
	}

	return iReturn;	
}

/*
 * Dado un archivo, el numero del bloque, y el numero de copia, me devuelva el Bloque que lo contiene
*/

int FS_ObtenerCopiaBloqueArchivo(tipo_id tIdArchivo, int iBloqueArch, int iCopia, tipo_datos_bloques  *tBloqueArch) {
	int iReturn = 0;
	tipo_datos_bloques tBloque;
	tipo_id tIdBloque;
	int  iCantBloques, iBloqActual;
	//Busco la Cantidad de Bloques del Archivo
	iReturn = BLOQUES_buscarBloqueArchivo(tIdArchivo,&tIdBloque,DB_SET, &iCantBloques);
	Macro_CheckError(iReturn,"Error al Buscar los Bloques del Archivo");
	
	//Recorro los Bloques
	for (iBloqActual = 1;  iBloqActual <= iCantBloques; iBloqActual++) {
		//Obtengo los Datos del Bloque
		iReturn = BLOQUES_Consultar(tIdBloque, &tBloque );
		Macro_CheckError(iReturn,"Error al Consultar los datos del Bloque");

		//Me fijo si es el numero de bloque y copia que buscaba, en ese caso paro y devuelvo por referencia los datos
		if (( tBloque.numeroBloqueDentroArchivo == iBloqueArch) && ( tBloque.copia == iCopia)) {
			*tBloqueArch = tBloque;
			break;
		}
		//Busco el Siguiente Bloque del Archivo
		iReturn = BLOQUES_buscarBloqueArchivo(tIdArchivo,&tIdBloque,DB_NEXT_DUP, NULL);
		//Cuando dejo de Encontrar Bloques y es El Final del For, salgo del For
		if( (iBloqActual>=iCantBloques) && (iReturn==DB_NOTFOUND) ){
			//Cambio el iReturn pra que no quede con un Numero de Error
			iReturn = DB_NOTFOUND;
			break;
		} else {
			Macro_CheckError(iReturn,"Error al Buscar los Bloques del Archivo");
		}
	}
	
	return iReturn;	
}
/*
 * Dado un archivo y el bloque de archivo, me informe las copias disponibles
*/ 


int FS_CantidadCopiasBloqueArchivo(tipo_id tIdArchivo, int numeroBloqueDentroArchivo, int *iCantCopias) {
	int iReturn = 0;
	tipo_id tIdBloque;
	int Copias = 0;
	tipo_datos_bloques tBloque;

	//Busco todos los bloques de Dato de cierto archivo
	iReturn = BLOQUES_buscarBloqueEspecificoArchivo(tIdArchivo, numeroBloqueDentroArchivo, &tIdBloque, true);
	Macro_CheckError(iReturn,"Error al Buscar los Bloques del Archivo");
	do {
		//Obtengo los Datos del Bloque Actual
		iReturn = BLOQUES_Consultar(tIdBloque, &tBloque );
		Macro_CheckError(iReturn,"Error al consultar los datos del Bloque");

		if (tBloque.copia > Copias){
			Copias = tBloque.copia;
		}
		//Copias++;

		iReturn = BLOQUES_buscarBloqueEspecificoArchivo(tIdArchivo, numeroBloqueDentroArchivo, &tIdBloque, false);
		if (iReturn == DB_NOTFOUND) {
			iReturn = 0;
			break;
		} else {
			Macro_CheckError(iReturn,"Error al Buscar los Bloques del Archivo");
		}
	} while (iReturn != DB_NOTFOUND);

	//Devuelvo por Referencia la Cantidad de Copias de Ese Archivo para Ese Bloque
	*iCantCopias = Copias;
	
	return iReturn;	
}

/*
 * Solicitud de Archivo de Marta
*/

/*
 * Eliminar Nodo, tambien elimina registros de Bloques de Archivo
*/

/*
 * Copiar Archivo, copia archivo y todos los datos de los nodos
 */

int FS_abrir_indices(){
	//Variable para Controlar Errores, por defecto es 0 que significa que no hay error
	int iReturn = 0;
	//Le Saque la Macro y Lo hice a Mano asi Puedo hacer que en Error imprima con Estado Final Error tambien, sino imprime Sobre la Linea Anterior, una encima de la Otra y sale cualquier cosa..

	Macro_ImprimirEstadoInicio("- Iniciando Tablas del Sistema");
	iReturn = DB_TablasSistema();
	if (iReturn < 0) {
		Macro_ImprimirEstadoFinal(EJECUCION_FALLIDA);
		printf("No se pudieron abrir lor indices de Tablas del Sistema \n");
		return iReturn;
	}
	Macro_ImprimirEstadoFinal(EJECUCION_CORRECTA);

	Macro_ImprimirEstadoInicio("- Iniciando Indices de Directorios");
	iReturn = DIR_AbrirIndices();
	if (iReturn < 0) {
		Macro_ImprimirEstadoFinal(EJECUCION_FALLIDA);
		printf("No se pudieron abrir lor indices de los Directorios \n");
		return iReturn;
	}
	Macro_ImprimirEstadoFinal(EJECUCION_CORRECTA);

	Macro_ImprimirEstadoInicio("- Iniciando Indices de Nodo");
	iReturn = NODO_AbrirIndices();
	if (iReturn < 0) {
		Macro_ImprimirEstadoFinal(EJECUCION_FALLIDA);
		printf("No se pudieron abrir lor indices de los Nodos \n");
		return iReturn;
	}
	Macro_ImprimirEstadoFinal(EJECUCION_CORRECTA);

	Macro_ImprimirEstadoInicio("- Iniciando Indices de Bloques");
	iReturn = BLOQUES_AbrirIndices();
	if (iReturn < 0) {
		Macro_ImprimirEstadoFinal(EJECUCION_FALLIDA);
		printf("No se pudieron abrir lor indices de los Bloques \n");
		return iReturn;
	}
	Macro_ImprimirEstadoFinal(EJECUCION_CORRECTA);

	Macro_ImprimirEstadoInicio("- Iniciando Indices de Archivos");
	iReturn = FILE_AbrirIndices();
	if (iReturn < 0) {
		Macro_ImprimirEstadoFinal(EJECUCION_FALLIDA);
		printf("No se pudieron abrir lor indices de los Archivos \n");
		return iReturn;
	}
	Macro_ImprimirEstadoFinal(EJECUCION_CORRECTA);


	//Si llegamos hasta aca es que se pudieron abrir bien los Indices
	return iReturn;
}

/*
 * --- Comentado por Julian 06062015
int FS_ListarNodosActivos(tipo_id *pIdNodo, int iNumNodo){
	int iReturn = 0;

	iReturn = FS_NodosActivos(pIdNodo,FS_NODO_LISTAR,iNumNodo,0);
	CheckError(iReturn,"No se pudo listar un nodo activo");

	return iReturn;
}
*/

/*
 * --- Comentado por Julian 06062015
int FS_EliminarNodoActivo(tipo_id *pIdNodo){
	int iReturn = 0;

	iReturn = FS_NodosActivos(pIdNodo,FS_NODO_ELIMINAR,0,0);
	CheckError(iReturn,"No se pudo eliminar un nodo activo");

	return iReturn;
}
*/

/*
 * --- Comentado por Julian 06062015
int FS_AgregarNodoActivo(tipo_id *pIdNodo){
	int iReturn = 0;
	tipo_datos_nodo tNodo;

	iReturn = FS_NodosActivos(pIdNodo,FS_NODO_AGREGAR,0,0);
	CheckError(iReturn,"No se pudo agregar un nodo activo");

	return iReturn;
}
*/

int FS_ContarNodosActivos(int *iCant){
	int iReturn = 0;

	// --- Comentado por Julian 06062015
	//iReturn = FS_NodosActivos(0,FS_NODO_CONTAR,0,iCant);
	//CheckError(iReturn,"No se pudo contar los nodos activos");
	// ---

	tipo_id id, tIdUltimo;
	tipo_datos_nodo tNodo;
	int icantNodosActivos = 0;

	iReturn = DB_getUltimoId("nodo",&tIdUltimo);
	Macro_CheckError(iReturn,"Error al obtener el ultimo Id de Nodo");

	for (id = 1; id < tIdUltimo; id++) {
		iReturn = NODO_Consultar(id , &tNodo);
		if (iReturn == DB_NOTFOUND) {
			continue;
		} else {
			Macro_CheckError(iReturn , "Error al consultar Nodo Id");

			//Veo si esta Activo para Contarlo, sino no lo cuento
			if (tNodo.activo == true) {
				icantNodosActivos++;
			}
		}
	}

	*iCant = icantNodosActivos;
	if (icantNodosActivos == 0) {
		iReturn = 0;
	}
	return iReturn;
}

/*
 * --- Comentado por Julian 06062015
int FS_IniciarRegistroNodos(){
	int iReturn = 0;

	iReturn = FS_NodosActivos(0,FS_NODO_RESET,0,0);
	CheckError(iReturn,"No se pudo reiniciar los nodos activos");

	return iReturn;
}
*/

/*
 * --- Comentado por Julian 06062015
int FS_NodosActivos(tipo_id *ptIdNodo, t_operacion_nodo estado, int iNumNodo, int *iTotNodos) {
	static tipo_id *tIdNodos;
	static int numNodos;
	int iReturn = 0;
	tipo_id id;

	if (iNumNodo < 0) {
		iReturn = -1;
		CheckError(iReturn,"La ubicacicion del numero de Nodo no puede ser negativa");
	}

	if (estado == FS_NODO_RESET) {
		numNodos = 0;
		tIdNodos = malloc(sizeof(tipo_id));
		memset(tIdNodos,0,sizeof(tipo_id));
	}
	if (estado == FS_NODO_LISTAR) {
		if (ptIdNodo == NULL) {
			iReturn = -1;
			CheckError(iReturn,"No se indico un parametro donde devolver el Id del Nodo Activo");
		}
		if (ptIdNodo != 0) {
			*ptIdNodo = tIdNodos[iNumNodo];
		}
	}
	if (estado == FS_NODO_AGREGAR) {
		if (ptIdNodo == NULL) {
			iReturn = -1;
			CheckError(iReturn,"No se indico un Id del Nodo Activo valido");
		}
		tIdNodos[numNodos] = *ptIdNodo;
		numNodos++;
		tIdNodos = realloc(tIdNodos,sizeof(tipo_id)*(numNodos+1));
	}
	if (estado == FS_NODO_ELIMINAR) {
		if (ptIdNodo == NULL) {
			iReturn = -1;
			CheckError(iReturn,"No se indico un Id del Nodo Activo valido");
		}
		// Aca no tendrias que borrar lo que tiene en tIdNodos[id+1 ?? O asignarlo lo que tiene su siguiente??
		// a partir de que encuentra el nodo, se setea el flag baja, por lo que elimina el elegido, y empieza a copiar hacia atras
		// me queda un espacio libre, que finalmente se elimina al hacer un realloc
		int baja = 0;
		for (id=0;id<numNodos;id++) {
			if(tIdNodos[id] == *ptIdNodo) {
				baja = 1;
			}
			if (baja == 1) {
				tIdNodos[id] = tIdNodos[id+1];
			}
		}
		if (baja == 1) {
			numNodos--;
			tIdNodos = realloc(tIdNodos,sizeof(tipo_id)*(numNodos+1));
		}
	}
	if (estado == FS_NODO_CONTAR) {
		if (iTotNodos == NULL) {
			iReturn = -1;
			CheckError(iReturn,"No se indico una variable para contar el numero de nodos");
		}
		if (iTotNodos != 0) {
			*iTotNodos = numNodos;
		}
	}
	return iReturn;
}
*/

int FS_CargarNodos() {
	tipo_id id, tIdUltimo, tIdUltimoArchivo;
	tipo_datos_nodo tNodo;
	int iReturn = 0;
	// Inicializo el Array por primera y unica Vez
	// --- Comentado por Julian 06062015
	//iReturn = FS_IniciarRegistroNodos();
	//CheckError(iReturn,"Error al inicializar los nodos");
	// ---

	//Obtengo el Ultimo ID de la Tabla Nodos, para poder Recorrer en un FOR toda la Tabla Viendo de Activar/Desactivar Nodos...
	iReturn = DB_getUltimoId("nodo",&tIdUltimo);
	Macro_CheckError(iReturn,"Error al obtener el ultimo Id de Nodo");


	// Pongo todos los nodos en desactivados por si hubo alguna salida mal del sistema
	for (id = 1; id < tIdUltimo; id++) {
		//Obtengo los Datos del Nodo
		iReturn = NODO_Consultar(id,&tNodo);
		//Si no pude obtener los Datos porque no lo encuentra, que siga de largo al proximo nodo (este nodo no se activa ni desactiva)
		if (iReturn == DB_NOTFOUND) {
			continue;

		} else {
		//Antes de activarlo reviso que no paso nada raro
			Macro_CheckError(iReturn , "Error al consultar Nodo Id");
			tNodo.activo = false;
			tNodo.tBloquesNodo = NULL;
			iReturn = NODO_Modificar(id,tNodo);
			Macro_CheckError(iReturn , "Error al actualizar datos del Nodo");
		}
	}

	// Antes de cargar los nodos, limpio los archivos que puedieran estar mal cargados
	// antes una interrupcion abrupta del FS
	tipo_datos_file tArchivo;
	iReturn = DB_getUltimoId("file",&tIdUltimoArchivo);
	tipo_id tIdArchivoActual;

	for (tIdArchivoActual = 1; tIdArchivoActual <= tIdUltimoArchivo; tIdArchivoActual++) {
		iReturn = FILE_ConsultarId(tIdArchivoActual, &tArchivo);
		if (iReturn == DB_NOTFOUND) {
			continue;
		} else {
			Macro_CheckError(iReturn,"Error al consultar Datos del Archivo");
			if (tArchivo.iCantBloques == 0) {
				FS_EliminarArchivo(tIdArchivoActual);
			}
		}
	}

	for (id = 1; id < tIdUltimo; id++) {
		//Obtengo los Datos del Nodo
		iReturn = NODO_Consultar(id,&tNodo);
		//Si no pude obtener los Datos porque no lo encuentra, que siga de largo al proximo nodo (este nodo no se activa ni desactiva)
		if (iReturn == DB_NOTFOUND) {
			continue;

		}

		//Antes de activarlo reviso que no paso nada raro
		Macro_CheckError(iReturn , "Error al consultar Nodo Id");

		// --- Comentado por Julian 06062015
		//iReturn = FS_AgregarNodoActivo(&id);
		//CheckError(iReturn, "Error al Agregar el nodo a Memoria");
		// ---

		//Veo si esta Disponible para Activarlo
		if ((Sockets_estaDisponibleServidor(tNodo.nodoIP , tNodo.nodoPuerto))) {
			//Como esta Disponible, lo activo
			iReturn = FS_ActivarNodo(tNodo.sNombre);
			if(iReturn<0){
				Macro_ImprimirParaDebug("Error al Activar el Nodo \n");
			}
			printf("- Se encontro y activo el Nodo: %s\n" , tNodo.sNombre);
		} else {
			//Como NO esta Disponible, debo Desactivarlo
			//Como hacemos que el Nodo se desactive al Apagar el FS, cuando inicia el FS estos estan desactvados y no hacen falta desactivarlos de vuelta
			//iReturn = FS_DesactivarNodo(tNodo.sNombre);
			//Macro_CheckError(iReturn , "Error al desactivar el Nodo");

			//FIXME Creo que habria que cambiar los "Macro_CheckError" por ifs que solo logueen y hagan continue, porque si hay un error al desactivar/activar un nodo se Para el FS porque esta funcion devuelve Negativo  para el Main
			printf("- El Nodo %s se encuentra desactivado\n" , tNodo.sNombre);
		}
		// colgarse el sistema

	}
	return EJECUCION_CORRECTA;
}



tipo_id FS_Validar_Archivo_Ruta(char*archivoNombre, char* archivoRuta) {
	//Variable para Manejar Errores, inicializo en 0 que significa no hay errores.
	int iNumeroError = 0;
	//Variable para guardar el ID de Archivo
	tipo_id archivoID = 0;
	//Variable para guardar el "supuesto" ID del Directorio Padre al Archivo
	tipo_id directorioPadreSupuestoID = 0;
	//Variable para Datos del Archivo
	tipo_datos_file archivoDatos;

	//Verifico que exista la Ruta del Archivo y ya que estoy, obtengo el ID del Supuesto Padre del archivo (ultimo Directorio de la Ruta)
	directorioPadreSupuestoID = DIR_validarRuta(archivoRuta, 0);
	if (directorioPadreSupuestoID == -1) {
		Macro_ImprimirParaDebug(ANSI_COLOR_RED "La ruta ingresada no existe\n" ANSI_COLOR_RESET);
		return -1;
	}

	//Ahora Verifico archivo exista dentro de ese Directorio y no en cualquier otro

	//Primero Le digo a la Base de Datos que me busque el archivoNombre y me obtenga su ID.
	int cantidadArchivosConMismoNombre = -1;
	iNumeroError = FILE_BuscarNombre(archivoNombre, &archivoID, DB_SET, &cantidadArchivosConMismoNombre);
	//Chequeo Errores
	if ((iNumeroError < 0) || (archivoID <= 0)) {
		Macro_ImprimirParaDebug(ANSI_COLOR_RED "No existe el archivo indicado\n" ANSI_COLOR_RESET);
		return -2;
	}

	//Segundo Busco si de todos los archivos con mismo Nombre Hay 1 solo que cumpla que este JUSTO en esa ruta. Sino, doy error
	int archivoActual;
	for (archivoActual = 1; archivoActual <= cantidadArchivosConMismoNombre; archivoActual++) {

		//Obtengo el Directorio Padre Real que tiene el archivo
		iNumeroError = FILE_ConsultarId(archivoID , &archivoDatos);
		//Chequeo Errores
		if (iNumeroError < 0) {
			Macro_ImprimirParaDebug(ANSI_COLOR_RED "Esto es Raro, si bien existe el archivo (o uno con ismo nombre) no se pueden obtener sus Datos\n" ANSI_COLOR_RESET);
			return -4;
		}

		//Comparo que tenga como Directorio Padre el que valide anteriomente
		if ((directorioPadreSupuestoID) == (archivoDatos.dirPadre)) {
			//Es el Archivo que Busco! Devuelvo el ID del archivo (es mayor a 0 siempre) porque existe el archivo pedido Justo en la Ruta pedida
			return archivoID;
		}
		//Como No coinciden los ID de los Padres. Pidieron un Archivo que existe pero no esta en esa Ruta asi que vamos al siguiente

		//En caso de que llegamos al final del While no se ejecuta la siguiente linea porque explota siempre en la Base ya que no hay mas archivos con mismo nombre, hago que salga del for y de Error
		if (archivoActual == cantidadArchivosConMismoNombre) {
			break;
		}

		//Busco al siguiente archivo con mismo Nombre, para ver si ese es el que piden
		iNumeroError = FILE_BuscarNombre(archivoNombre , &archivoID , DB_NEXT_DUP , &cantidadArchivosConMismoNombre);
		//Chequeo Errores
		if (iNumeroError < 0) {
			Macro_ImprimirParaDebug(ANSI_COLOR_RED "Exploto en la Base al buscar otro Archivo con mismo Nombre\n" ANSI_COLOR_RESET);
			return -5;
		}
	}

	//Si llegamos hasta aca es que reviso todos los archivos y ninguno era el que queria, devuelvo -3
	Macro_ImprimirParaDebug(ANSI_COLOR_RED "Pediste un Archivo que No esta JUSTO en esa Ruta, por lo que NO es el archivo que Pedias\n" ANSI_COLOR_RESET);
	return -3;
}


bool FS_VerificarEspacioParaCopias(int iCantBloqueArchivo, int iCantCopias, tipo_id ***Reservado) {
	// Esta es una aproximacion a la idea de Leo
	// iNodo es un Array Dinamico
	// Esta funcion va a recibir la cantidad de Copias que tiene que hacer de un Bloque de Archivo
	// Y la cantidad de Bloques de Archivos que hay y va a generar un Array con el Numero de Nodo
	// y el numero de Bloque donde se van a Guarda, para pasarlos despues a la funcion Cargar_Archivo
	// En caso de que no pueda encontrar el espacio disponible, devuelve Error

	int iReturn = 0;
	bool bReturn = true;
	int iBloqueArchivoAct;//, iCopiaActual;
	// Se reemplaza por una funcion que devuelve los datos
/*	tipo_id IdNodoActual, IdUltimoNodo;
	tipo_datos_nodo DatosDelNodo;
	IdNodoActual = 1;
	DB_getUltimoId("nodo",&IdUltimoNodo);

	// Creo y lleno un array dinamico con el espacio disponible por nodo
	int *iEspacioNodo;
	iEspacioNodo = malloc(sizeof(int));
	memset(iEspacioNodo, 0, sizeof(int));
	// Uso una lista doble porque los Id no tienen porque se correlativos
	tipo_id *IdDelNodo;
	IdDelNodo = malloc(sizeof(tipo_id));
	memset(IdDelNodo, 0, sizeof(tipo_id));
	int iPosicionArray = 0;
	//Completo el Array
	do {
		iReturn = NODO_Consultar(IdNodoActual,&DatosDelNodo);
		if (iReturn != DB_NOTFOUND) {
			Macro_CheckError(iReturn,"Error al consultar los datos del Nodo");
			if (DatosDelNodo.activo == true) {
				iEspacioNodo[iPosicionArray] = DatosDelNodo.bloques_libres - DatosDelNodo.bloques_reservados;
				IdDelNodo[iPosicionArray] = IdNodoActual;
				iPosicionArray++;
				iEspacioNodo = realloc(iEspacioNodo,sizeof(int) * (iPosicionArray+1));
				IdDelNodo = realloc(IdDelNodo, sizeof(tipo_id) * (iPosicionArray+1));
			}
		}
		IdNodoActual++;
	} while (IdNodoActual < IdUltimoNodo);*/
	int *iEspacioNodo = NULL;
//	iEspacioNodo = malloc(sizeof(int));
//	memset(iEspacioNodo, 0, sizeof(int));
	tipo_id *IdDelNodo = NULL;
//	IdDelNodo = malloc(sizeof(tipo_id));
//	memset(IdDelNodo, 0, sizeof(tipo_id));
	int iPosicionArray;
	iReturn = FS_crearArrayNodosDisp(&iEspacioNodo, &IdDelNodo, &iPosicionArray);


	// Array Bidimensional Dinamico para reservar bloques
	tipo_id **NodosReservados;


	//int iPosActual;
	// Ahora adopto una vision positiva de la vida, y presumo que voy a tener siempre espacio!
	for (iBloqueArchivoAct = 1; iBloqueArchivoAct <= iCantBloqueArchivo; iBloqueArchivoAct++) {
		if (iBloqueArchivoAct == 1) {
			NodosReservados = malloc(sizeof(tipo_id*));
		} else {
			NodosReservados = realloc(NodosReservados,sizeof(tipo_id*)*iBloqueArchivoAct);
		}
		tipo_id *NodosDestino = NULL;
		//memset(NodosDestino,0,sizeof(tipo_id)*iCantCopias);
		//NodosReservados[iBloqueArchivoAct-1] = malloc(sizeof(tipo_id) * iCantCopias);
		iReturn = FS_ObtenerNodosDondeCopiar(iEspacioNodo, IdDelNodo, iPosicionArray, iCantCopias, &NodosDestino);
		if (iReturn < 0) {
			NodosReservados[iBloqueArchivoAct-1] = NULL;
			iBloqueArchivoAct--;
			int iCopiaActual;
			while (iBloqueArchivoAct > 0) {
				for (iCopiaActual = 1; iCopiaActual <= iCantCopias; iCopiaActual++)	{
					FS_DesMarcarBloquesResevadosNodo(NodosReservados[iBloqueArchivoAct-1][iCopiaActual-1]);
				}
				free(NodosReservados[iBloqueArchivoAct-1]);
				NodosReservados[iBloqueArchivoAct-1] = NULL;
				iBloqueArchivoAct--;
			}
			// TODO: Ver si no falta liberar el bloque 0 a copiar
			bReturn = false;
			free(NodosReservados);
			NodosReservados = NULL;
			break;
		}
		NodosReservados[iBloqueArchivoAct-1] = NodosDestino;
	}
	free(IdDelNodo);
	free(iEspacioNodo);
	//free(NodosReservados);
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
	// Siempre devuelve el valor para liberarlo despues
	*Reservado = NodosReservados;
#pragma GCC diagnostic pop
	return bReturn;
}

bool FS_ReasignarEspacioParaCopias(int iCantBloqueArchivo, int BloqueDesde, int iCantCopias, tipo_id ***NodosReserv) {
	int iReturn = 0;
	bool bReturn = true;
	int iBloqueArchivoAct;//, iCopiaActual;
	int *iEspacioNodo = NULL;
	tipo_id *IdDelNodo = NULL;
	int iPosicionArray;
	tipo_id *NodosDestino;
	int iPosActual;
	tipo_id **NodosReservados;
	NodosReservados = *NodosReserv;

	// Voy a liberar las reservas de las copias que quedaron pendientes
	for (iBloqueArchivoAct = BloqueDesde; iBloqueArchivoAct <= iCantBloqueArchivo; iBloqueArchivoAct++) {
		for (iPosActual = 0; iPosActual < iCantCopias; iPosActual++) {
			if (iBloqueArchivoAct != BloqueDesde){
				FS_DesMarcarBloquesResevadosNodo(NodosReservados[iBloqueArchivoAct-1][iPosActual]);
			}
		}
	}

	// Liberadas las reservas, vuelvo a tener el array con el espacio libre en los nodos
	iReturn = FS_crearArrayNodosDisp(&iEspacioNodo, &IdDelNodo, &iPosicionArray);

	// Ahora adopto una vision positiva de la vida, y presumo que voy a tener siempre espacio!
	for (iBloqueArchivoAct = BloqueDesde; iBloqueArchivoAct <= iCantBloqueArchivo; iBloqueArchivoAct++) {
		// Voy a pedir n Cantidad de Copias Nuevas
		iReturn = FS_ObtenerNodosDondeCopiar(iEspacioNodo, IdDelNodo, iPosicionArray, iCantCopias, &NodosDestino);
		if (iReturn < 0) {
			iBloqueArchivoAct--;
			int iCopiaActual;
			while (iBloqueArchivoAct >= BloqueDesde) {
				for (iCopiaActual = 1; iCopiaActual <= iCantCopias; iCopiaActual++)	{
					FS_DesMarcarBloquesResevadosNodo(NodosReservados[iBloqueArchivoAct-1][iCopiaActual-1]);
				}
				free(NodosReservados[iBloqueArchivoAct-1]);
				iBloqueArchivoAct--;
			}
			bReturn = false;
			break;
		}

		// Voy a comparar una por una con las que ya tengo y las que sean diferentes las cambio
		for (iPosActual = 0; iPosActual < iCantCopias; iPosActual++) {
			if (NodosDestino[iPosActual] != NodosReservados[iBloqueArchivoAct-1][iPosActual]) {
				NodosReservados[iBloqueArchivoAct-1][iPosActual] = NodosDestino[iPosActual];
			}
		}

	}
	*NodosReserv = NodosReservados;
	free(IdDelNodo);
	free(iEspacioNodo);
	return bReturn;
}


int FS_ObtenerNodosDondeCopiar(int *Espacio, tipo_id *IdNodos, int TamanoArray, int iCantCopias, tipo_id **NodosDes) {
	int iReturn = 0;
	int iCopiaActual, iPosActual;
	tipo_id iNodoActual;
	int iPosNodoElegido;
	bool bReturn;
	tipo_id *NodosDestino;
	// FIXME: memory leak
	NodosDestino = malloc(sizeof(tipo_id) * iCantCopias);

	for (iCopiaActual = 1; iCopiaActual <= iCantCopias; iCopiaActual++) {
		bReturn = false;
		int EspacioLibre = 0;
		for (iPosActual = 0 ; iPosActual < TamanoArray; iPosActual++) {
			if (Espacio[iPosActual] > 0) {
				if (iCopiaActual > 1) {
					bool bEsDistinto = true;
					for (iNodoActual=1;iNodoActual < iCopiaActual; iNodoActual++) {
						if (NodosDestino[iNodoActual-1] == IdNodos[iPosActual]) {
							bEsDistinto = false;
							break;
						}
					}
					if (bEsDistinto == true) {
						if (Espacio[iPosActual] > EspacioLibre) {
							EspacioLibre = Espacio[iPosActual];
							iPosNodoElegido = iPosActual;
							bReturn = true;
						}
					}
				} else {
					if (Espacio[iPosActual] > EspacioLibre) {
						EspacioLibre = Espacio[iPosActual];
						iPosNodoElegido = iPosActual;
						bReturn = true;
					}
				}
			}
		}
		if (bReturn == false) {
			// En caso de error, libero los bloques ya marcados
			for (iPosActual = 1; iPosActual < iCopiaActual; iPosActual++ ) {
				FS_DesMarcarBloquesResevadosNodo(IdNodos[iPosActual-1]);
			}
			free(NodosDestino);
			NodosDestino = NULL;
			iReturn = -1;
			break;
		}
		NodosDestino[iCopiaActual-1] = IdNodos[iPosNodoElegido];
		Espacio[iPosNodoElegido]--;
		iReturn = FS_MarcarBloquesResevadosNodo(IdNodos[iPosNodoElegido]);
		Macro_CheckError(iReturn,"Error de base de datos al resevar bloques en un nodo");
	}

	*NodosDes = NodosDestino;
	return iReturn;
}

int FS_MarcarBloquesResevadosNodo(tipo_id IdNodo) {
	int iReturn = 0;
	tipo_datos_nodo DatosDelNodo;

	iReturn = NODO_Consultar(IdNodo,&DatosDelNodo);
	Macro_CheckError(iReturn,"Error al consultar los datos del Nodo");

	DatosDelNodo.bloques_reservados++;

	iReturn = NODO_Modificar(IdNodo, DatosDelNodo);
	Macro_CheckError(iReturn,"Error al modificar los datos del Nodo");

	return iReturn;
}

int FS_DesMarcarBloquesResevadosNodo(tipo_id IdNodo) {
	int iReturn = 0;
	tipo_datos_nodo DatosDelNodo;

	iReturn = NODO_Consultar(IdNodo,&DatosDelNodo);
	Macro_CheckError(iReturn,"Error al consultar los datos del Nodo");

	DatosDelNodo.bloques_reservados--;

	iReturn = NODO_Modificar(IdNodo, DatosDelNodo);
	Macro_CheckError(iReturn,"Error al modificar los datos del Nodo");

	return iReturn;
}

int FS_crearArrayNodosDisp(int **Espacio, tipo_id **IdNodos, int *TamanioArray) {
	int iReturn = 0;
	tipo_id IdNodoActual, IdUltimoNodo;
	tipo_datos_nodo DatosDelNodo;
	IdNodoActual = 1;
	DB_getUltimoId("nodo",&IdUltimoNodo);
	Macro_CheckError(iReturn,"Error al consultar el Id del Ultimo Nodo");

	// Creo y lleno un array dinamico con el espacio disponible por nodo
	int *iEspacioNodo;
	iEspacioNodo = malloc(sizeof(int));
	memset(iEspacioNodo, 0, sizeof(int));
	// Uso una lista doble porque los Id no tienen porque se correlativos
	tipo_id *IdDelNodo;
	IdDelNodo = malloc(sizeof(tipo_id));
	memset(IdDelNodo, 0, sizeof(tipo_id));
	int iPosicionArray = 0;
	//Completo el Array
	do {
		iReturn = NODO_Consultar(IdNodoActual,&DatosDelNodo);
		if (iReturn != DB_NOTFOUND) {
			Macro_CheckError(iReturn,"Error al consultar los datos del Nodo");
			if (DatosDelNodo.activo == true) {
				iPosicionArray++;
				iEspacioNodo = realloc(iEspacioNodo,sizeof(int) * (iPosicionArray));
				IdDelNodo = realloc(IdDelNodo, sizeof(tipo_id) * (iPosicionArray));
				iEspacioNodo[iPosicionArray-1] = DatosDelNodo.bloques_libres - DatosDelNodo.bloques_reservados;
				IdDelNodo[iPosicionArray-1] = IdNodoActual;
			}
		}
		IdNodoActual++;
	} while (IdNodoActual < IdUltimoNodo);
	*IdNodos = IdDelNodo;
	*Espacio = iEspacioNodo;
	*TamanioArray = iPosicionArray;

	return iReturn;
}

int FS_LiberarEspacioReservado(int iCantBloqueArchivo, int iCantCopias, tipo_id **Reservado) {
	int iReturn =0;
	int iBloqueArchivoAct, iPosActual;

	for (iBloqueArchivoAct = 1; iBloqueArchivoAct <= iCantBloqueArchivo; iBloqueArchivoAct++) {
		if (Reservado[iBloqueArchivoAct-1] != NULL) {
			for (iPosActual = 0; iPosActual < iCantCopias; iPosActual++) {
				FS_DesMarcarBloquesResevadosNodo(Reservado[iBloqueArchivoAct-1][iPosActual]);
			}
		} else {
			break;
		}
	}

	return iReturn;
}


