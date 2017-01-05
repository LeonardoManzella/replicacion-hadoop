C_SRCS_FS += \
FileSystem/main_File_System.c 

OBJS_FS += \
obj/FileSystem/main_File_System.o 

C_DEPS_FS += \
obj/FileSystem/main_File_System.d 

obj/FileSystem/%.o: FileSystem/%.c
	@mkdir -p obj/FileSystem
	@printf "$(MSGOBJ) $< \r"
	@if gcc $(FLAGS) -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<" $(OUTPUT) ; then \
        printf "$(COLUMNA)$(VERDE)$(COMPILADO)$(SINCOLOR)\n" ; \
        else printf "$(COLUMNA)$(ROJO)$(ERROR)$(SINCOLOR)\n" ; fi



	
