C_SRCS_MARTA += \
Marta/marta.c 

OBJS_MARTA += \
obj/Marta/marta.o 

C_DEPS_MARTA += \
obj/Marta/marta.d 


obj/Marta/%.o: Marta/%.c
	@mkdir -p obj/Marta
	@printf "$(MSGOBJ) $< \r"
	@if gcc $(FLAGS) -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<" $(OUTPUT) ; then \
        printf "$(COLUMNA)$(VERDE)$(COMPILADO)$(SINCOLOR)\n" ; \
        else printf "$(COLUMNA)$(ROJO)$(ERROR)$(SINCOLOR)\n" ; fi



