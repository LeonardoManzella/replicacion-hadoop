#!/bin/bash
make all
rxno='([n])'
rxip='([1-9]?[0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])'
rxpuerto='([1-9]?[0-9]{3}|[1-5][0-9]{4}|6[0-5]{2}[0-3][0-5])'
eth="eth0"
#if [ -n "$1" ]; then
#	eth="$1"
#fi
iplocal="$(ifconfig | grep -A 1 $eth | tail -1 | cut -d ':' -f 2 | cut -d ' ' -f 1)"

conffsmarta=
printf "Desea configurar FS y Marta (s/n)?"
read conffsmarta
if [[ $conffsmarta == [sS] ]]; then

	ipfs=
	while true; do
		printf "Direccion Ip del FileSystem ($iplocal): "
		read ipfs
		if [ ! -n "$ipfs" ]; then
			ipfs="$iplocal";
			break;
		else
			if [[ $ipfs =~ ^$rxip\.$rxip\.$rxip\.$rxip$ ]]; then
				break
			else
				printf "Direccion IP invalida\n"
				ipfs=
			fi
		fi
	done

	puertofs=
	puertodef=6000
	while true; do
		printf "Puerto del FS ($puertodef): "
		read puertofs
		if [ ! -n "$puertofs" ]; then
			puertofs="$puertodef";
			break;
		else
			if [[ $puertofs =~ ^$rxpuerto$ ]]; then
				break
			else
				printf "Puerto invalido\n"
				puertofs=
			fi
		fi
	done

	ipmarta=
	while true; do
		printf "Direccion Ip de Marta ($iplocal): "
		read ipmarta
		if [ ! -n "$ipmarta" ]; then
			ipmarta="$iplocal";
			break;
		else
			if [[ $ipmarta =~ ^$rxip\.$rxip\.$rxip\.$rxip$ ]]; then
				break
			else
				printf "Direccion IP invalida\n"
				ipmarta=
			fi
		fi
	done

	puertomarta=
	puertodef=5000
	while true; do
		printf "Puerto de Marta ($puertodef): "
		read puertomarta
		if [ ! -n "$puertomarta" ]; then
			puertomarta="$puertodef";
			break;
		else
			if [[ $puertomarta =~ ^$rxpuerto$ ]]; then
				break
			else
				echo "Puerto invalido\n"
				puertomarta=
			fi
		fi
	done
else
	ipfs="$(sed -n -e 's/^\s*IP_FS\s*=\s*//p' deploy/config.cfg)"
	puertofs="$(sed -n -e 's/^\s*PUERTO_FS\s*=\s*//p' deploy/config.cfg)"
	ipmarta="$(sed -n -e 's/^\s*IP_MARTA\s*=\s*//p' deploy/JOBConfig.cfg)"
	puertomarta="$(sed -n -e 's/^\s*PUERTO_MARTA\s*=\s*//p' deploy/JOBConfig.cfg)"
fi

mkdir -p deploy

cp FileSystem/cfgfs.sh deploy/
cp bin/fs/fs deploy/
cp bin/fs/view deploy/
if [[ $conffsmarta == [sS] ]]; then
	touch deploy/FSconfig.cfg
	printf "PUERTO_LISTEN=$puertofs\nCANTIDAD_NODOS_MINIMOS=3\n" > deploy/FSconfig.cfg
fi

cp bin/marta/marta deploy/
if [[ $conffsmarta == [sS] ]]; then
	touch deploy/MartaConfig.cfg
	printf "FS_IP=$ipfs\nFS_PUERTO=$puertofs\nMARTA_PUERTO_ESCUCHA=$puertomarta\n" > deploy/MartaConfig.cfg
fi

cp Nodo/cfgnodo.sh deploy/
cp bin/nodo/nodo deploy/
cp bin/nodo/taskRunner deploy/
if [[ $conffsmarta == [sS] ]]; then
	touch deploy/config.cfg
	printf "IP_FS=$ipfs\nPUERTO_FS=$puertofs\nARCHIVO_BIN=datos.bin\nDIR_TEMP=/tmp/\nNODO_NUEVO=SI\nIP_NODO=127.0.0.1\nPUERTO_NODO=8088\nNOMBRE_NODO=nodo1\nTIEMPO_REINTENTO_FORK=15\n" > deploy/config.cfg
fi

cp Job/cfgjob.sh deploy/
cp bin/job/job deploy/
if [[ $conffsmarta == [sS] ]]; then
	touch deploy/JOBConfig.cfg
	printf "PUERTO_LISTEN=10001\nIP_MARTA=$ipmarta\nPUERTO_MARTA=$puertomarta\nMAPPER=mapper.sh\nREDUCE=reduce.pl\nCOMBINER=false\nARCHIVOS=[/InFile.txt]\nRESULTADO=/ArchivoFinal.txt\n" > deploy/JOBConfig.cfg
fi

printf "\nIngrese la clave para enviar archivos: "
stty -echo
read password
stty echo
printf "\n"

enviar=
printf "Enviar FileSystem (s/n)?"
read enviar
if [[ $enviar == [sS] ]]; then
	./send.sh fs $password $ipfs
	enviar=	
fi

printf "Enviar Marta (s/n)?"
read enviar
if [[ $enviar == [sS] ]]; then
	./send.sh marta $password $ipmarta
	enviar=
fi

comandos=()
if [ -n "$1" ]; then
	while IFS='= ' read nombre valor
	do
    		if [[ $nombre == \[*] ]]; then
        		modulo=$nombre
			modulo=${modulo#*[}
			modulo=${modulo%]*}
    		elif [[ $valor ]]
   		then
			comandos+=("$modulo $password $nombre $valor")
    		fi
	done < $1
	for i in "${comandos[@]}"
	do
   		./send.sh $i
	done
	exit
fi

while true; do
	printf "Enviar Nodo (s/n)?"
	read enviar
	if [[ $enviar == [sS] ]]; then
		./send.sh nodo $password
		enviar=
	else
		break
	fi
done

while true; do
	printf "Enviar Job (s/n)?"
	read enviar
	if [[ $enviar == [sS] ]]; then
		./send.sh job $password
		enviar=
	else
		break
	fi
done

exit

