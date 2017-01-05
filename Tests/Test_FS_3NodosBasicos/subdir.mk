C_SRCS_FS_3N += \
Tests/Test_FS_3NodosBasicos/Servidor_Nodos.c \
Tests/Test_FS_3NodosBasicos/main.c

OBJS_FS_3N += \
obj/Tests/Test_FS_3NodosBasicos/Servidor_Nodos.o \
obj/Tests/Test_FS_3NodosBasicos/main.o

C_DEPS_FS_3N += \
obj/Tests/Test_FS_3NodosBasicos/Servidor_Nodos.d \
obj/Tests/Test_FS_3NodosBasicos/main.d

obj/Tests/Test_FS_3NodosBasicos/%.o: Tests/Test_FS_3NodosBasicos/%.c
	@mkdir -p obj/Tests/Test_FS_3NodosBasicos
	@printf "$(MSGOBJ) $< \r"
	@if gcc $(FLAGS) -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<" $(OUTPUT) ; then \
        printf "$(COLUMNA)$(VERDE)$(COMPILADO)$(SINCOLOR)\n" ; \
        else printf "$(COLUMNA)$(ROJO)$(ERROR)$(SINCOLOR)\n" ; fi




	
