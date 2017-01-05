#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include "cspecs/cspec.h"
#include "../src/bibliotecaNodo.h"

//TODO verificar qué estoy haciendo mal al generar el script ya que al leerlo en la función, viene roto. Esto no pasa con un archivo creado a mano.
context(nodo_serializadores) {

	bool resultado_comparacion;

	//genero el elemento a serializar
	t_datos_archivo_final datos_archivo_final;
	datos_archivo_final.tamanio = 20000000;
	datos_archivo_final.cantidad_bloques = 10;
	strcpy(datos_archivo_final.md5, "4200ee8b84e49ee7f3f111f87b1634b892\0");
	//serializo
	char* datos_archivo_final_serializado = serializar_datos_archivo_final(datos_archivo_final);

	//deserializo
	t_datos_archivo_final datos_archivo_final_deserializado = deserializar_datos_archivo_final(datos_archivo_final_serializado);

	//validaciones
	describe("Funciónes de serialización") {

		it("el tamaño debería ser el mismo") {
			resultado_comparacion = datos_archivo_final_deserializado.tamanio == datos_archivo_final.tamanio;

			fprintf(stdout, "%ul == %ul?", datos_archivo_final_deserializado.tamanio, datos_archivo_final.tamanio);

			should_bool(resultado_comparacion) be truthy;

		} end

		it("la cantidad de bloques debería ser la misma") {
			resultado_comparacion = datos_archivo_final_deserializado.cantidad_bloques == datos_archivo_final.cantidad_bloques;

			fprintf(stdout, "%ul == %ul?", datos_archivo_final_deserializado.cantidad_bloques, datos_archivo_final.cantidad_bloques);

			should_bool(resultado_comparacion) be truthy;

		} end

		it("el md5 debería ser el mismo") {

			resultado_comparacion = strcmp(datos_archivo_final_deserializado.md5, datos_archivo_final.md5) == 0;

			fprintf(stdout, "%s == %s?", datos_archivo_final_deserializado.md5, datos_archivo_final.md5);

			should_bool(resultado_comparacion) be truthy;

		} end

	} end

	//libero recursos
	datos_archivo_final_liberar_recursos(datos_archivo_final_serializado);

}
