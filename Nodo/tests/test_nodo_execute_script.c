#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include "cspecs/cspec.h"
#include "../src/bibliotecaNodo.h"

//TODO verificar qué estoy haciendo mal al generar el script ya que al leerlo en la función, viene roto. Esto no pasa con un archivo creado a mano.
context(nodo_execute_script) {

	long finalSize = 20000000;
	char* in = "/tmp/in";
	char* out = "/tmp/out";
	char* script = "/tmp/script.sh";
	int returnValue = 0;
	int result = 0;

	//defino función para generar los archivos de pruebas
	int generateTestFiles(char* scriptFilePath, char* textFilePath, long finalSize) {

		char* shScript = "grep -c '.*prueba.*'";

		//armo archivo de script

		FILE* scriptFile = fopen(scriptFilePath, "w+");

		if(scriptFile == NULL) {
			fprintf(stderr, "Error al intentar abrir/crear archivo de script.");
			return EXIT_FAILURE;
		}

		if(0 == fwrite(shScript, strlen(shScript), 1, scriptFile)) {
			fprintf(stderr, "Error escribiendo a archivo de script.");
			return EXIT_FAILURE;
		}

		fclose(scriptFile);

		//armo archivo de texto

		FILE* textFile = fopen(textFilePath, "w+");

		if(textFile == NULL) {
			fprintf(stderr, "Error al intentar abrir/crear archivo de texto.");
			return EXIT_FAILURE;
		}

		char* str = "prueba1111111111111111111111111111111111111111111\n";

		int strSize = strlen(str);

		long fSize = 0;

		int result = -1;

		while(result != 0 && fSize <  finalSize) {
			result = fwrite(str, strSize, 1, textFile);
			fSize= fSize + strSize;
		}

		fprintf(stdout, "Archivo de prueba generado con éxito.\n");

		fclose(textFile);

		return result;
	}

	int getExecutionResult(char* resultFile) {

		//obtengo stat del script
		struct stat fileStat;

		if (stat(resultFile, &fileStat) == -1) {
			//falló stat.
			fprintf(stderr, "Error al realizar stat.");
			return EXIT_FAILURE;
		}

		FILE* file = fopen(resultFile,"r");

		if(file == NULL) {
			fprintf(stderr, "Error al intentar abrir/crear archivo de script.");
			return EXIT_FAILURE;
		}

		char* fileContent = malloc(fileStat.st_size);

		size_t result = fread(fileContent, fileStat.st_size, 1, file);

		if ( result == 0 || ferror(file) != 0 || strcmp(fileContent,"") == 0) {
			//error al leer archivo.
			fprintf(stderr, "Error al intentar leer el archivo resultante.");
			return EXIT_FAILURE;
		}

		fclose(file);

		return atoi(fileContent);
	}

	//genero archivos de prueba (script e in)
	generateTestFiles(script, in, finalSize);

	//ejecuto el método y obtengo su retorno
	returnValue = executeScript(script, in, out);
	//returnValue = executeScript("grep -c '.*prueba.*'",in,out);
	//obtengo resultado, escrito en el archivo out
	result = getExecutionResult(out);



	//validaciones
	describe("Función executeScript") {

		it("debería ser distinto de 1") {

			fprintf(stdout, "retorno de la función: %d", returnValue);
			should_int(returnValue) not be equal to(1);

		} end

		//50 es la longitud de bytes por línea del archivo generado
		it("debería ser finalSize/50") {

			fprintf(stdout, "retorno de la ejecución del script: %d", result);
			should_long(result) be equal to(finalSize/50);

		} end

	} end

}
