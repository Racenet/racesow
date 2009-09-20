#!/bin/bash

if [ $# != 1 ]
then
	echo "Usage: $0 OUTPUTDIR"
	echo "Creates .tga files out of the .ase files in OUTPUTDIR."
	echo "These .tga files can be used as minimaps in Warsow."
	echo
	echo "Example: $0 $HOME/minimaptest"
	exit 1
fi

OUTPUTDIR=$1

for i in $OUTPUTDIR/*.ase; do
	python ase2png.py $i ${i/ase/png}
	convert ${i/ase/png} ${i/ase/tga}
done
