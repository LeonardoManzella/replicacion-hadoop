C_SRCS_MARTA += \
Marta/src/marta_server.c \
Marta/src/marta_comunicacion_padre_hijo.c \
Marta/src/marta_configuration.c \
Marta/src/marta_list_manipulation.c \
Marta/src/marta_hijo.c \
Marta/src/marta_victima.c 

OBJS_MARTA += \
obj/Marta/src/marta_server.o \
obj/Marta/src/marta_comunicacion_padre_hijo.o \
obj/Marta/src/marta_configuration.o \
obj/Marta/src/marta_list_manipulation.o \
obj/Marta/src/marta_hijo.o \
obj/Marta/src/marta_victima.o 

C_DEPS_MARTA += \
obj/Marta/src/marta_server.d \
obj/Marta/src/marta_comunicacion_padre_hijo.d \
obj/Marta/src/marta_configuration.d \
obj/Marta/src/marta_list_manipulation.d \
obj/Marta/src/marta_hijo.d \
obj/Marta/src/marta_victima.d 

obj/Marta/src/%.o: Marta/src/%.c
	@mkdir -p obj/Marta/src
	@printf "$(MSGOBJ) $< \r"
	@if gcc $(FLAGS) -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<" $(OUTPUT) ; then \
        printf "$(COLUMNA)$(VERDE)$(COMPILADO)$(SINCOLOR)\n" ; \
        else printf "$(COLUMNA)$(ROJO)$(ERROR)$(SINCOLOR)\n" ; fi



