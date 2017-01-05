C_SRCS_PETMARTA += \
Tests/Test_FS_PeticionMarta/main.c 

OBJS_PETMARTA += \
obj/Tests/Test_FS_PeticionMarta/main.o 

C_DEPS_PETMARTA += \
obj/Tests/Test_FS_PeticionMarta/main.d 


# Each subdirectory must supply rules for building sources it contributes
obj/Tests/Test_FS_PeticionMarta/%.o: Tests/Test_FS_PeticionMarta/%.c
	@mkdir -p obj/Tests/Test_FS_PeticionMarta
	@printf "$(MSGOBJ) $< \r"
	@if gcc $(FLAGS) -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<" $(OUTPUT) ; then \
        printf "$(COLUMNA)$(VERDE)$(COMPILADO)$(SINCOLOR)\n" ; \
        else printf "$(COLUMNA)$(ROJO)$(ERROR)$(SINCOLOR)\n" ; fi



