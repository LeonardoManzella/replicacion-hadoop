#include "../headers/Biblioteca_Bloques.h"

#include "../../Biblioteca_Comun/Biblioteca_Comun.h"

	#include <stdio.h>
	#include <stdlib.h> //Para "Malloc" y "Free"
	#include <sys/mman.h> //Para "mmap"
	#include <sys/types.h> //Para Portabilidad
	#include <string.h>
	#include <sys/stat.h> //Para el Struct y Funcion "stat"
	#include <pthread.h>	//Para Mutex
	#include <time.h> 	//Para Mutex
	#include <unistd.h> //Para "sysconf"
	#include <fcntl.h> // Para open


//Inicializo un Mutex para el Stat. Le tuve que poner Mutex porque nadie me asegura que Stat sea Thread Safe
static pthread_mutex_t Mutex_Interno_Stat  = PTHREAD_MUTEX_INITIALIZER;


tipo_bloque* Bloques_crear_nuevo(){
	tipo_bloque* tNuevoBloque = malloc( sizeof(tipo_bloque) );

	//Inicializo el Bloque con Ceros, asi no hay basura dentro del Bloque.
	memset( tNuevoBloque->contenidoBloque , '0', sizeof(char)*TAMANIO_BLOQUE );
	
	//Pongo al final del contenido del bloque el \0 (caracter de fin de string)
	tNuevoBloque->contenidoBloque[TAMANIO_BLOQUE] = '\0';

	//Inicializo a 0 el tamaño del Bloque.
	tNuevoBloque->tamanioBloque = 0;

	return tNuevoBloque;
}




void Bloques_destruir( tipo_bloque* tBloque ){
	free( tBloque );
}

size_t Bloques_obtener_tamanio_bloque_en_memoria( char **punteroBloqueMemoria ){

	//Busco para atras hasta el primer \n, asi obtengo donde se termina realmente los datos dentro de lo que lei.
	size_t lPosicionLecturaBarraN = 0;
	for( lPosicionLecturaBarraN = TAMANIO_BLOQUE; lPosicionLecturaBarraN > 0; lPosicionLecturaBarraN-- ){
		//Para Debug: Imprimo que estoy leyendo
		//printf( "En posicion %i lei %c.\n", (int) (lPosicionLecturaMemoria + lOffset), ((char*)punteroBloqueMemoria)[lPosicionLecturaMemoria]  );
		//printf(" - Bloques_obtener_desde_archivo_texto_nodo: dentro del FOR. pos: %ud\n", lPosicionLecturaBarraN);
		//Cuando encuentro la posicion del \n paro de buscar.
		if ((*punteroBloqueMemoria)[lPosicionLecturaBarraN - 1] == '\n') {
			break;
		}
	}

	printf(" - Se encontro el final del bloque\n");

	if ( lPosicionLecturaBarraN == 0 ){
		printf( "No hay ningun \"\\n\" en el Bloque de la posicion. Puede ser que el archivo este Vacio. \n Muestro el bloque que lei:  \n" );

		return 0;
	}


	return lPosicionLecturaBarraN;
}



tipo_bloque* Bloques_obtener_desde_archivo_texto_nodo( const char *sRutaArchivo, uint32_t lOffset ){
	//Variable para ver errores
	int iNumeroError = 0;

	//TODO sacar estos Printf en cuanto ande, porque asi anda MUCHO mas Rapido
	printf(" - Bloques_obtener_desde_archivo_texto_nodo: Me llamaron\n");


	//Abro el archivo para solo lectura e imprimo si hay errores.
		FILE *archivoTexto = fopen( sRutaArchivo, "r" );
		if( archivoTexto==NULL ){
			Macro_ImprimirParaDebug( "Problema al abrir el Archivo de Texto \n " );
			Macro_ImprimirParaDebug("Error Detectado:" ANSI_COLOR_RED "%s" ANSI_COLOR_RESET "\n", Macro_Obtener_Errno());

			return NULL;
		}

		printf(" - Bloques_obtener_desde_archivo_texto_nodo: Antes de Mmap\n");

	//Del archivo abierto, obtengo su File Descriptor para darselo a mmap.
		int iDescriptorArchivo = fileno( archivoTexto );
		errno=0;
		char *punteroBloqueMemoria =  mmap( NULL, TAMANIO_BLOQUE, PROT_READ, MAP_PRIVATE, iDescriptorArchivo, lOffset );
	//Veo si hay error con el mmap.
		if( punteroBloqueMemoria==(((void *) -1)) || (errno!=0) ){
			Macro_ImprimirParaDebug( "Problema al realizar el Mappeo del archivo \n" );
			Macro_ImprimirParaDebug("Error Detectado:" ANSI_COLOR_RED "%s" ANSI_COLOR_RESET "\n", Macro_Obtener_Errno());

			return NULL;
		}

	printf(" - Bloques_obtener_desde_archivo_texto_nodo: Llego hasta antes FOR\n");

	//Busco para atras hasta el primer \n, asi obtengo donde se termina realmente los datos dentro de lo que lei.
	uint32_t lPosicionLecturaBarraN = 0;
	for( lPosicionLecturaBarraN = TAMANIO_BLOQUE; lPosicionLecturaBarraN > 0; lPosicionLecturaBarraN-- ){
		//Para Debug: Imprimo que estoy leyendo
		//printf( "En posicion %i lei %c.\n", (int) (lPosicionLecturaMemoria + lOffset), ((char*)punteroBloqueMemoria)[lPosicionLecturaMemoria]  );
		//printf(" - Bloques_obtener_desde_archivo_texto_nodo: dentro del FOR. pos: %ud\n", lPosicionLecturaBarraN);
		//Cuando encuentro la posicion del \n paro de buscar.
		if ((punteroBloqueMemoria)[lPosicionLecturaBarraN - 1] == '\n') {
			break;
		}
	}

	printf(" - Bloques_obtener_desde_archivo_texto_nodo: Llego despues FOR\n");

	if ( lPosicionLecturaBarraN == 0 ){
		printf( "No hay ningun \"\\n\" en el Bloque de la posicion. Puede ser que el archivo este Vacio. \n Muestro el bloque que lei:  \n" );
		
		return NULL;
	}
	
	printf(" - Bloques_obtener_desde_archivo_texto_nodo: Llego hasta antes del close \n");

	iNumeroError = fclose( archivoTexto );
	if( iNumeroError!=0 ){
		Macro_ImprimirParaDebug( "Hubo un Error al cerrar el archivo  \n" );
		Macro_ImprimirParaDebug("Error Detectado:" ANSI_COLOR_RED "%s" ANSI_COLOR_RESET "\n", Macro_Obtener_Errno());

		return NULL;
	}

	printf(" - Bloques_obtener_desde_archivo_texto_nodo: Llego hasta antes crearbloque\n");

	//Esto es para Evitar Memory Leaks por no hacer 'munmap'. Ya se que hace que por cada lectura ocupen 40mb en memoria, pero es solo durante un pequeño momento y nos evitamos problemas de memory leaks siempre que se le haga un free a lo que devuelve mi funcion.
		tipo_bloque* bloquePorDevolver = Bloques_crear_nuevo();

		/*
		Copio a "bloquePorDevolver" solo la cantidad de bytes hasta el \n que encontre (desde la posicion donde se habia encontrado el ultimo bloque anterior), No la totalidad de lo que leyo mmap.
		Debo restar lLoQueMePaseDelBloqueAnterior en el 3er argumento porque no lo usa como Posicion, sino que lo usa como cantidad de Bytes a Leer desde el 2do argumento.
		*/

		printf(" - Bloques_obtener_desde_archivo_texto_nodo: Llego hasta antes mmcpy\n");

		memcpy( bloquePorDevolver->contenidoBloque , punteroBloqueMemoria, lPosicionLecturaBarraN+1);

		printf(" - Bloques_obtener_desde_archivo_texto_nodo: Llego hasta antes de munmap\n");

		iNumeroError = munmap( punteroBloqueMemoria,  TAMANIO_BLOQUE);
		if( iNumeroError!=0 ){
			Macro_ImprimirParaDebug( "Hubo un Error al hacer el munmap final \n" );
			Macro_ImprimirParaDebug("Error Detectado:" ANSI_COLOR_RED "%s" ANSI_COLOR_RESET "\n", Macro_Obtener_Errno());

			return NULL;
		}

		//Cargo la Longitud del Bloque
		bloquePorDevolver->tamanioBloque = lPosicionLecturaBarraN+1;

		printf(" - Bloques_obtener_desde_archivo_texto_nodo: pos:%ul\n", lPosicionLecturaBarraN);

	return bloquePorDevolver;
}

tipo_bloque* Bloques_obtener_desde_archivo_texto( const char *sRutaArchivo, uint32_t *piPosicionLecturaArchivo ){
	//Variable para ver errores
	int iNumeroError = 0;

	//Abro el archivo para solo lectura e imprimo si hay errores.
		FILE *archivoTexto = fopen( sRutaArchivo, "r" );
		if( archivoTexto==NULL ){
			Macro_ImprimirParaDebug( "Problema al abrir el Archivo de Texto \n " );
			Macro_ImprimirParaDebug("Error Detectado:" ANSI_COLOR_RED "%s" ANSI_COLOR_RESET "\n", Macro_Obtener_Errno());

			return NULL;
		}

	//Del archivo abierto, obtengo su File Descriptor para darselo a mmap.
		int iDescriptorArchivo = fileno( archivoTexto );

	/*
	Aca esta la MAGIA de mi Algoritmo.
	La cosa es modificar las variables lOffset y lLength.
	Yo con la ultima posicionLecturaArchivo lo que hago es volver para atras hasta la pagina mas proxima, ahi se posiciona el lOffset.
	En el lLength aparte del TAMANIO_BLOQUE, yo leo lLoQueMePaseDelBloqueAnterior, que como siempre sera menor a 1 Pagina, lo que realmente termina haciendo el mmap es leer 1 pagina extra.
	Luego al final, yo solo devuelvo el bloque pero a partir de la ultima posicionLecturaArchivo, que es la posicion del ultimo bloque leido.
	NOTA: Si estas leyendo esto, mi consejo es que mejor me preguntes ami como funciona (soy Leo) porque se que es re complicado de entender.
	*/
		//Obtengo el tamaño (en bytes) de las Paginas del Sistema
		uint32_t lPageSize = sysconf(_SC_PAGESIZE);
		//Desde que Numero de Pagina leo.
		uint32_t lOffset = (*piPosicionLecturaArchivo / lPageSize) * lPageSize;
		uint32_t lLoQueMePaseDelBloqueAnterior = (*piPosicionLecturaArchivo % lPageSize);
		//Cuanto leo desde el "lOffset".
		uint32_t lLength = TAMANIO_BLOQUE + lLoQueMePaseDelBloqueAnterior;

	//En caso que se quiera pasar del final del archivo, le digo que solo lea hasta el final del archivo (cambio el lLength).
		uint32_t lTamanioArchivo = Bloques_obtener_tamanio_archivo( sRutaArchivo );
		if( (lOffset+lLength)>=lTamanioArchivo ){
			lLength = lTamanioArchivo - lOffset;
		} else if( lTamanioArchivo==-1 ){
			//Hubo un error y no se pudo determinar el tamaño del archivo. No hace falta imprimir error porque la funcion "Bloques_obtener_tamanio_archivo" ya se encarga de hacerlo.
			return NULL;
		}

	//Ahora me encargo de Leer con el Mmap
		char *punteroBloqueMemoria =  mmap( NULL, lLength, PROT_READ, MAP_PRIVATE, iDescriptorArchivo, lOffset );
		//Veo si hay error con el mmap.
		if( punteroBloqueMemoria==((void *) -1) ){
			Macro_ImprimirParaDebug( "Problema al realizar el Mappeo del archivo \n" );
			Macro_ImprimirParaDebug("Error Detectado:" ANSI_COLOR_RED "%s" ANSI_COLOR_RESET "\n", Macro_Obtener_Errno());

			return NULL;
		}

	//Busco para atras hasta el primer \n, asi obtengo donde se termina realmente los datos dentro de lo que lei.
	uint32_t lPosicionLecturaBarraN = 0;
	for( lPosicionLecturaBarraN = (lLength-1); lPosicionLecturaBarraN > 0; lPosicionLecturaBarraN-- ){
			//Para Debug: Imprimo que estoy leyendo
			//printf( "En posicion %i lei %c.\n", (int) (lPosicionLecturaMemoria + lOffset), ((char*)punteroBloqueMemoria)[lPosicionLecturaMemoria]  );

			//Cuando encuentro la posicion del \n paro de buscar.
			if( (punteroBloqueMemoria)[lPosicionLecturaBarraN] == '\n' ){
				break;
			}
		}

	if ( lPosicionLecturaBarraN == 0 ){
		printf( "No hay ningun \"\\n\" en el Bloque de la posicion %ld. Puede ser que el archivo este Vacio. \n Muestro el bloque que lei: %s \n", (long int)*piPosicionLecturaArchivo,punteroBloqueMemoria );
		
		return NULL;

	//Si la Posicion del Barra N (+1 Porque es Justo la Posicion Anterior al BarraN) coincide con lo que me pase del ultimo Bloque tambien significa que no hay ningun Barra N en el Bloque Actual (Tomo el Barra N del Bloque Anterior, lo cual estaria Mal)
	} else if ( (lPosicionLecturaBarraN + 1) == lLoQueMePaseDelBloqueAnterior) {
		printf("No hay ningun \"\\n\" en el Bloque de la posicion %ld. Puede ser que el archivo este Vacio. \n Muestro el bloque que lei: %s \n" , (long int)*piPosicionLecturaArchivo,punteroBloqueMemoria);

		return NULL;
	}

	//Como ya encontro justo la pocision del \n, le hago +1 para que lo agarre tambien el memcpy, si no lo hiciera, No me copia el \n por cada bloque.
	 lPosicionLecturaBarraN++;


	//Ciero el archivo y veo si hay errores.
	iNumeroError = fclose( archivoTexto );
	if( iNumeroError!=0 ){
		Macro_ImprimirParaDebug( "Hubo un Error al cerrar el archivo  \n" );
		Macro_ImprimirParaDebug("Error Detectado:" ANSI_COLOR_RED "%s" ANSI_COLOR_RESET "\n", Macro_Obtener_Errno());

		return NULL;
	}

	//Esto es para Evitar Memory Leaks por no hacer 'munmap'. Ya se que hace que por cada lectura ocupen 40mb en memoria, pero es solo durante un pequeño momento y nos evitamos problemas de memory leaks siempre que se le haga un free a lo que devuelve mi funcion.
		tipo_bloque* bloquePorDevolver = Bloques_crear_nuevo();

		/*
		Copio a "bloquePorDevolver" solo la cantidad de bytes hasta el \n que encontre (desde la posicion donde se habia encontrado el ultimo bloque anterior), No la totalidad de lo que leyo mmap.
		Debo restar lLoQueMePaseDelBloqueAnterior en el 3er argumento porque no lo usa como Posicion, sino que lo usa como cantidad de Bytes a Leer desde el 2do argumento.
		*/
		memcpy( bloquePorDevolver->contenidoBloque , punteroBloqueMemoria + lLoQueMePaseDelBloqueAnterior, lPosicionLecturaBarraN - lLoQueMePaseDelBloqueAnterior );


		//Hago el munmap final.
		iNumeroError = munmap( punteroBloqueMemoria,  lPosicionLecturaBarraN);
		if( iNumeroError!=0 ){
			Macro_ImprimirParaDebug( "Hubo un Error al hacer el munmap final \n" );
			Macro_ImprimirParaDebug("Error Detectado:" ANSI_COLOR_RED "%s" ANSI_COLOR_RESET "\n", Macro_Obtener_Errno());

			return NULL;
		}

		//Cargo la Longitud del Bloque
		bloquePorDevolver->tamanioBloque = lPosicionLecturaBarraN - lLoQueMePaseDelBloqueAnterior;


	//Cargo en la vieja piPosicionLecturaArchivo la nueva posicion (despues de leido el bloque), para devolverla por referencia.
	//La nueva posicion es (partiendo del lOffset) la lPosicionLecturaBarraN (donde encontro el \n en mi memoria mapeada).
	*piPosicionLecturaArchivo = lPosicionLecturaBarraN + lOffset;

	return bloquePorDevolver;
}




uint32_t Bloques_obtener_tamanio_archivo( const char* sRutaArchivo ){
	uint32_t numeroError = 0;
	int numeroErrorStat = 0;

	//Uso stat para obtener el tamaño del archivo, tengo que usar una estructura auxiliar "sBufferAuxiliar"
	struct stat sBufferAuxiliar;

	//Le tuve que poner Mutex porque nadie me asegura que Stat sea Thread Safe
	numeroError = pthread_mutex_lock(&Mutex_Interno_Stat);
	Macro_Check_And_Handle_Error(numeroError != 0 ,"Problema al Pedir Lock Mutex. Numero Error:%d" , numeroError);

	//Obtengo el tamaño
	numeroErrorStat = stat(sRutaArchivo , &sBufferAuxiliar);

	numeroError = pthread_mutex_unlock(&Mutex_Interno_Stat);
	Macro_Check_And_Handle_Error(numeroError != 0 ,"Problema al Hacer UnLock Mutex. Numero Error:%d" , numeroError);

	Macro_Check_And_Handle_Error(numeroErrorStat == -1 , "Hubo un error y no se pudo determinar el tamaño del archivo. Error en el Stat");

	Macro_Check_And_Handle_Error(sBufferAuxiliar.st_size <= 0 , "Tamanio Invalido del Archivo, Tamanio: %ld" , (long int)sBufferAuxiliar.st_size);

	return sBufferAuxiliar.st_size;


Error_Handler:
	return -1;
}

uint32_t Bloques_obtener_cantidad_bloques_archivo( const char* sRutaArchivo  ){
	//Esta Funcion NO falla al Calcular la Cantidad de Bloques. La dejo y arregle todos los Problemas donde Deberia Explotar y no lo hacia.
	//FIXME Mentira Arregle Todo, Falta 1 vez que deberia explotar, pero por como armaste la Logica no tengo manera de Ponerlo, fijate de ponerlo vos julian, te puse un comentario.
	uint32_t	numeroError = 0;
	uint32_t 	cantidadBloques = -1;
	off_t		offset_Archivo = 0;
	uint32_t 	tamanioArchivo = -1;
	bool 		bEncontradoBarraN;
	uint32_t 	posArchivo, posInicioBloque;
	uint32_t 	iPosString;
	uint32_t 	iSizeLectura = sysconf(_SC_PAGESIZE);
	char 		*sDatosLeidos = NULL;
	char 		testFile;
	//FILE* 		pArchivo = NULL;
	int 		pArchivo=0;

	tamanioArchivo = Bloques_obtener_tamanio_archivo(sRutaArchivo);
	Macro_Check_And_Handle_Error(tamanioArchivo == -1 , "Hubo un Problema al Obtener Tamaño del Archivo");

	sDatosLeidos = malloc(sizeof(char)*iSizeLectura);
	pArchivo = open(sRutaArchivo, O_RDONLY | O_NONBLOCK);
	Macro_Check_And_Handle_Error(pArchivo <= 0 , "Hubo un Problema al Abrir el Archivo");

	// Si el archivo es menor a un bloque, me fijo nada mas que tenga el \n Final
	if (tamanioArchivo <= TAMANIO_BLOQUE) {
		cantidadBloques = 1;
		offset_Archivo = tamanioArchivo;
		numeroError = lseek(pArchivo, offset_Archivo - 1, SEEK_SET);		//-1 Porque debo Leer el ultimo Char
		Macro_Check_And_Handle_Error(numeroError == -1 , "Hubo un Problema Al Posicionarme para ver si hay un BarraN Final");

#pragma GCC diagnostic ignored "-Wunused-result"
		numeroError = read(pArchivo, sDatosLeidos, 1);		//1 Porque debo Leer Nada mas el ultimo Char
#pragma GCC diagnostic pop
		Macro_Check_And_Handle_Error(numeroError < 0 , "Problema al Hacer Fread");

		// FIXED : Esto no es tan valido, el \n podria estar antes del ultimo caracter
		//EL \n Si o si debe ir en el ultimo caracter o esta mal. Restriccion del TP
		Macro_Check_And_Handle_Error((*sDatosLeidos) != '\n' , "No hay un '\\n' al Final del Archivo. El Archivo No es Valido");

	} else {
		//Empiezo a Contar Bloques
		cantidadBloques = 0;
		// El bloque comienza con el inicio del archivo
		posInicioBloque = 0;

		//Mientras hayan Datos..
		while (read(pArchivo, &testFile, 1) > 0){
			//Macro_Check_And_Handle_Error(ferror(pArchivo)==true , "Problema al Hacer Fread");
			cantidadBloques++;
			bEncontradoBarraN = false;
			posArchivo = 0;
			offset_Archivo += TAMANIO_BLOQUE;

			if (offset_Archivo > tamanioArchivo) {
				// Estas ultimas lineas serian necesarias si quiero verificar que el ultimo pedazito de archivo tenga \n
				//offset_Archivo -= TAMANIO_BLOQUE;
				//offset_Archivo += (tamanioArchivo - offset_Archivo);
				// alternativa donde directamente leo el final del archivo
				offset_Archivo = tamanioArchivo;
				numeroError = lseek(pArchivo, offset_Archivo - 1, SEEK_SET);		//-1 Porque debo Leer el ultimo Char
				Macro_Check_And_Handle_Error(numeroError == -1 , "Hubo un Problema Al Posicionarme para ver si hay un BarraN Final");
#pragma GCC diagnostic ignored "-Wunused-result"
				numeroError = read(pArchivo, sDatosLeidos, 1);		//1 Porque debo Leer Nada mas el ultimo Char
#pragma GCC diagnostic pop
				Macro_Check_And_Handle_Error(numeroError < 0 , "Problema al Hacer Fread");
				Macro_Check_And_Handle_Error((*sDatosLeidos) != '\n' , "No hay un '\\n' al Final del Archivo. El Archivo No es Valido");
				break;
			}


			while (bEncontradoBarraN == false) {
				posArchivo++;
				if  ((offset_Archivo - ((iSizeLectura * posArchivo))) < posInicioBloque) {
					// Exploto porque paso el inicio del bloque
					goto Error_Handler;
				}
				numeroError = lseek(pArchivo, offset_Archivo - (iSizeLectura*posArchivo), SEEK_SET);
				Macro_Check_And_Handle_Error(numeroError == -1 , "Hubo un Problema Al Posicionarme para ver un Bloque");
#pragma GCC diagnostic ignored "-Wunused-result"
				numeroError = read(pArchivo, sDatosLeidos, iSizeLectura);
#pragma GCC diagnostic pop
				Macro_Check_And_Handle_Error(numeroError < 0, "Problema al Hacer Fread");

				//Recorro Para Atras Buscando el BarraN
				for(iPosString = (iSizeLectura-1); iPosString > 0; iPosString--) {
					offset_Archivo--;
					if (sDatosLeidos[iPosString] == '\n') {
						bEncontradoBarraN = true;
						break;
					}
				}
				//FIXME Si un Bloque No tiene un BarraN No explota y deberia! Ademas hay riesgo de quedarse en Bucle Infinito o Que encuentre un BarraN de un Bloque Anterior, que no le corresponde al Bloque Actual
			}
			posInicioBloque = offset_Archivo;
		}
	}
	numeroError = close(pArchivo);
	pArchivo = 0;
	if(numeroError==-1){
		Macro_Imprimir_Error("Fallo el Close del Archivo:'%s'", sRutaArchivo);
	}


	Comun_LiberarMemoria((void**) &sDatosLeidos);

	Macro_Check_And_Handle_Error(cantidadBloques == 0 ,"Error Cantidad de Bloques es 0");		//No se respetaba la Interfaz ni el Contrato de la Funcion, Arreglado
	return cantidadBloques;


Error_Handler:
	if (pArchivo != 0) {
		numeroError = close(pArchivo);
		if(numeroError==-1){
			Macro_Imprimir_Error("Fallo el Close del Archivo:'%s'", sRutaArchivo);
		}
	}

	Comun_LiberarMemoria((void**) &sDatosLeidos);

	return -1;

/*	VIEJA Idea: Aproximar Matematicamente
	uint32_t 	cantidadBloques = -1;
	int 		resto = 0;

	Macro_Check_And_Handle_Error(tamanioArchivo <= 0, "Tamanio de Archivo Invalido")

	cantidadBloques = tamanioArchivo / TAMANIO_BLOQUE;
	resto = tamanioArchivo % TAMANIO_BLOQUE;

	//En el Caso que no Dividio Justo, Debemos Agregarle un Bloque Mas
	if (resto != 0) {
		cantidadBloques++;
	}

	//Siempre va 1 Bloque Mas, porque el 90% de las veces el \n esta corrido y termina dando un Bloque extra
	cantidadBloques++;

	return cantidadBloques;

Error_Handler:
	return -1;
*/
}


char* Bloques_serializar(const tipo_bloque* punteroBloque, uint32_t* tamanioSerializacion) {
	//Uso malloc porque no quiero que se anden copiando 20mb entre funciones, prefiero que se pasen un puntero a siempre la misma memoria
	char* punteroBloqueSerializado = malloc(punteroBloque->tamanioBloque + sizeof(punteroBloque->tamanioBloque));
	//Offset para el Memcpy
	uint32_t offsetTamanioBloque = sizeof(punteroBloque->tamanioBloque);

	//OLD, no lo borro por las dudas
	//El "+1" es porque agregue el caracter de Fin de String \0
	//mmemcpy(punteroBloqueSerializado,punteroBloque,TAMANIO_BLOQUE+1);

	//Primero Copio la Longitud del Bloque
	memcpy(punteroBloqueSerializado , &(punteroBloque->tamanioBloque) , sizeof(punteroBloque->tamanioBloque));

	//Segundo Copio el contenido del  Bloque, a partir de donde termine de copiar antes
	//Si hay problemas restablecer el '&' delante de "(punteroBloque->contenidoBloque)"
	memcpy(punteroBloqueSerializado + offsetTamanioBloque , (punteroBloque->contenidoBloque) , punteroBloque->tamanioBloque);

	//Podria aca usar Bloques_destruir(), pero no se si va a querer seguir usando al bloque fuera de la funcion... Por eso No lo uso y dejo que EL lo haga a mano

	//Devuelvo por Referencia el tamaño de la serializacion
	*tamanioSerializacion = punteroBloque->tamanioBloque + sizeof(punteroBloque->tamanioBloque);

	return punteroBloqueSerializado;
	//Deben recordar hacerle FREE despues de usar "package_create"..
}

tipo_bloque* Bloques_des_serializar(const char* bloqueSerializado){
	//Creo un bloque para cargarle los datos des-serializados
	tipo_bloque* punteroBloque = Bloques_crear_nuevo();
	//Offset para el Memcpy
	uint32_t offsetTamanioBloque = sizeof(punteroBloque->tamanioBloque);

	//Primero Des-serializo la longitud del bloque,    debo usar "&" porque debe escribir a partir de la direccion de memoria de "punteroBloque->tamanioBloque"
	memcpy(&punteroBloque->tamanioBloque, bloqueSerializado, sizeof(punteroBloque->tamanioBloque));

	//Segundo des-serializo el contenido del Bloque (uso la longitud para saber el tamaño)
	//Si hay problemas restablecer el '&' delante de "(punteroBloque->contenidoBloque)"
	memcpy(punteroBloque->contenidoBloque,bloqueSerializado + offsetTamanioBloque, punteroBloque->tamanioBloque);


	return punteroBloque;
}

