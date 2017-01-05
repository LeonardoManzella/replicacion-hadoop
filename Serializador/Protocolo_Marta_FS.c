#include "Protocolo_Marta_FS.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "commons/string.h"


//Constante Interna para agregar Expresividad
#define TAMANIO_COPIA sizeof(tipo_copia_bloqueNodo)

//Constante Interna para agregar Expresividad, sino es inentendible
#define TAMANIO_TRIADA (sizeof(int)+(3*(sizeof(tipo_copia_bloqueNodo))))

//Constante interna para agregar expresividad y unicidad. El valor es arbitrario
//FIXME Si se cortan los argumentos, borrar y cambiar todos los lados donde se referenciaba esto por RUTA_LONGITUD_MAXIMA
#define TAMANIO_RUTA    RUTA_LONGITUD_APROX	

//Constante interna para agregar expresividad
#define TAMANIO_NODO_RUTA_FINAL (TAMANIO_RUTA + NODO_LONGITUD_NOMBRE)

//Funcion Interna, recordar Hacer Free del Char (use Malloc para que sea Thread Safe)
static char* Serializar_unaCopia(const tipo_copia_bloqueNodo copiaPorSerializar);

static tipo_copia_bloqueNodo DesSerializar_unaCopia( const char* copiaSerializada);

// Funcion Depreacada Julian 2015-06-21
//Funcion interna para poder imprimir por pantalla las copias cuando estan Ya serializadas
//static void imprimir_copiaSerializada(const char* datosSerializadosConHeader);

// --- Funcion Deprecada Julian 2015-06-21
//Funcion Interna, recordar Hacer Free del Char (use Malloc para que sea Thread Safe)
//static char* TriadaCopias_Serializar(const tipo_triadaCopias* triadaPorSerializar);

static tipo_triadaCopias* TriadaCopias_DesSerializar(const char* triadaSerializada);






tipo_triadaCopias* TriadaCopias_crear(){
	//Hago el malloc para generar un nuevo tipo de dato
	// --- Julian 2015-06-21
	//tipo_triadaCopias* triadaPorCrear = malloc(sizeof(int) + (3*( sizeof(tipo_copia_bloqueNodo) ))) ;
	//int sizeTriada = sizeof(int) + sizeof(tipo_copia_bloqueNodo);
	int sizeTriada = sizeof(tipo_triadaCopias);
	tipo_triadaCopias* triadaPorCrear = malloc(sizeTriada);
	memset(triadaPorCrear, 0 , sizeTriada);

	//Inicializo a mano los structs, primero la cantidad de copias validas
	triadaPorCrear->cantidadCopiasValidas = 0;

	// --- Julian 2015-06-21
	//Ahora inicializo para cada struct copia
	//int copiaActual = 0;
	//for(copiaActual = 0; copiaActual<3; copiaActual++){
		//strcpy(triadaPorCrear->copia[copiaActual].nodoNombre,  "000000");
		//strcpy(triadaPorCrear->copia[copiaActual].nodoIP,  "00000");
		//strcpy(triadaPorCrear->copia[copiaActual].nodoPuerto,  "000000");
		//triadaPorCrear->copia[copiaActual].numeroBloqueDentroNodo = 0;
	//}

	return triadaPorCrear;
}

void TriadaCopias_destruir(tipo_triadaCopias* triadaPorDestruir) {
	//Parche porque el "list_destroy_and_destroy_elements" de las Commons anda mal..
	if (triadaPorDestruir != NULL) {
		if (triadaPorDestruir->copia != NULL) {
			free(triadaPorDestruir->copia);
		}
		free(triadaPorDestruir);
	}
	return;
}

void TriadaCopias_agregarCopia(tipo_triadaCopias* triadaDondeAgregar, const tipo_copia_bloqueNodo copiaPorAgregar, const int numeroDeCopia){
	//Conversion interna Para que desde afuera puedan usar numeros de 1 a 3 para agregar copias al vector (es mas expresivo).
	int numeroDeCopiaReal = numeroDeCopia-1;

	// --- Julian 2015-06-21
	//int sizeTriada = sizeof(tipo_triadaCopias);
	//triadaDondeAgregar = realloc(triadaDondeAgregar,sizeTriada);
	if (numeroDeCopiaReal != 0) {
		triadaDondeAgregar->copia = realloc(triadaDondeAgregar->copia, ((numeroDeCopia) * sizeof(tipo_copia_bloqueNodo)));
	} else {
		triadaDondeAgregar->copia = malloc(sizeof(tipo_copia_bloqueNodo));
	}
	//Duplico los valores pasados en la Copia que me pidieron
	memcpy(triadaDondeAgregar->copia[numeroDeCopiaReal].nodoNombre, copiaPorAgregar.nodoNombre, strlen(copiaPorAgregar.nodoNombre)+1 );

	memcpy(triadaDondeAgregar->copia[numeroDeCopiaReal].nodoIP, copiaPorAgregar.nodoIP, strlen(copiaPorAgregar.nodoIP)+1 );

	memcpy(triadaDondeAgregar->copia[numeroDeCopiaReal].nodoPuerto, copiaPorAgregar.nodoPuerto, strlen(copiaPorAgregar.nodoPuerto) +1 );

	triadaDondeAgregar->copia[numeroDeCopiaReal].numeroBloqueDentroNodo = copiaPorAgregar.numeroBloqueDentroNodo;

	// --- Julian 2015-06-21
	triadaDondeAgregar->copia[numeroDeCopiaReal].numeroBloqueDeArchivo = copiaPorAgregar.numeroBloqueDeArchivo;

	triadaDondeAgregar->copia[numeroDeCopiaReal].sizeArchivoDentroDelBloque = copiaPorAgregar.sizeArchivoDentroDelBloque;

}

tipo_copia_bloqueNodo TriadaCopias_obtenerCopia(const tipo_triadaCopias* triada, const int numeroDeCopia){
	//Conversion interna Para que desde afuera puedan usar numeros de 1 a 3 para agregar copias al vector (es mas expresivo).
	int numeroDeCopiaReal = numeroDeCopia-1;
	return triada->copia[numeroDeCopiaReal];
}


static char* Serializar_unaCopia(const tipo_copia_bloqueNodo copiaPorSerializar){
	Macro_ImprimirParaDebug("Serializando Copia..\n")

	//Debo usar Malloc para poder devolver el string
	char* copiaSerializada = malloc( TAMANIO_COPIA );
	//Inicializo la memoria, porque uso Static
	memset(copiaSerializada, 0, TAMANIO_COPIA);

	//Ahora SI serializo
	int offset = 0;
	//Si hay quilombo, restablecer (volver a agregar delante) los '&' alos primeros 3 memcpy
	memcpy(copiaSerializada, (copiaPorSerializar.nodoNombre), sizeof(copiaPorSerializar.nodoNombre) );
	offset = sizeof(copiaPorSerializar.nodoNombre);
	memcpy(copiaSerializada + offset, (copiaPorSerializar.nodoIP), sizeof(copiaPorSerializar.nodoIP) );
	offset += sizeof(copiaPorSerializar.nodoIP);
	memcpy(copiaSerializada + offset, (copiaPorSerializar.nodoPuerto), sizeof(copiaPorSerializar.nodoPuerto) );
	offset += sizeof(copiaPorSerializar.nodoPuerto);
	memcpy(copiaSerializada + offset, &(copiaPorSerializar.numeroBloqueDentroNodo), sizeof(copiaPorSerializar.numeroBloqueDentroNodo) );
	offset += sizeof(copiaPorSerializar.numeroBloqueDentroNodo);
	memcpy(copiaSerializada + offset, &(copiaPorSerializar.numeroBloqueDeArchivo), sizeof(copiaPorSerializar.numeroBloqueDeArchivo) );
	offset += sizeof(copiaPorSerializar.numeroBloqueDeArchivo);
	memcpy(copiaSerializada + offset, &(copiaPorSerializar.sizeArchivoDentroDelBloque), sizeof(copiaPorSerializar.sizeArchivoDentroDelBloque) );

	return copiaSerializada;
}

static tipo_copia_bloqueNodo DesSerializar_unaCopia(const char* copiaSerializada){
	//Aca no hace falta Verificar si empieza con \0, ya que eso es considerado Valido (Copia Vacia)

	//Imprimo para DEBUG lo que llego para Des-serializar, caracter por caracter
	//imprimir_copiaSerializada(copiaSerializada);

	//Variable por devolver
	tipo_copia_bloqueNodo copiaDesSerializada;

	//DesSerializo
	int offset = 0;
	//Si hay quilombo, restablecer (volver a agregar delante) los '&' alos primeros 3 memcpy
	memcpy((copiaDesSerializada.nodoNombre), copiaSerializada, sizeof(copiaDesSerializada.nodoNombre) );
	offset = sizeof(copiaDesSerializada.nodoNombre);
	memcpy((copiaDesSerializada.nodoIP), copiaSerializada + offset, sizeof(copiaDesSerializada.nodoIP) );
	offset += sizeof(copiaDesSerializada.nodoIP);
	memcpy((copiaDesSerializada.nodoPuerto), copiaSerializada + offset, sizeof(copiaDesSerializada.nodoPuerto) );
	offset += sizeof(copiaDesSerializada.nodoPuerto);
	memcpy(&(copiaDesSerializada.numeroBloqueDentroNodo), copiaSerializada + offset, sizeof(copiaDesSerializada.numeroBloqueDentroNodo) );
	offset += sizeof(copiaDesSerializada.numeroBloqueDentroNodo);
	memcpy(&(copiaDesSerializada.numeroBloqueDeArchivo), copiaSerializada + offset, sizeof(copiaDesSerializada.numeroBloqueDeArchivo) );
	offset += sizeof(copiaDesSerializada.numeroBloqueDeArchivo);
	memcpy(&(copiaDesSerializada.sizeArchivoDentroDelBloque), copiaSerializada + offset, sizeof(copiaDesSerializada.sizeArchivoDentroDelBloque) );

	return copiaDesSerializada;
}

/*
 * Funcion depreacada Julian 2015-06-21
static void imprimir_copiaSerializada(const char* copiaSerializada){
	//Voy a imprimir caracter por caracter
	int caracterActual;
	for (caracterActual = 0; caracterActual < TAMANIO_COPIA; caracterActual++) {
		//Como no puedo imprimir \0 porque Buguea el STDIN, imprimo el \0 como literal
		if (copiaSerializada[caracterActual] == '\0') {
			printf("\\0");
			fflush(stdout);
		} else {
			printf("%c" , (char) copiaSerializada[caracterActual]);
			fflush(stdout);
		}
	}
	//Para Separar entre copias
	printf("\n");
	fflush(stdout);
}
*/

/*
 * Funcion Deprecada por Julian 2015-06-21
static char* TriadaCopias_Serializar(const tipo_triadaCopias* triadaPorSerializar){
	Macro_ImprimirParaDebug("Serializando Triada..\n")

	//Debo usar Malloc para poder devolver el string
	char* triadaSerializada = malloc( TAMANIO_TRIADA );
	//Inicializo la memoria, porque uso Static
	memset(triadaSerializada, 0, TAMANIO_TRIADA);

	//Primero Serializo la cantidad De Copias Validas
	int offset = 0;
	memcpy(triadaSerializada, &(triadaPorSerializar->cantidadCopiasValidas), sizeof(int));
	offset = sizeof(int);

	//Ahora serializo las 3 copias con un FOR
	int copiaActual;
	for (copiaActual = 0; copiaActual < 3; copiaActual++) {
		char* copiaSerializada = Serializar_unaCopia(triadaPorSerializar->copia[copiaActual]);
		memcpy(triadaSerializada + offset , copiaSerializada , TAMANIO_COPIA);
		offset += TAMANIO_COPIA;
		free(copiaSerializada);
	}

	return triadaSerializada;
}
*/

static tipo_triadaCopias* TriadaCopias_DesSerializar(const char* triadaSerializada){
	//Imprimo para DEBUG lo que llego para Des-Serializar, en realidad solo imprimo un separador entre Copias (unos espacios), ya que el Des-Serializador de Copias se encarga de imprimir
	Macro_ImprimirParaDebug("   ");
	fflush(stdout);


	//Reviso que el char* no empieze con \0, sino es que se serializo o envio mal (lo cual seria raro)
	if(string_is_empty((char*)triadaSerializada)){
		Macro_ImprimirParaDebug("Error al des-serializar una Triada, fijate que el char* de esta triada empieza con un \\0 y no deberia");
		return NULL;
	}


	tipo_triadaCopias* triadaPorDevovler = TriadaCopias_crear();

	//Primero DesSerializo la cantidad De Copias Validas
	int offset = 0;
	memcpy(&(triadaPorDevovler->cantidadCopiasValidas) , triadaSerializada , sizeof(int));
	offset = sizeof(int);

	//Ahora DesSerializo las 3 copias con un FOR
	int copiaActual;
	for (copiaActual = 0; copiaActual < triadaPorDevovler->cantidadCopiasValidas; copiaActual++) {
		tipo_copia_bloqueNodo copiaDesSerializada = DesSerializar_unaCopia(triadaSerializada+ offset);
		offset += TAMANIO_COPIA;

		TriadaCopias_agregarCopia(triadaPorDevovler , copiaDesSerializada, copiaActual+1);
	}
	return triadaPorDevovler;
}

char* Serializar_listaCopiasBloqueNodo(const t_list* listaPorSerializar, uint32_t* tamanioSerializacion ){
	Macro_ImprimirParaDebug("Serializando Lista..\n")
	//Lo primero que voy a hacer es obtener la cantidad de Elementos asi puedo hacer el malloc.
	uint32_t cantidadElementos = list_size( (t_list*)listaPorSerializar);


	//Voy a serializar la cantidad de elementos + los elementos en si
	// --- Julian 2015-06-21
	//char* listaSerializada = malloc(sizeof(uint32_t)+(cantidadElementos* TAMANIO_TRIADA));
	char* listaSerializada = malloc(sizeof(uint32_t));

	//Lo Segundo es serializar la cantidad de elementos de la lista, asi me facilita el DesSerializar
	uint32_t offset=0;
	memcpy(listaSerializada, &cantidadElementos, sizeof(uint32_t));
	offset = sizeof(uint32_t);


	//Ahora voy a serializar cada Elemento de la Lista, para eso usare un FOR
	uint32_t elementoActual;
	for (elementoActual = 0; elementoActual < cantidadElementos; elementoActual++) {
		//NO estoy seguro si debe ir "<" o "<=", porque no se como devuelve bien el "list_size". Pareceria ser que va "<" por los tests de las commons (ver https://github.com/sisoputnfrba/so-commons-library/blob/master/tests/unit-tests/test_list.c#L116-L141 )

		// Julian 2015-06-21
		//tipo_triadaCopias* triada = TriadaCopias_crear();

		//Obtengo el Elemento (Sin eliminarlo de la lista), lo serializo y aumento el offset
		// Julian 2015-06-21
		//triada = list_get( (t_list*)listaPorSerializar, elementoActual);

		//Debo usar Malloc para poder devolver el string
		char* triadaSerializada = malloc( sizeof(int) );
		//Inicializo la memoria, porque uso Static
		memset(triadaSerializada, 0, sizeof(int));

		int copiaActualDatosBloque;
		tipo_triadaCopias* datosDeLista = list_get( (t_list*)listaPorSerializar, elementoActual);
		int cantidadCopias;
		int offsetLista = 0;
		cantidadCopias = datosDeLista->cantidadCopiasValidas;

		memcpy(triadaSerializada, &cantidadCopias, sizeof(int));
		offsetLista = sizeof(int);

		//	Es menor porque hay una variacion entre el array y el numero real de copias
		for (copiaActualDatosBloque = 0; copiaActualDatosBloque < cantidadCopias; copiaActualDatosBloque++) {
			char* copiaSerializada = Serializar_unaCopia(datosDeLista->copia[copiaActualDatosBloque]);

			triadaSerializada = realloc(triadaSerializada,offsetLista + TAMANIO_COPIA);
			memcpy(triadaSerializada + offsetLista , copiaSerializada , TAMANIO_COPIA);
			offsetLista += TAMANIO_COPIA;
		
			free(copiaSerializada);
		}

		// Julian 2015-06-21
		//char* triadaSerializada = TriadaCopias_Serializar(triada);

		listaSerializada = realloc(listaSerializada, offset + offsetLista);
		memcpy(listaSerializada + offset, triadaSerializada, offsetLista);
		offset += offsetLista;

		free(triadaSerializada);
	}

	//Ya se serializaron todos los elementos de la Lista, asi que ahora digo cuanto es el tamanio de la serializacion
	*tamanioSerializacion = offset;

	return listaSerializada;
}

t_list* DesSerializar_listaCopiasBloqueNodo(const char* listaSerializada) {
	//Primero voy a obtener la cantidad de elementos que hay en la serializacion, asi lo uso en el FOR
	uint32_t cantidadElementos;

	uint32_t offset = 0;
	memcpy(&cantidadElementos , listaSerializada , sizeof(uint32_t));
	offset = sizeof(uint32_t);

	//Imprimo para DEBUG lo que llego para Des-Serializar, solo hace falta imprimir el numero que encabeza a las triadas, lo demas se imprime en el des-serializar de triadas
	Macro_ImprimirParaDebug("El Char* que se va a Des-Serializar es: %s\n",listaSerializada);
	fflush(stdout);
	Macro_ImprimirParaDebug("<%d Bytes que representan al Numero %d> \n", sizeof(uint32_t), cantidadElementos);
	fflush(stdout);


	//Reviso que el char* no empieze con \0, sino es que se serializo o envio mal (lo cual seria raro)
	if (string_is_empty((char*)(listaSerializada + offset))) {
		Macro_ImprimirParaDebug("Error al des-serializar la lista, fijate que el char* empieza con un \\0 y no deberia");
		return NULL;
	}

	//Creo la Lista por Serializar
	t_list* listaPorDevolver = list_create();

	//Ahora voy a Des-Serializar cada Elemento de la Lista, para eso usare un FOR
	uint32_t elementoActual;
	for (elementoActual = 0; elementoActual < cantidadElementos; elementoActual++) {
		//NO estoy seguro si debe ir "<" o "<=", porque no se como devuelve bien el "list_size". Pareceria ser que va "<" por los tests de las commons (ver https://github.com/sisoputnfrba/so-commons-library/blob/master/tests/unit-tests/test_list.c#L116-L141 )

		// Julian 2015-06-21
		int cantidadCopiasValidas;
		int offsetTriadaActual;
		memcpy(&cantidadCopiasValidas, listaSerializada + offset, sizeof(int));
		offsetTriadaActual = sizeof(int) + (cantidadCopiasValidas * TAMANIO_COPIA);

		char triadaSerializada[offsetTriadaActual];

		memcpy(triadaSerializada, listaSerializada + offset, offsetTriadaActual);
		offset += offsetTriadaActual;

		//Obtengo la Triada Serializada, la des-serializo y agrego a la lista.
		//char triadaSerializada[TAMANIO_TRIADA];
		//memcpy(triadaSerializada, listaSerializada + offset, TAMANIO_TRIADA);
		//offset += TAMANIO_TRIADA;

		tipo_triadaCopias* triada = TriadaCopias_DesSerializar(triadaSerializada);
		if(triada==NULL){
			Macro_ImprimirParaDebug("Llego Cualquier Cosa de Triada \n");
			return NULL;
		}

		list_add(listaPorDevolver, triada);
	}

	//Ya obtuve todos los elementos, asi que estoy listo para devolver la lista

	return listaPorDevolver;
}

//NO OLVIDAR HACER FREE AL CHAR* DEVUELTO UNA VEZ UTILIZADO, YA QUE SE USO MALLOC
char* serializar_rutaFinalYNodoContenedor(char* rutaFinal, char* nombreNodoContenedor, uint32_t* tamanioSerializacion){
	//inicio el char* del resultado final, este es el maximo nombre de nodo + el maximo tamano de ruta
	char* copiaSerializada = malloc(TAMANIO_NODO_RUTA_FINAL);

	//Inicializo la memoria
	memset(copiaSerializada, 0, TAMANIO_NODO_RUTA_FINAL);

	//Copio el string hasta el largo del string de nombre de nodo
	memcpy(copiaSerializada, nombreNodoContenedor, strlen(nombreNodoContenedor)+1 );

	//copio el string de ruta, dejando como offset el tamano maximo de nombre de nodo dado por NODO_LONGITUD_NOMBRE de nododb.h
	memcpy(copiaSerializada + NODO_LONGITUD_NOMBRE, rutaFinal, strlen(rutaFinal)+1);

	//Retorno tamano de la serializacion, para poder empaquetarse y enviarse (sino no tenemos manera de saber la longitud porque con un strlen nos devolveria hasta el primer \0)
	*tamanioSerializacion = TAMANIO_NODO_RUTA_FINAL;

	return copiaSerializada;
}

//NO OLVIDAR HACER FREE EN LOS DOS CHAR* PASADOS UNA VEZ UTILIZADOS
void deserializar_rutaFinalYNodoContenedor(const char* datosSerializados, char** rutaFinal, char** nombreNodoContenedor){
	*nombreNodoContenedor = malloc(NODO_LONGITUD_NOMBRE + 1);
	*rutaFinal = malloc(TAMANIO_RUTA + 1);

	memcpy( *nombreNodoContenedor, datosSerializados, NODO_LONGITUD_NOMBRE);
	memcpy( *rutaFinal, (datosSerializados + NODO_LONGITUD_NOMBRE), TAMANIO_RUTA );

	//Me aseguro de devolver un string
	(*nombreNodoContenedor)[NODO_LONGITUD_NOMBRE] = '\0';
	(*rutaFinal)[TAMANIO_RUTA] = '\0';
}
