C_SRCS += \
Sockets/Biblioteca_Sockets.c 

OBJS += \
obj/Sockets/Biblioteca_Sockets.o 

C_DEPS += \
obj/Sockets/Biblioteca_Sockets.d 

obj/Sockets/%.o: Sockets/%.c
	@mkdir -p obj/Sockets
	@printf "$(MSGOBJ) $< \r"
	@if gcc $(FLAGS) -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<" $(OUTPUT) ; then \
        printf "$(COLUMNA)$(VERDE)$(COMPILADO)$(SINCOLOR)\n" ; \
        else printf "$(COLUMNA)$(ROJO)$(ERROR)$(SINCOLOR)\n" ; fi



	
