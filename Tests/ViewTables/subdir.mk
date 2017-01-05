C_SRCS_VT += \
Tests/ViewTables/main.c 

OBJS_VT += \
obj/Tests/ViewTables/main.o 

C_DEPS_VT += \
obj/Tests/ViewTables/main.d 


# Each subdirectory must supply rules for building sources it contributes
obj/Tests/ViewTables/%.o: Tests/ViewTables/%.c
	@mkdir -p obj/Tests/ViewTables
	@printf "$(MSGOBJ) $< \r"
	@if gcc $(FLAGS) -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<" $(OUTPUT) ; then \
        printf "$(COLUMNA)$(VERDE)$(COMPILADO)$(SINCOLOR)\n" ; \
        else printf "$(COLUMNA)$(ROJO)$(ERROR)$(SINCOLOR)\n" ; fi



