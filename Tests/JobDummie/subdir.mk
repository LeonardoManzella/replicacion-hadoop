C_SRCS_JOBD += \
Tests/JobDummie/Main_Dummie_JOB.c 

OBJS_JOBD += \
obj/Tests/JobDummie/Main_Dummie_JOB.o 

C_DEPS_JOBD += \
obj/Tests/JobDummie/Main_Dummie_JOB.d 


# Each subdirectory must supply rules for building sources it contributes
obj/Tests/JobDummie/%.o: Tests/JobDummie/%.c
	@mkdir -p obj/Tests/JobDummie
	@printf "$(MSGOBJ) $< \r"
	@if gcc $(FLAGS) -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<" $(OUTPUT) ; then \
        printf "$(COLUMNA)$(VERDE)$(COMPILADO)$(SINCOLOR)\n" ; \
        else printf "$(COLUMNA)$(ROJO)$(ERROR)$(SINCOLOR)\n" ; fi



