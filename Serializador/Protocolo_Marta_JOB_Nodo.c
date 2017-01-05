#include <stdlib.h>		//Para Malloc y Free
#include <string.h>		//Para Memcpy

#include <commons/string.h>

#include "Protocolo_Marta_JOB_Nodo.h"

//Constantes Internas para agregar Expresividad y legibilidad
#define TAMANIO_CONEXION_INICIAL  	sizeof(tipo_Datos_Conexion_Inicial)
#define TAMANIO_RESPUESTA_MARTA  	sizeof(tipo_Datos_RespuestaMarta)
#define TAMANIO_NODO_EXTERNO  		sizeof(tipo_nodoExterno)

//Serializadores Internos
char* Serializar_nodoExterno(const tipo_nodoExterno datosPorSerializar);
//Recordar Hacer Free del Char*
tipo_nodoExterno* DesSerializar_nodoExterno(const char* datosSerializados);
//Recordar que usa Malloc, para poder agregarlos a la Lista al Des-Serializar

char* Serializar_ListaNodoExterno(const t_list* listaPorSerializar, uint32_t* tamanioSerializacion);
//Recordar Hacer Free del Char*
t_list* DesSerializar_ListaNodoExterno(const char* listaSerializada);

char* Serializar_DatosConexionInicial(const tipo_Datos_Conexion_Inicial datosPorSerializar,
		uint32_t* tamanioSerializacion) {
	Macro_ImprimirParaDebug("Serializando Datos Conexion Inicial..\n");

	char* datosSerializados = malloc( TAMANIO_CONEXION_INICIAL);
	uint32_t offset = 0;

	memcpy(datosSerializados, datosPorSerializar.jobPuerto, sizeof(datosPorSerializar.jobPuerto));
	offset = sizeof(datosPorSerializar.jobPuerto);

	memcpy(datosSerializados + offset, &datosPorSerializar.soportaCombiner, sizeof(datosPorSerializar.soportaCombiner));
	offset += sizeof(datosPorSerializar.soportaCombiner);

	memcpy(datosSerializados + offset, datosPorSerializar.rutaYnombreArchivoFinal,
			sizeof(datosPorSerializar.rutaYnombreArchivoFinal));

	*tamanioSerializacion = TAMANIO_CONEXION_INICIAL;
	return datosSerializados;
}

tipo_Datos_Conexion_Inicial DesSerializar_DatosConexionInicial(const char* datosSerializados) {
	Macro_ImprimirParaDebug("DesSerializando Datos Conexion Inicial..\n");

	tipo_Datos_Conexion_Inicial datosConexion;
	uint32_t offset = 0;

	memcpy(datosConexion.jobPuerto, datosSerializados, sizeof(datosConexion.jobPuerto));
	offset = sizeof(datosConexion.jobPuerto);

	memcpy(&datosConexion.soportaCombiner, datosSerializados + offset, sizeof(datosConexion.soportaCombiner));
	offset += sizeof(datosConexion.soportaCombiner);

	memcpy(datosConexion.rutaYnombreArchivoFinal, datosSerializados + offset,
			sizeof(datosConexion.rutaYnombreArchivoFinal));

	return datosConexion;
}

char* Serializar_listaArchivos(const t_list* listaPorSerializar, uint32_t* tamanioSerializacion) {
	Macro_ImprimirParaDebug("Serializando Lista Archivos..\n");

	//Lo primero que voy a hacer es obtener el tamaño de Elementos asi puedo hacer el malloc. Voy a ir sumando los String Length +1
	uint32_t cantidadElementos = list_size((t_list*) listaPorSerializar);
	Macro_ImprimirParaDebug("Tamanio Lista:'%d'\n", cantidadElementos);

	uint32_t elementoActual;
	uint32_t sumadorTamanioStrings = 0;

	for (elementoActual = 0; elementoActual < cantidadElementos; elementoActual++) {
		//NO estoy seguro si debe ir "<" o "<=", porque no se como devuelve bien el "list_size". Pareceria ser que va "<" por los tests de las commons (ver https://github.com/sisoputnfrba/so-commons-library/blob/master/tests/unit-tests/test_list.c#L116-L141 )
		sumadorTamanioStrings += strlen((char*) list_get((t_list*) listaPorSerializar, elementoActual)) + 1;
	}

	//Voy a serializar la cantidad de elementos + los elementos en si
	char* listaSerializada = malloc(sizeof(uint32_t) + sumadorTamanioStrings);

	//Lo Segundo es serializar la cantidad de elementos de la lista, asi me facilita el DesSerializar
	uint32_t offset = 0;
	memcpy(listaSerializada, &cantidadElementos, sizeof(uint32_t));
	offset = sizeof(uint32_t);

	//Ahora voy a serializar cada Elemento de la Lista, para eso usare un FOR
	for (elementoActual = 0; elementoActual < cantidadElementos; elementoActual++) {
		//NO estoy seguro si debe ir "<" o "<=", porque no se como devuelve bien el "list_size". Pareceria ser que va "<" por los tests de las commons (ver https://github.com/sisoputnfrba/so-commons-library/blob/master/tests/unit-tests/test_list.c#L116-L141 )

		char* StringPorSerializar;

		//Obtengo el Elemento (Sin eliminarlo de la lista), lo serializo y aumento el offset
		StringPorSerializar = list_get((t_list*) listaPorSerializar, elementoActual);

		memcpy(listaSerializada + offset, StringPorSerializar, strlen(StringPorSerializar) + 1);
		offset += strlen(StringPorSerializar) + 1;

		Macro_ImprimirParaDebug("Serializado Archivo:'%s'\n", StringPorSerializar);

	}

	//Ya se serializaron todos los elementos de la Lista, asi que ahora digo cuanto es el tamanio de la serializacion
	*tamanioSerializacion = sizeof(uint32_t) + sumadorTamanioStrings;
	return listaSerializada;
}

t_list* DesSerializar_listaArchivos(const char* listaSerializada) {
	Macro_ImprimirParaDebug("DesSerializando Lista Archivos..\n");

	//Primero voy a obtener la cantidad de elementos que hay en la serializacion, asi lo uso en el FOR
	uint32_t cantidadElementos;

	uint32_t offset = 0;
	memcpy(&cantidadElementos, listaSerializada, sizeof(uint32_t));
	offset = sizeof(uint32_t);

	Macro_ImprimirParaDebug("Cantidad Elementos Serializados Lista:'%d'\n", cantidadElementos);

	//Creo la Lista por Serializar
	t_list* listaPorDevolver = list_create();

	//Ahora voy a Des-Serializar cada Elemento de la Lista, para eso usare un FOR
	uint32_t elementoActual;
	for (elementoActual = 0; elementoActual < cantidadElementos; elementoActual++) {
		//NO estoy seguro si debe ir "<" o "<=", porque no se como devuelve bien el "list_size". Pareceria ser que va "<" por los tests de las commons (ver https://github.com/sisoputnfrba/so-commons-library/blob/master/tests/unit-tests/test_list.c#L116-L141 )

		//Obtengo el Char*, lo duplico con Malloc y Funcion Commons y agrego a la lista.
		char* StringDuplicado;
		StringDuplicado = string_duplicate((char*) listaSerializada + offset);
		offset += strlen(StringDuplicado) + 1;

		list_add(listaPorDevolver, StringDuplicado);

		Macro_ImprimirParaDebug("Nombre Archivo Des-Serializado:'%s'\n", StringDuplicado);
	}

	//Ya obtuve todos los elementos, asi que estoy listo para devolver la lista

	return listaPorDevolver;

}

void destructor_elementoListaArchivo(char* stringPorLiberar) {
	if (stringPorLiberar != NULL) {
		free(stringPorLiberar);
	}
}

char* Serializar_RespuestaMarta(const tipo_Datos_RespuestaMarta datosPorSerializar, uint32_t* tamanioSerializacion) {
	Macro_ImprimirParaDebug("Serializando Datos Respuesta Marta..\n");

	char* datosSerializados = malloc( TAMANIO_RESPUESTA_MARTA);
	uint32_t offset = 0;

	memcpy(datosSerializados, datosPorSerializar.nodoIP, sizeof(datosPorSerializar.nodoIP));
	offset = sizeof(datosPorSerializar.nodoIP);

	memcpy(datosSerializados + offset, datosPorSerializar.nodoPuerto, sizeof(datosPorSerializar.nodoPuerto));
	offset += sizeof(datosPorSerializar.nodoPuerto);

	memcpy(datosSerializados + offset, &datosPorSerializar.tipoDeOrden, sizeof(datosPorSerializar.tipoDeOrden));
	offset += sizeof(datosPorSerializar.tipoDeOrden);

	memcpy(datosSerializados + offset, &datosPorSerializar.terminacionCorrecta,
			sizeof(datosPorSerializar.terminacionCorrecta));

	*tamanioSerializacion = TAMANIO_RESPUESTA_MARTA;
	return datosSerializados;
}

tipo_Datos_RespuestaMarta DesSerializar_RespuestaMarta(const char* datosSerializados) {
	Macro_ImprimirParaDebug("DesSerializando Datos Respuesta Marta..\n");

	tipo_Datos_RespuestaMarta datosRespuestaMarta;
	uint32_t offset = 0;

	memcpy(datosRespuestaMarta.nodoIP, datosSerializados, sizeof(datosRespuestaMarta.nodoIP));
	offset = sizeof(datosRespuestaMarta.nodoIP);

	memcpy(datosRespuestaMarta.nodoPuerto, datosSerializados + offset, sizeof(datosRespuestaMarta.nodoPuerto));
	offset += sizeof(datosRespuestaMarta.nodoPuerto);

	memcpy(&datosRespuestaMarta.tipoDeOrden, datosSerializados + offset, sizeof(datosRespuestaMarta.tipoDeOrden));
	offset += sizeof(datosRespuestaMarta.tipoDeOrden);

	//Aca puede pasar algo raro con el Booleano, ya que no se bien cuanto ocupa en memoria porque stdbool no me lo dice
	memcpy(&datosRespuestaMarta.terminacionCorrecta, datosSerializados + offset,
			sizeof(datosRespuestaMarta.terminacionCorrecta));

	return datosRespuestaMarta;
}

char* Serializar_DatosNodo(const char nodoIP[LONGITUD_CHAR_IP], const char nodoPuerto[LONGITUD_CHAR_PUERTOS],
		uint32_t* tamanioSerializacion) {
	Macro_ImprimirParaDebug("Serializando Datos Nodo..\n");

	char* datosSerializados = malloc( LONGITUD_CHAR_IP + LONGITUD_CHAR_PUERTOS);
	uint32_t offset = 0;

	memcpy(datosSerializados, nodoIP, LONGITUD_CHAR_IP);
	offset = LONGITUD_CHAR_IP;

	memcpy(datosSerializados + offset, nodoPuerto, LONGITUD_CHAR_PUERTOS);

	*tamanioSerializacion = LONGITUD_CHAR_IP + LONGITUD_CHAR_PUERTOS;
	return datosSerializados;
}

void DesSerializar_DatosNodo(const char* datosSerializados, char** nodoIP, char** nodoPuerto) {
	Macro_ImprimirParaDebug("DesSerializando Datos Nodo..\n");

	*nodoIP = string_duplicate((char*) datosSerializados);
	*nodoPuerto = string_duplicate((char*) datosSerializados + LONGITUD_CHAR_IP);

	return;
}

char* Serializar_DatosMapping(const uint32_t numeroBloquePorTrabajar, const uint32_t tamanioBloquePorTrabajar,
		const char* nombreArchivoResultado, uint32_t* tamanioSerializacion) {
	Macro_ImprimirParaDebug("Serializando Datos Mapping..\n");

	char* datosSerializados = malloc(
			sizeof(numeroBloquePorTrabajar) + sizeof(tamanioBloquePorTrabajar) + strlen(nombreArchivoResultado) + 1);
	uint32_t offset = 0;

	memcpy(datosSerializados, &numeroBloquePorTrabajar, sizeof(numeroBloquePorTrabajar));
	offset = sizeof(numeroBloquePorTrabajar);

	memcpy(datosSerializados + offset, &tamanioBloquePorTrabajar, sizeof(tamanioBloquePorTrabajar));
	offset += sizeof(tamanioBloquePorTrabajar);

	memcpy(datosSerializados + offset, nombreArchivoResultado, strlen(nombreArchivoResultado) + 1);

	*tamanioSerializacion = sizeof(numeroBloquePorTrabajar) + sizeof(tamanioBloquePorTrabajar)
			+ strlen(nombreArchivoResultado) + 1;

	return datosSerializados;
}

void DesSerializar_DatosMapping(const char* datosSerializados, uint32_t* numeroBloquePorTrabajar,
		uint32_t* tamanioBloquePorTrabajar, char** nombreArchivoResultado) {

	Macro_ImprimirParaDebug("DesSerializando Datos Mapping..\n");

	memcpy(numeroBloquePorTrabajar, datosSerializados, sizeof(uint32_t));
	uint32_t offset = sizeof(numeroBloquePorTrabajar);

	memcpy(tamanioBloquePorTrabajar, datosSerializados + offset, sizeof(uint32_t));
	offset += sizeof(tamanioBloquePorTrabajar);

	*nombreArchivoResultado = string_duplicate((char*) (datosSerializados + offset));
}

char* Serializar_DatosReduceConCombiner(const t_list* listaArchivos, const char* nombreArchivoResultado,
		uint32_t* tamanioSerializacion) {
	Macro_ImprimirParaDebug("Serializando Datos Reduce Con Combiner..\n");

	//Primero necesito serialiar la lista para saber cuanto ocupara de tamaño
	uint32_t tamanioSerializacionLista;
	char* listaSerializada = Serializar_listaArchivos(listaArchivos, &tamanioSerializacionLista);

	char* datosSerializados = malloc(strlen(nombreArchivoResultado) + 1 + tamanioSerializacionLista);

	//Primero Copio el archivo resultado, luego la lista serializada
	memcpy(datosSerializados, nombreArchivoResultado, strlen(nombreArchivoResultado) + 1);
	uint32_t offset = strlen(nombreArchivoResultado) + 1;

	memcpy(datosSerializados + offset, listaSerializada, tamanioSerializacionLista);

	free(listaSerializada);

	*tamanioSerializacion = strlen(nombreArchivoResultado) + 1 + tamanioSerializacionLista;
	return datosSerializados;
}

void DesSerializar_DatosReduceConCombiner(const char* datosSerializados, t_list** listaArchivos,
		char** nombreArchivoResultado) {
	Macro_ImprimirParaDebug("DesSerializando Datos Reduce Con Combiner..\n");

	//Primero Extraigo el Archivo Resultado
	*nombreArchivoResultado = string_duplicate((char*) datosSerializados);

	Macro_ImprimirParaDebug("nombreArchivoResultado:'%s'\n", *nombreArchivoResultado);

	//Ahora des-Serializo la Lista

	*listaArchivos = DesSerializar_listaArchivos((char*) (datosSerializados + strlen(*nombreArchivoResultado) + 1));
	Macro_ImprimirParaDebug("Tamanio Lista Archivos:'%d'\n", list_size(*listaArchivos));

	return;
}

tipo_nodoExterno* CrearNodoExterno(char nodoIP[LONGITUD_CHAR_IP], char nodoPuerto[LONGITUD_CHAR_PUERTOS],
		char nombreArchivo[FILE_LONGITUD_NOMBRE]) {

	tipo_nodoExterno* nodoPorArmar = malloc( TAMANIO_NODO_EXTERNO);

	strcpy(nodoPorArmar->nodoIP, nodoIP);
	strcpy(nodoPorArmar->nodoPuerto, nodoPuerto);
	strcpy(nodoPorArmar->nombreArchivo, nombreArchivo);

	return nodoPorArmar;
}

void destructor_elementoListaNodo(tipo_nodoExterno* nodoPorLiberar) {
	if (nodoPorLiberar != NULL) {
		free(nodoPorLiberar);
	}
}

char* Serializar_nodoExterno(const tipo_nodoExterno datosPorSerializar) {
	Macro_ImprimirParaDebug("Serializando Nodo Externo..\n");

	char* datosSerializados = malloc( TAMANIO_NODO_EXTERNO);
	uint32_t offset = 0;

	memcpy(datosSerializados, datosPorSerializar.nodoIP, sizeof(datosPorSerializar.nodoIP));
	offset = sizeof(datosPorSerializar.nodoIP);

	memcpy(datosSerializados + offset, datosPorSerializar.nodoPuerto, sizeof(datosPorSerializar.nodoPuerto));
	offset += sizeof(datosPorSerializar.nodoPuerto);

	memcpy(datosSerializados + offset, datosPorSerializar.nombreArchivo, sizeof(datosPorSerializar.nombreArchivo));

	return datosSerializados;
}

tipo_nodoExterno* DesSerializar_nodoExterno(const char* datosSerializados) {
	Macro_ImprimirParaDebug("DesSerializando Nodo Externo..\n");

	tipo_nodoExterno* nodoExterno = malloc( TAMANIO_NODO_EXTERNO);
	uint32_t offset = 0;

	memcpy(nodoExterno->nodoIP, datosSerializados, sizeof(nodoExterno->nodoIP));
	offset = sizeof(nodoExterno->nodoIP);

	memcpy(&nodoExterno->nodoPuerto, datosSerializados + offset, sizeof(nodoExterno->nodoPuerto));
	offset += sizeof(nodoExterno->nodoPuerto);

	memcpy(nodoExterno->nombreArchivo, datosSerializados + offset, sizeof(nodoExterno->nombreArchivo));

	return nodoExterno;
}

char* Serializar_ListaNodoExterno(const t_list* listaPorSerializar, uint32_t* tamanioSerializacion) {
	Macro_ImprimirParaDebug("Serializando Lista de Nodos Externos..\n");
	//Lo primero que voy a hacer es obtener la cantidad de Elementos asi puedo hacer el malloc.
	uint32_t cantidadElementos = list_size((t_list*) listaPorSerializar);

	//Voy a serializar la cantidad de elementos + los elementos en si
	char* listaSerializada = malloc(sizeof(uint32_t) + (cantidadElementos * TAMANIO_NODO_EXTERNO));

	//Lo Segundo es serializar la cantidad de elementos de la lista, asi me facilita el DesSerializar
	uint32_t offset = 0;
	memcpy(listaSerializada, &cantidadElementos, sizeof(uint32_t));
	offset = sizeof(uint32_t);

	//Ahora voy a serializar cada Elemento de la Lista, para eso usare un FOR
	uint32_t elementoActual;
	for (elementoActual = 0; elementoActual < cantidadElementos; elementoActual++) {
		//NO estoy seguro si debe ir "<" o "<=", porque no se como devuelve bien el "list_size". Pareceria ser que va "<" por los tests de las commons (ver https://github.com/sisoputnfrba/so-commons-library/blob/master/tests/unit-tests/test_list.c#L116-L141 )

		//Obtengo el Elemento (Sin eliminarlo de la lista), lo serializo y aumento el offset
		tipo_nodoExterno* nodo = list_get((t_list*) listaPorSerializar, elementoActual);

		Macro_ImprimirParaDebug("Serializando Nodo Externo::>  NodoIp:'%s' nodoPuerto:'%s' NombreArchivo:'%s'\n",
				nodo->nodoIP, nodo->nodoPuerto, nodo->nombreArchivo);

		char* nodoSerializado = Serializar_nodoExterno(*nodo);

		memcpy(listaSerializada + offset, nodoSerializado, TAMANIO_NODO_EXTERNO);
		offset += TAMANIO_NODO_EXTERNO;

		free(nodoSerializado);
	}

	//Ya se serializaron todos los elementos de la Lista, asi que ahora digo cuanto es el tamanio de la serializacion
	*tamanioSerializacion = sizeof(uint32_t) + (cantidadElementos * TAMANIO_NODO_EXTERNO);

	return listaSerializada;
}

t_list* DesSerializar_ListaNodoExterno(const char* listaSerializada) {
	Macro_ImprimirParaDebug("DesSerializando Lista de Nodos Externos..\n");

	//Primero voy a obtener la cantidad de elementos que hay en la serializacion, asi lo uso en el FOR
	uint32_t cantidadElementos;

	uint32_t offset = 0;
	memcpy(&cantidadElementos, listaSerializada, sizeof(uint32_t));
	offset = sizeof(uint32_t);

	//Creo la Lista por Serializar
	t_list* listaPorDevolver = list_create();

	//Ahora voy a Des-Serializar cada Elemento de la Lista, para eso usare un FOR
	uint32_t elementoActual;
	for (elementoActual = 0; elementoActual < cantidadElementos; elementoActual++) {
		//NO estoy seguro si debe ir "<" o "<=", porque no se como devuelve bien el "list_size". Pareceria ser que va "<" por los tests de las commons (ver https://github.com/sisoputnfrba/so-commons-library/blob/master/tests/unit-tests/test_list.c#L116-L141 )

		//Obtengo el Nodo Serializado, lo des-serializo y agrego a la lista.
		char nodoSerializado[TAMANIO_NODO_EXTERNO];
		memcpy(nodoSerializado, listaSerializada + offset, TAMANIO_NODO_EXTERNO);
		offset += TAMANIO_NODO_EXTERNO;

		tipo_nodoExterno* nodo = DesSerializar_nodoExterno(nodoSerializado);

		Macro_ImprimirParaDebug("Nodo Externo Des-Serializado::>  NodoIp:'%s' nodoPuerto:'%s' NombreArchivo:'%s'\n",
				nodo->nodoIP, nodo->nodoPuerto, nodo->nombreArchivo);

		list_add(listaPorDevolver, nodo);
	}

	//Ya obtuve todos los elementos, asi que estoy listo para devolver la lista

	return listaPorDevolver;
}

char* Serializar_DatosReduceSinCombiner(const t_list* listaArchivosLocales, const t_list* listaNodosExternos,
		const char* nombreArchivoResultado, uint32_t* tamanioSerializacion) {
	Macro_ImprimirParaDebug("Serializando Datos Reduce Sin Combiner..\n");

	//Primero Realizo las Serializaciones Parciales y Al Final Junto Todo
	uint32_t tamanioSerializacionParcial;
	//NOTA: Aprovecho al otro Serializador de Reduce
	char* serializacionParcial = Serializar_DatosReduceConCombiner(listaArchivosLocales, nombreArchivoResultado,
			&tamanioSerializacionParcial);

	uint32_t listaNodosTamanio;
	char* listaNodosSerializada = Serializar_ListaNodoExterno(listaNodosExternos, &listaNodosTamanio);

	//Ahora si Junto Todo
	char* datosSerializados = malloc(tamanioSerializacionParcial + listaNodosTamanio);

	memcpy(datosSerializados, listaNodosSerializada, listaNodosTamanio); //Serializo este primero porque luego es mas facil al Des-Serializar
	memcpy(datosSerializados + listaNodosTamanio, serializacionParcial, tamanioSerializacionParcial);

	free(listaNodosSerializada);
	free(serializacionParcial);

	*tamanioSerializacion = tamanioSerializacionParcial + listaNodosTamanio;
	return datosSerializados;
}

void DesSerializar_DatosReduceSinCombiner(const char* datosSerializados, t_list** listaArchivos,
		t_list** listaNodosExternos, char** nombreArchivoResultado) {
	Macro_ImprimirParaDebug("DesSerializando Datos Reduce Sin Combiner..\n");

	//Primero Des-Serializo la Lista de Nodos
	*listaNodosExternos = DesSerializar_ListaNodoExterno(datosSerializados);

	//Ahora calculo donde Debere Pararme para Des-Serializar el Resto de los Datos, basandome en la cantidad de elementos y que son siempre del mismo tamaño
	uint32_t cantidadElementos = list_size(*listaNodosExternos);
	uint32_t offset = sizeof(uint32_t) + (cantidadElementos * TAMANIO_NODO_EXTERNO); //OJO, esto depepende del "Serializar_ListaNodoExterno", si se modifica esa funcion impacta aca.

	//Des-Serializo el Resto de los Datos
	//NOTA: Aprovecho al otro DesSerializador de Reduce
	DesSerializar_DatosReduceConCombiner(datosSerializados + offset, listaArchivos, nombreArchivoResultado);

	return;
}
