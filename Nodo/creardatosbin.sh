#!/bin/bash
#echo "Creando archivo de datos"
let countm=$1*20
#(./pv -n /dev/zero | dd of=datos.bin bs=1M conv=notrunc,noerror iflag=fullblock count=$countm) 2>&1 | dialog --gauge "Creando archivo datos.bin" 10 70 0
dd if=/dev/zero of=datos.bin bs=1M count=$countm iflag=fullblock & pid=$!; while true; do sleep 1; [ -d "/proc/$pid" ] && kill -USR1 $pid || break; sleep 3; done
