C_SRCS_NODO += \
Nodo/src/bibliotecaNodo.c \
Nodo/src/nodo.c

OBJS_NODO += \
obj/Nodo/src/bibliotecaNodo.o \
obj/Nodo/src/nodo.o 

C_DEPS_NODO += \
obj/Nodo/src/bibliotecaNodo.d \
obj/Nodo/src/nodo.d


# Each subdirectory must supply rules for building sources it contributes
obj/Nodo/src/%.o: Nodo/src/%.c
	@mkdir -p obj/Nodo/src
	@printf "$(MSGOBJ) $< \r"
	@if gcc $(FLAGS) -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<" $(OUTPUT) ; then \
        printf "$(COLUMNA)$(VERDE)$(COMPILADO)$(SINCOLOR)\n" ; \
        else printf "$(COLUMNA)$(ROJO)$(ERROR)$(SINCOLOR)\n" ; fi



