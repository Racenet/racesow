#!/bin/bash

if [ $# != 4 ]
then
	echo "Usage: $0 BASEDIR GAME Q3MAP2 OUTPUTDIR"
	echo "Converts all .bsp files in OUTPUTDIR to .ase using the game"
	echo "information taken from BASEDIR and GAME with Q3MAP2."
	echo
	echo "Example: $0 $HOME/warsow \\"
	echo "             basewsw \\"
	echo "             $HOME/q3map2 \\"
	echo "             $HOME/minimaptest"
	exit 1
fi

BASEDIR=$1
GAME=$2
Q3MAP2=$3
OUTPUTDIR=$4

for i in $OUTPUTDIR/*.bsp; do
#	$Q3MAP2 -fs_basepath $BASEDIR -fs_game $GAME -game warsow -convert $i
	$Q3MAP2 -fs_basepath $BASEDIR -fs_game $GAME -game qfusion -convert $i
done
