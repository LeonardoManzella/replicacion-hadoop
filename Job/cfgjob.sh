#!/bin/bash
rxpuerto='([1-9]?[0-9]{3}|[1-5][0-9]{4}|6[0-5]{2}[0-3][0-5])'

case "$1" in 
	1 ) #primer job
		mapper="mapper.sh"
		reduce="reduce.pl"
		combiner="true"
		archivos="[\/mr\/weather\/201301hourly.txt, \/mr\/weather\/201302hourly.txt, \/mr\/weather\/201303hourly.txt, \/mr\/weather\/201304hourly.txt]"
		resultado="\/output\/job1\/max-temps.txt"
		;;
	2 ) #segundo job
		mapper="map.py"
		reduce="reduce.py"
		combiner="true"
		archivos="[\/mr\/textos\/gutenberg.txt, \/mr\/textos\/linux.txt]"
		resultado="\/output\/job2y3\/textos-comb.txt"
		;;
	3 ) #tercer job
		mapper="map.py"
		reduce="reduce.py"
		combiner="false"
		archivos="[\/mr\/textos\/gutenberg.txt, \/mr\/textos\/linux.txt]"
		resultado="\/output\/job2y3\/textos-nocomb.txt"
		;;
	4 ) #cuarto job
		mapper="mapper"
		reduce="reducer"
		combiner="true"
		archivos="[\/mr\/textos\/gutenberg.txt, \/mr\/textos\/linux.txt, \/mr\/textos\/kernel.txt]"
		resultado="\/output\/job4\/rep-letras.txt"
		;;
	5 ) #cuarto job
		mapper="basic_sentiment_analysis.py"
		reduce="top_sent.pl"
		combiner="false"
		archivos="[\/sentiment\/tweets.csv]"
		resultado="\/output\/job5\/tweets-sent.csv"
		;;
	* )
		mapper=
		reduce=
		combiner=
		archivos=
		resultado=
esac

if [ -n "$2" ]; then
	puertojob="$2"
else
	puertojob="$(sed -n -e 's/^\s*PUERTO_LISTEN\s*=\s*//p' JOBConfig.cfg)"
	while true; do
		printf "Puerto del Job ($puertojob): "
		read puerto
		if [ ! -n "$puerto" ]; then
			break
		else
			if [[ $puerto =~ ^$rxpuerto$ ]]; then
				puertojob=$puerto
				break
			else
				printf "\nPuerto invalido\n"
			fi
		fi
	done
fi
sed -i "/^PUERTO_LISTEN=/s/=.*/=$puertojob/" JOBConfig.cfg

if [ -n "$1" ]; then
	sed -i "/^MAPPER=/s/=.*/=$mapper/" JOBConfig.cfg
	sed -i "/^REDUCE=/s/=.*/=$reduce/" JOBConfig.cfg
	sed -i "/^COMBINER=/s/=.*/=$combiner/" JOBConfig.cfg
	sed -i "/^ARCHIVOS=/s/=.*/=$archivos/" JOBConfig.cfg
	sed -i "/^RESULTADO=/s/=.*/=$resultado/" JOBConfig.cfg
	printf "\n"
	cat JOBConfig.cfg
	exit
fi

mapper="$(sed -n -e 's/^\s*MAPPER\s*=\s*//p' JOBConfig.cfg)"
printf "Mapper ($mapper): "
read mappernew
if [ -n "$mappernew" ]; then
	mapper="$mappernew";
fi
sed -i "/^MAPPER=/s/=.*/=$mapper/" JOBConfig.cfg

reduce="$(sed -n -e 's/^\s*REDUCE\s*=\s*//p' JOBConfig.cfg)"
printf "Reduce ($reduce): "
read reducenew
if [ -n "$reducenew" ]; then
	reduce="$reducenew";
fi
sed -i "/^REDUCE=/s/=.*/=$reduce/" JOBConfig.cfg

combiner="$(sed -n -e 's/^\s*COMBINER\s*=\s*//p' JOBConfig.cfg)"
printf "Combiner ($combiner) (true/false): "
read combinertf
if [ -n "$combinertf" ]; then
	combiner="$combinertf"
fi
sed -i "/^COMBINER=/s/=.*/=$combiner/" JOBConfig.cfg

printf "\n"
cat JOBConfig.cfg
