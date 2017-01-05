C_SRCS += \
FileSystem/src/Biblioteca_Bloques.c \

OBJS += \
obj/FileSystem/src/Biblioteca_Bloques.o 

C_DEPS += \
obj/FileSystem/src/Biblioteca_Bloques.d 

C_SRCS_FS += \
FileSystem/src/Servidor_FileSystem.c \
FileSystem/src/bloqdb.c \
FileSystem/src/console.c \
FileSystem/src/dirdb.c \
FileSystem/src/filedb.c \
FileSystem/src/fsdb.c \
FileSystem/src/fslogic.c \
FileSystem/src/nododb.c 

OBJS_FS += \
obj/FileSystem/src/Servidor_FileSystem.o \
obj/FileSystem/src/bloqdb.o \
obj/FileSystem/src/console.o \
obj/FileSystem/src/dirdb.o \
obj/FileSystem/src/filedb.o \
obj/FileSystem/src/fsdb.o \
obj/FileSystem/src/fslogic.o \
obj/FileSystem/src/nododb.o 

C_DEPS_FS += \
obj/FileSystem/src/Servidor_FileSystem.d \
obj/FileSystem/src/bloqdb.d \
obj/FileSystem/src/console.d \
obj/FileSystem/src/dirdb.d \
obj/FileSystem/src/filedb.d \
obj/FileSystem/src/fsdb.d \
obj/FileSystem/src/fslogic.d \
obj/FileSystem/src/nododb.d 


obj/FileSystem/src/%.o: FileSystem/src/%.c
	@mkdir -p obj/FileSystem/src
	@printf "$(MSGOBJ) $< \r"
	@if gcc $(FLAGS) -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<" $(OUTPUT) ; then \
        printf "$(COLUMNA)$(VERDE)$(COMPILADO)$(SINCOLOR)\n" ; \
        else printf "$(COLUMNA)$(ROJO)$(ERROR)$(SINCOLOR)\n" ; fi


	
