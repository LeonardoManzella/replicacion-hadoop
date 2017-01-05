C_SRCS_JOB += \
Job/Main_JOB.c 

OBJS_JOB += \
obj/Job/Main_JOB.o 

C_DEPS_JOB += \
obj/Job/Main_JOB.d 


# Each subdirectory must supply rules for building sources it contributes
obj/Job/%.o: Job/%.c
	@mkdir -p obj/Job
	@printf "$(MSGOBJ) $< \r"
	@if gcc $(FLAGS) -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<" $(OUTPUT) ; then \
        printf "$(COLUMNA)$(VERDE)$(COMPILADO)$(SINCOLOR)\n" ; \
        else printf "$(COLUMNA)$(ROJO)$(ERROR)$(SINCOLOR)\n" ; fi



