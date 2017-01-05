C_SRCS += \
Serializador/Protocolo_Marta_FS.c \
Serializador/Protocolo_Marta_JOB_Nodo.c \
Serializador/Protocolo_Nodos_FS.c \
Serializador/serialized_package.c 

OBJS += \
obj/Serializador/Protocolo_Marta_FS.o \
obj/Serializador/Protocolo_Marta_JOB_Nodo.o \
obj/Serializador/Protocolo_Nodos_FS.o \
obj/Serializador/serialized_package.o 

C_DEPS += \
obj/Serializador/Protocolo_Marta_FS.d \
obj/Serializador/Protocolo_Marta_JOB_Nodo.d \
obj/Serializador/Protocolo_Nodos_FS.d \
obj/Serializador/serialized_package.d 

obj/Serializador/%.o: Serializador/%.c
	@mkdir -p obj/Serializador
	@printf "$(MSGOBJ) $< \r"
	@if gcc $(FLAGS) -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<" $(OUTPUT) ; then \
        printf "$(COLUMNA)$(VERDE)$(COMPILADO)$(SINCOLOR)\n" ; \
        else printf "$(COLUMNA)$(ROJO)$(ERROR)$(SINCOLOR)\n" ; fi



	
