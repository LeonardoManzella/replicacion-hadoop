#!/bin/bash
#sed -n -e 's/^\s*my_key\s*=\s*//p' my_file
rxip='([1-9]?[0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])'
rxpuerto='([1-9]?[0-9]{3}|[1-5][0-9]{4}|6[0-5]{2}[0-3][0-5])'

eth="eth0"
#if [ -n "$1" ]; then
#	eth="$1"
#fi
iplocal="$(ifconfig | grep -A 1 $eth | tail -1 | cut -d ':' -f 2 | cut -d ' ' -f 1)"

if [ -n "$1" ]; then
	sed -i "/^IP_NODO=/s/=.*/=$iplocal/" config.cfg
	sed -i "/^PUERTO_NODO=/s/=.*/=$2/" config.cfg
	sed -i "/^NOMBRE_NODO=/s/=.*/=$1/" config.cfg
	truncate --size=$3 datos.bin
	cat config.cfg
	exit
fi

archbin="$(sed -n -e 's/^\s*ARCHIVO_BIN\s*=\s*//p' config.cfg)"
printf "Nombre Archivo Bin ($archbin): "
read arch
if [ ! -n "$arch" ]; then
	arch="$archbin"
fi
sed -i "/^ARCHIVO_BIN=/s/=.*/=$arch/" config.cfg

nuevo="$(sed -n -e 's/^\s*NODO_NUEVO\s*=\s*//p' config.cfg)"
while true; do
	printf "Nodo Nuevo ($nuevo): "
	read nuevosn
	if [ ! -n "$nuevosn" ]; then
		nuevosn="$nuevo"
		break
	else
		if [[ $nuevosn == *SI* ]] || [[ $nuevosn == *NO* ]]; then
			printf "\n"
			break
		else
			printf "\n"
			echo "Debe ingresar SI o NO (en mayusculas)\n"
		fi
	fi
done
sed -i "/^NODO_NUEVO=/s/=.*/=$nuevosn/" config.cfg

ipnodo="$(sed -n -e 's/^\s*IP_NODO\s*=\s*//p' config.cfg)"
if [[ ! $ipnodo == $iplocal ]]; then
	printf "La IP Local ($iplocal) y la IP del Nodo ($ipnodo) son diferentes\n"
	printf "Desea usar la IP Local ($iplocal) (s/n)?\n"
	read usarip
	if [[ $usarip =~ [sS] ]]; then
		ipnodo=$iplocal
		printf "\n"
	fi
fi

while true; do
	printf "IP del Nodo ($ipnodo): "
	read ipnueva
	if [ -n "$ipnueva" ]; then
		if [[ $ipnueva =~ ^$rxip\.$rxip\.$rxip\.$rxip$ ]]; then
			ipnodo=$ipnueva
			break
		else
			printf "Direccion IP invalida\n"
		fi
	else
		break
	fi
done
sed -i "/^IP_NODO=/s/=.*/=$ipnodo/" config.cfg

puertonodo="$(sed -n -e 's/^\s*PUERTO_NODO\s*=\s*//p' config.cfg)"
while true; do
	printf "Puerto del Nodo ($puertonodo): "
	read puerto
	if [ ! -n "$puerto" ]; then
		break
	else
		if [[ $puerto =~ ^$rxpuerto$ ]]; then
			puertonodo=$puerto
			break
		else
			printf "\nPuerto invalido\n"
		fi
	fi
done
sed -i "/^PUERTO_NODO=/s/=.*/=$puertonodo/" config.cfg

nombrenodo="$(sed -n -e 's/^\s*NOMBRE_NODO\s*=\s*//p' config.cfg)"
printf "Nombre del Nodo ($nombrenodo): "
read nombre
if [ ! -n "$nombre" ]; then
	nombre="$nombrenodo";
fi
sed -i "/^NOMBRE_NODO=/s/=.*/=$nombre/" config.cfg

printf "Desea crear el archivo de datos (s/n)? "
read creardatos
if [[ $creardatos == [sS] ]]; then
	printf "\n"
	printf "Cuantos BLOQUES desea que tenga? "
	read bloques
	if [[ $bloques == 0 ]]; then
		printf "No se puede crear un archivo de 0 bloques\n"
	else 
		let countm=$bloques*20
		dd if=/dev/zero of=$arch bs=1M count=$countm iflag=fullblock & pid=$!; while true; do sleep 1; [ -d "/proc/$pid" ] && kill -USR1 $pid || break; sleep 3; done
	fi
fi

cat config.cfg
