#!/bin/bash

cat ipconfig.xml | awk -F[=\"] '/ip/{print $3}' | while read line
do 

	ipchar=`echo $line | awk -F, '{print $1}'` 

	echo "$ipchar" 
	nfdc register /yl udp://$ipchar
	nfdc create -P udp://$ipchar

done
