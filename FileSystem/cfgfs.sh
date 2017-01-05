#!/bin/bash
#sed -n -e 's/^\s*my_key\s*=\s*//p' my_file
nodosmin="$(sed -n -e 's/^\s*CANTIDAD_NODOS_MINIMOS\s*=\s*//p' FSconfig.cfg)"
printf "Cantidad de Nodos Minimos ($nodosmin): "
read nodos
if [ ! -n "$nodos" ]; then
	nodos="$nodosmin";
fi
sed -i "/^CANTIDAD_NODOS_MINIMOS=/s/=.*/=$nodos/" FSconfig.cfg
cat FSconfig.cfg
