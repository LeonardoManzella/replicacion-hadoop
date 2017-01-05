#include "../../lib/include/db.h"
#include <stdbool.h>
#include <string.h>
#include <commons/bitarray.h>
#include "../../Biblioteca_Comun/Biblioteca_Comun.h"
#include "../../FileSystem/headers/nododb.h"
#include "../../FileSystem/headers/filedb.h"
#include "../../FileSystem/headers/bloqdb.h"
#include "../../FileSystem/headers/dirdb.h"

#define tipo_id int


typedef struct  __attribute__ ((__packed__)) {
        char sTabla[20]; // Nombre de la Tabla
        char sNombreArch[25]; // Nombre del Archivo
        DB *pdbTabla; // Puntero a la Taba una vez cargada
        tipo_id idUltimoRegistro; // Id del ultimo registro
        bool bDuplicado; // Si soporta duplicados
        DBC *pdbcTabla; // Puntero al Cursos para recorrer la tabla
        bool bIndiceAbierto; //Indica si es un indice y ya esta usado
        int iCantReg; //Cantidad de Registros de la Tabla?
        bool bFormatear; // Flag par saber si la tabla se formatea o no
} tipo_tabla_sistema;


int main(int argc, char* argv[]) {
	int iReturn, iUltimoID;
	DB_ENV *pEntornoDatos;
	iReturn = db_env_create(&pEntornoDatos,0);
	iReturn = pEntornoDatos->open(pEntornoDatos,"fsdb",DB_INIT_MPOOL, 0664); //DB_RECOVER y DB_CREATE juntos

	DB *pTabla;
	iReturn = db_create(&pTabla,pEntornoDatos,0);
	iReturn = pTabla->open(pTabla, NULL, "tablasis.db", NULL, DB_BTREE, 0, 0664);

	DBC *pCursorTabla;
	iReturn = pTabla->cursor(pTabla, NULL, &pCursorTabla, 0);

	tipo_tabla_sistema tTablaSistema;
	DBT key,data;
	memset(&key,0,sizeof(DBT));
	memset(&data,0,sizeof(DBT));
	iUltimoID = 0;

	printf("\n --- Datos TablaSis --- \n");
	printf("%4s|%20s|%25s|%10s|%4s|%4s|%10s|%4s|%4s|%4s\n","Id","Nombre Tabla","Nombre Archivo","puntTabl","IdUl","Dup","puntCurs","InAb","CanR","Frmt");
	iReturn = pCursorTabla->get(pCursorTabla, &key, &data, DB_FIRST);
	while (iReturn != DB_NOTFOUND) {
		tTablaSistema = *((tipo_tabla_sistema*) data.data);

 		printf("%4d|%20s|%25s|0x%08x|%4d|%4s|0x%08x|%4s|%4d|%4s\n", *((int*) key.data),
				tTablaSistema.sTabla,
				tTablaSistema.sNombreArch,
				(uint)tTablaSistema.pdbTabla,
				tTablaSistema.idUltimoRegistro,
				(tTablaSistema.bDuplicado) ? "True" : "Fals",
				(uint) tTablaSistema.pdbcTabla,
				(tTablaSistema.bIndiceAbierto) ? "True" : "Fals",
				tTablaSistema.iCantReg,
				(tTablaSistema.bFormatear) ? "True" : "Fals");

		if (  ( *((int*) key.data) ) > iUltimoID) {
			iUltimoID = *((int*)key.data);
		}
		iReturn = pCursorTabla->get(pCursorTabla, &key, &data, DB_NEXT);
	}
	printf("Ultimo ID: %d\n",iUltimoID);
	iUltimoID = 0;

	pCursorTabla->close(pCursorTabla);
	pTabla->close(pTabla,0);

	iReturn = db_create(&pTabla,pEntornoDatos,0);
	iReturn = pTabla->open(pTabla, NULL, "nodo.db", NULL, DB_BTREE, 0, 0664);

	iReturn = pTabla->cursor(pTabla, NULL, &pCursorTabla, 0);

	tipo_datos_nodo tDatosNodo;
	memset(&key,0,sizeof(DBT));
	memset(&data,0,sizeof(DBT));

	printf("\n --- Datos Tabla Nodo --- \n");
	printf("%4s|%50s|%17s|%5s|%4s|%10s|%4s|%4s|%4s|%4s\n","Id","Nombre Nodo","IP","Puert","Acti","puBitArr","Tota","Usad","Libr","Rese");
	iReturn = pCursorTabla->get(pCursorTabla, &key, &data, DB_FIRST);
	while (iReturn != DB_NOTFOUND) {
		tDatosNodo = *((tipo_datos_nodo*) data.data);
 		printf("%4d|%50s|%17s|%5s|%4s|0x%08x|%4d|%4d|%4d|%4d\n", *((int*)key.data),
 				tDatosNodo.sNombre,
				tDatosNodo.nodoIP,
				tDatosNodo.nodoPuerto,
				(tDatosNodo.activo) ? "True" : "Fals",
				(uint) tDatosNodo.tBloquesNodo,
				tDatosNodo.bloques_totales,
				tDatosNodo.bloques_usados,
				tDatosNodo.bloques_libres,
				tDatosNodo.bloques_reservados);
		if (  ( *((int*) key.data) ) > iUltimoID) {
			iUltimoID = *((int*)key.data);
		}
 		iReturn = pCursorTabla->get(pCursorTabla, &key, &data, DB_NEXT);
	}
	pCursorTabla->close(pCursorTabla);
	pTabla->close(pTabla,0);
	printf("Ultimo ID: %d\n",iUltimoID);
	iUltimoID = 0;

	iReturn = db_create(&pTabla,pEntornoDatos,0);
	iReturn = pTabla->open(pTabla, NULL, "dir.db", NULL, DB_BTREE, 0, 0664);

	iReturn = pTabla->cursor(pTabla, NULL, &pCursorTabla, 0);

	tipo_datos_dir tDatosDir;
	memset(&key,0,sizeof(DBT));
	memset(&data,0,sizeof(DBT));

	printf("\n --- Datos Tabla Dir --- \n");
	printf("%4s|%50s|%4s\n","Id", "Nombre Directorio","IdPa");
	iReturn = pCursorTabla->get(pCursorTabla, &key, &data, DB_FIRST);
	while (iReturn != DB_NOTFOUND) {
		tDatosDir = *((tipo_datos_dir*) data.data);
		printf("%4d|%50s|%4d\n", *((int*) key.data), tDatosDir.sNombre, tDatosDir.iPadre);
		if (  ( *((int*) key.data) ) > iUltimoID) {
			iUltimoID = *((int*)key.data);
		}
		iReturn = pCursorTabla->get(pCursorTabla, &key, &data, DB_NEXT);
	}
	pCursorTabla->close(pCursorTabla);
	pTabla->close(pTabla,0);
	printf("Ultimo ID: %d\n",iUltimoID);
	iUltimoID = 0;	

	iReturn = db_create(&pTabla,pEntornoDatos,0);
	iReturn = pTabla->open(pTabla, NULL, "file.db", NULL, DB_BTREE, 0, 0664);

	iReturn = pTabla->cursor(pTabla, NULL, &pCursorTabla, 0);

	tipo_datos_file tDatosFile;
	memset(&key,0,sizeof(DBT));
	memset(&data,0,sizeof(DBT));

	printf("\n --- Datos Tabla File --- \n");
	printf("%4s|%50s|%4s|%33s|%4s|%10s|%4s|%8s\n","Id","Nombre Archivo","IdPa","MD5","Disp","punBitArr","CaBl","TamArch");
	iReturn = pCursorTabla->get(pCursorTabla, &key, &data, DB_FIRST);
	while (iReturn != DB_NOTFOUND) {
		tDatosFile = *((tipo_datos_file*) data.data);
		printf("%4d|%50s|%4d|%33s|%4s|0x%08x|%4d|%8lu\n", *((int*) key.data),
				tDatosFile.sNombre,
				tDatosFile.dirPadre,
				tDatosFile.md5,
				(tDatosFile.disponible) ? "True" : "Fals",
				(uint) tDatosFile.tBloquesActivos,
				tDatosFile.iCantBloques,
				tDatosFile.tamanioArchivo);
		if (  ( *((int*) key.data) ) > iUltimoID) {
			iUltimoID = *((int*)key.data);
		}
		iReturn = pCursorTabla->get(pCursorTabla, &key, &data, DB_NEXT);
	}
	pCursorTabla->close(pCursorTabla);
	pTabla->close(pTabla,0);
	printf("Ultimo ID: %d\n",iUltimoID);
	iUltimoID = 0;

	iReturn = db_create(&pTabla,pEntornoDatos,0);
	iReturn = pTabla->open(pTabla, NULL, "bloques.db", NULL, DB_BTREE, 0, 0664);

	iReturn = pTabla->cursor(pTabla, NULL, &pCursorTabla, 0);

	tipo_datos_bloques tDatosBloques;
	memset(&key,0,sizeof(DBT));
	memset(&data,0,sizeof(DBT));

	printf("\n --- Datos Tabla Bloques --- \n");
	printf("%4s|%4s|%4s|%4s|%4s|%4s|%4s|%8s\n","Id","IdAr","NBlA","Copi","IdNo","NBlN","Disp","Tamano");
	iReturn = pCursorTabla->get(pCursorTabla, &key, &data, DB_FIRST);
	while (iReturn != DB_NOTFOUND) {
		tDatosBloques = *((tipo_datos_bloques*) data.data);
		printf("%4d|%4d|%4d|%4d|%4d|%4d|%4s|%8lu\n", *((int *) key.data),
				tDatosBloques.tIdArchivo,
				tDatosBloques.numeroBloqueDentroArchivo,
				tDatosBloques.copia,
				tDatosBloques.tIdNodo,
				tDatosBloques.numeroBloqueDentroNodo,
				(tDatosBloques.disponible) ? "True" : "Fals",
				(ulong) tDatosBloques.tamano);
		if (  ( *((int*) key.data) ) > iUltimoID) {
			iUltimoID = *((int*)key.data);
		}
		iReturn = pCursorTabla->get(pCursorTabla, &key, &data, DB_NEXT);
	}
	pCursorTabla->close(pCursorTabla);
	pTabla->close(pTabla,0);
	printf("Ultimo ID: %d\n", iUltimoID);
	iUltimoID = 0;

	pEntornoDatos->close(pEntornoDatos,0);

	return iReturn;
}
