C_SRCS_SUBMARTA += \
Tests/Test_FS_SubirArchivoFinal/main.c 

OBJS_SUBMARTA += \
obj/Tests/Test_FS_SubirArchivoFinal/main.o 

C_DEPS_SUBMARTA += \
obj/Tests/Test_FS_SubirArchivoFinal/main.d 


# Each subdirectory must supply rules for building sources it contributes
obj/Tests/Test_FS_SubirArchivoFinal/%.o: Tests/Test_FS_SubirArchivoFinal/%.c
	@mkdir -p obj/Tests/Test_FS_SubirArchivoFinal
	@printf "$(MSGOBJ) $< \r"
	@if gcc $(FLAGS) -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<" $(OUTPUT) ; then \
        printf "$(COLUMNA)$(VERDE)$(COMPILADO)$(SINCOLOR)\n" ; \
        else printf "$(COLUMNA)$(ROJO)$(ERROR)$(SINCOLOR)\n" ; fi



