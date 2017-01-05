C_SRCS += \
Biblioteca_Comun/Biblioteca_Comun.c 

OBJS += \
obj/Biblioteca_Comun/Biblioteca_Comun.o 

C_DEPS += \
obj/Biblioteca_Comun/Biblioteca_Comun.d 


C_SRCS_FS += \
Biblioteca_Comun/2048.c 

OBJS_FS += \
obj/Biblioteca_Comun/2048.o 

C_DEPS_FS += \
obj/Biblioteca_Comun/2048.d 

obj/Biblioteca_Comun/%.o: Biblioteca_Comun/%.c
	@mkdir -p obj/Biblioteca_Comun
	@printf "$(MSGOBJ) $< \r"
	@if gcc $(FLAGS) -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<" $(OUTPUT) ; then \
	printf "$(COLUMNA)$(VERDE)$(COMPILADO)$(SINCOLOR)\n" ; \
	else printf "$(COLUMNA)$(ROJO)$(ERROR)$(SINCOLOR)\n" ; fi


