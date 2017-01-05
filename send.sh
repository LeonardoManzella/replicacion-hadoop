#!/bin/bash
rxip='([1-9]?[0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])'

#leer parametro de linea de comandos, y verificar que no sea vacio
if [ ! -n "$1" ]; then
	printf "Debe ingresar un modulo a copiar (commons, fs, marta, nodo, job, all, tp)\n"
	exit
fi
case "$1" in
	*fs* ) 
		tar -cvzf deploy/fs.tar.gz deploy/view deploy/fs deploy/cfgfs.sh deploy/FSconfig.cfg
		;;
	*marta* )
		tar -cvzf deploy/marta.tar.gz deploy/marta deploy/MartaConfig.cfg
		;;
	*nodo* )
		tar -cvzf deploy/nodo.tar.gz deploy/nodo deploy/config.cfg deploy/cfgnodo.sh deploy/taskRunner
		;;
	*job* )
		tar -cvzf deploy/job.tar.gz deploy/job deploy/JOBConfig.cfg deploy/cfgjob.sh deploy/mapper deploy/reducer deploy/top-sent.pl deploy/basic_snetiment_analysis.py deploy/mapper.sh deploy/reduce.pl deploy/map.py deploy/reduce.py
		;;
	*commons* )
		tar -cvzf deploy/commons.tar.gz ../so-commons-library/*
		;;
	*all* )
		tar -cvzf --exclude "bin/*" --exclude "bin" --exclude "obj/*" --exclude "obj" --exclude "deploy/*" --exclue "deploy" deploy/all.tar.gz ../so-commons-library/* ./* #excluir bin obj deploy
		;;
	*tp* )
		tar -cvzf --exclude "bin/*" --exclude "bin" --exclude "obj/*" --exclude "obj" --exclude "deploy/*" --exclue "deploy" deploy/tp.tar.gz ./* #excluir bin obj deploy
		;;
esac
#preguntar por ip y verificar que sea valida

if [ -n "$2" ]; then
	password="$2"
else
	password=
	printf "\nIngrese la clave: "
	stty -echo
	read password
	stty echo
	printf "\n"
fi


if [ -n "$3" ]; then
	ipdestino="$3"
else
	ipdestino=
	while true; do
		printf "Direccion Ip de destino: "
		read ipdestino
		if [ -n "$ipdestino" ]; then
			if [[ $ipdestino =~ ^$rxip\.$rxip\.$rxip\.$rxip$ ]]; then
				break
			else
				printf "Direccion IP invalida\n"
				ipdestino=
			fi
		fi
	done
fi

printf "Copiando archivos\n"
./sshpass -p $password scp -o StrictHostKeyChecking=no -pC deploy/$1.tar.gz $ipdestino:/home/utnso/$1.tar.gz
printf "Descomprimiendo archivos remotos\n"
./sshpass -p $password ssh -o StrictHostKeyChecking=no $ipdestino tar -xvf $1.tar.gz
printf "Configurando $1\n"

if [[ "$1" == *nodo* ]]; then
	./sshpass -p $password ssh -o StrictHostKeyChecking=no $ipdestino "cd deploy; ./cfg$1.sh $4 $5 $6"
fi

if [[ "$1" == *job* ]]; then
	if [ -n "$4" ]; then 
		./sshpass -p $password ssh -o StrictHostKeyChecking=no $ipdestino "cd deploy; ./cfg$1.sh $4 $5"
		exit
	fi
	printf "Que Job desea configurar? "
	read numjob
	./sshpass -p $password ssh -o StrictHostKeyChecking=no $ipdestino "cd deploy; ./cfg$1.sh $numjob"
fi

