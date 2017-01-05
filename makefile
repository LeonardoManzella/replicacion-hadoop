NARANJA := \033[0;35m
ROJO := \033[0;31m
SINCOLOR := \033[0m
VERDE := \033[0;32m
COLUMNA :=  \033[60C
COMPILADO = "Compilado"
ERROR = "Error!"
RM := rm -rf
DEBUG := -O0 -g3 -D_LARGEFILE_SOURCE=1 -D_LARGEFILE64_SOURCE=1  -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE #-w -D_GNU_SOURCES ignora warnings 
BUILD := -s -DNODEBUG -O3 -D_LARGEFILE_SOURCE=1 -D_LARGEFILE64_SOURCE=1 -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE
FLAGS := $(DEBUG)
CURRENT_DIR := $(shell pwd)
#LIBSGCC =  -Llib -Wl,-rpath=$(CURRENT_DIR)/lib
LIBSGCC = -Llib 
#LIBS := -ldb-6.1 -lpthread -lcommons -lssl -lcrypto
LIBS_DB := lib/libdb-6.1.a 
LIBS_SSL := lib/libssl.a lib/libcrypto.a
LIBS_SHR := -lpthread -lcommons $(LIBS_SSL) 
OBJS :=
OBJS_FS :=
OBJS_MARTA :=
C_SRCS :=
C_DEPS :=
C_DEPS_FS :=
C_DEPS_MARTA :=
MSGOBJ :=  #Compilando: 
MSGEXE := 
OUTPUT := 2>> builderr.log
#TARGET := fs job marta nodo martapet martasub jobdummie taskrunner
TARGET := fs job marta nodo
SUBDIRS := \
Sockets \
Serializador \
Tests/Test_FS_3NodosBasicos \
Tests/Test_FS_SubirArchivoFinal \
Tests/Test_FS_PeticionMarta \
Tests/ViewTables \
Tests/JobDummie \
FileSystem/src \
FileSystem \
Biblioteca_Comun \
Job \
Marta/src \
Marta \
Nodo/src \
taskRunner 

-include Sockets/subdir.mk
-include Serializador/subdir.mk
-include Tests/Test_FS_3NodosBasicos/subdir.mk
-include Tests/Test_FS_SubirArchivoFinal/subdir.mk
-include Tests/Test_FS_PeticionMarta/subdir.mk
-include Tests/ViewTables/subdir.mk
-include Tests/JobDummie/subdir.mk
-include FileSystem/src/subdir.mk
-include FileSystem/subdir.mk
-include Biblioteca_Comun/subdir.mk
-include Job/subdir.mk
-include Marta/src/subdir.mk
-include Marta/subdir.mk
-include Nodo/src/subdir.mk
-include taskRunner/subdir.mk

#ifneq (,$(findstring release, $(filter-out $@,$(MAKECMDGOALS)))
ifneq (,$(findstring r, $(MAKEFLAGS)))
	#LIBSGCC := -Llib -Wl,-rpath=./lib
	FLAGS := $(BUILD)
	#TARGET := fs job marta nodo taskrunner
endif

help: 
	@printf "make help\n Modulos disponibles: fs filesystem view nodo nodo_solo taskrunner job marta 3nodos martasub martapet jobdummie"
# All Target
all: $(TARGET)
	@printf "Compilacion Completa!\nPara ver Warnings y Errores vea el archivo builderr.log\n"


# Tool invocations
fs: filesystem view
nodo: nodo_solo taskrunner

filesystem:  destdirobj destdirbin $(OBJS) $(OBJS_FS)
	@mkdir -p bin/fs
	@printf "$(NARANJA)$(MSGEXE) bin/fs/$@ $(SINCOLOR) \r"
	@if gcc $(LIBSGCC) -o "bin/fs/fs" $(OBJS) $(OBJS_FS) $(LIBS_SHR) $(LIBS_DB)  $(OUTPUT) ; then \
        printf "$(COLUMNA)$(VERDE)$(COMPILADO)$(SINCOLOR)\n" ; \
        else printf "$(COLUMNA)$(ROJO)$(ERROR)$(SINCOLOR)\n" ; fi
	@cp FileSystem/FSconfig.cfg bin/fs/FSconfig.cfg

marta: destdirobj destdirbin $(OBJS) $(OBJS_MARTA)
	@mkdir -p bin/marta
	@printf "$(NARANJA)$(MSGEXE) bin/marta/$@ $(SINCOLOR) \r"
	@if gcc $(LIBSGCC) -o "bin/marta/marta" $(OBJS) $(OBJS_MARTA) $(LIBS_SHR) $(OUTPUT) ; then \
        printf "$(COLUMNA)$(VERDE)$(COMPILADO)$(SINCOLOR)\n" ; \
        else printf "$(COLUMNA)$(ROJO)$(ERROR)$(SINCOLOR)\n" ; fi
	@cp Marta/MartaConfig.cfg bin/marta/MartaConfig.cfg

job: destdirobj destdirbin $(OBJS) $(OBJS_JOB)
	@mkdir -p bin/job
	@printf "$(NARANJA)$(MSGEXE) bin/job/$@ $(SINCOLOR) \r"
	@if gcc $(LIBSGCC) -o "bin/job/job" $(OBJS) $(OBJS_JOB) $(LIBS_SHR) $(OUTPUT) ; then \
        printf "$(COLUMNA)$(VERDE)$(COMPILADO)$(SINCOLOR)\n" ; \
        else printf "$(COLUMNA)$(ROJO)$(ERROR)$(SINCOLOR)\n" ; fi
	@cp Job/JOBConfig.cfg bin/job/JOBConfig.cfg
	@cp Job/mapper.sh bin/job/mapper.sh
	@cp Job/reduce.pl bin/job/reduce.pl

nodo_solo:  destdirobj destdirbin $(OBJS) $(OBJS_NODO)
	@mkdir -p bin/nodo
	@printf "$(NARANJA)$(MSGEXE) bin/nodo/$@ $(SINCOLOR) \r"
	@if gcc $(LIBSGCC) -o "bin/nodo/nodo" $(OBJS) $(OBJS_NODO) $(LIBS_SHR) $(OUTPUT) ; then \
        printf "$(COLUMNA)$(VERDE)$(COMPILADO)$(SINCOLOR)\n" ; \
        else printf "$(COLUMNA)$(ROJO)$(ERROR)$(SINCOLOR)\n" ; fi
	@cp Nodo/config.cfg bin/nodo/config.cfg
	@cp Nodo/creardatosbin.sh bin/nodo/creardatosbin.sh

3nodos: destdirobj destdirbin $(OBJS) $(OBJS_FS_3N)
	@mkdir -p bin/tests
	@printf "$(NARANJA)$(MSGEXE) bin/tests/$@ $(SINCOLOR) \r"
	@if gcc $(LIBSGCC) -o "bin/tests/3Nodos" $(OBJS) $(OBJS_FS_3N) $(LIBS_SHR)  $(OUTPUT) ; then \
        printf "$(COLUMNA)$(VERDE)$(COMPILADO)$(SINCOLOR)\n" ; \
        else printf "$(COLUMNA)$(ROJO)$(ERROR)$(SINCOLOR)\n" ; fi

view: destdirobj destdirbin $(OBJS) $(OBJS_VT)
	@mkdir -p bin/fs
	@printf "$(NARANJA)$(MSGEXE) bin/fs/$@ $(SINCOLOR) \r"
	@if gcc $(LIBSGCC) -o "bin/fs/view" $(OBJS) $(OBJS_VT) $(LIBS_SHR) $(LIBS_DB) $(OUTPUT) ; then \
        printf "$(COLUMNA)$(VERDE)$(COMPILADO)$(SINCOLOR)\n" ; \
        else printf "$(COLUMNA)$(ROJO)$(ERROR)$(SINCOLOR)\n" ; fi

jobdummie: destdirobj destdirbin $(OBJS) $(OBJS_JOBD)
	@mkdir -p bin/tests
	@printf "$(NARANJA)$(MSGEXE) bin/tests/$@ $(SINCOLOR) \r"
	@if gcc $(LIBSGCC) -o "bin/tests/jobdummie" $(OBJS) $(OBJS_JOBD) $(LIBS_SHR) $(OUTPUT) ; then \
        printf "$(COLUMNA)$(VERDE)$(COMPILADO)$(SINCOLOR)\n" ; \
        else printf "$(COLUMNA)$(ROJO)$(ERROR)$(SINCOLOR)\n" ; fi

martapet: destdirobj destdirbin $(OBJS) $(OBJS_PETMARTA)
	@mkdir -p bin/tests
	@printf "$(NARANJA)$(MSGEXE) bin/tests/$@ $(SINCOLOR)\r"
	@if gcc $(LIBSGCC) -o "bin/tests/martapet" $(OBJS) $(OBJS_PETMARTA) $(LIBS_SHR) $(OUTPUT) ; then \
        printf "$(COLUMNA)$(VERDE)$(COMPILADO)$(SINCOLOR)\n" ; \
        else printf "$(COLUMNA)$(ROJO)$(ERROR)$(SINCOLOR)\n" ; fi

martasub: destdirobj destdirbin $(OBJS) $(OBJS_SUBMARTA)
	@mkdir -p bin/tests
	@printf "$(NARANJA)$(MSGEXE) bin/tests/$@ $(SINCOLOR)\r"
	@if gcc $(LIBSGCC) -o "bin/tests/martasub" $(OBJS) $(OBJS_SUBMARTA) $(LIBS_SHR) $(OUTPUT) ; then \
        printf "$(COLUMNA)$(VERDE)$(COMPILADO)$(SINCOLOR)\n" ; \
        else printf "$(COLUMNA)$(ROJO)$(ERROR)$(SINCOLOR)\n" ; fi

taskrunner: destdirobj destdirbin $(OBJS) $(OBJS_TASKRUNNER)
	@mkdir -p bin/nodo
	@printf "$(NARANJA)$(MSGEXE) bin/nodo/$@ $(SINCOLOR)\r"
	@if gcc $(LIBSGCC) -o "bin/nodo/taskRunner" $(OBJS) $(OBJS_TASKRUNNER) $(LIBS_SHR) $(OUTPUT) ; then \
        printf "$(COLUMNA)$(VERDE)$(COMPILADO)$(SINCOLOR)\n" ; \
        else printf "$(COLUMNA)$(ROJO)$(ERROR)$(SINCOLOR)\n" ; fi

clean:
	-$(RM) $(OBJS) $(OBJS_FS) $(OBJS_MARTA) $(OBJS_NODO) $(OBJS_JOB) $(OBJS_FS_3N) $(OBJS_VT) $(OBJS_JOBD) $(OBJS_PETMARTA) $(OBJS_SUBMARTA) $(OBJS_TASKRUNNER) 
	-$(RM) $(C_DEPS) $(C_DEPS_FS) $(C_DEPS_MARTA) $(C_DEPS_NODO) $(C_DEPS_JOB) $(C_DEPS_FS_3N) $(C_DEPS_VT) $(C_DEPS_JOBD) $(C_DEPS_PETMARTA) $(C_DEPS_SUBMARTA) $(C_DEPS_TASKRUNNER) 
	-$(RM) bin/fs/fs bin/marta/marta bin/job/job bin/nodo/nodo bin/tests/3Nodos bin/tests/jobdummie bin/fs/view bin/tests/martapet bin/tests/martasub bin/nodo/taskRunner
	-$(RM) builderr.log

destdirobj:
	@mkdir -p obj/$(SUBDIRS)

destdirbin:
	@mkdir -p bin

cleanall: clean
	-$(RM) bin/fs/fsdb

.PHONY: all clean
.SECONDARY:
