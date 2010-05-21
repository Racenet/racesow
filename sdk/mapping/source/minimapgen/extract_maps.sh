#!/bin/bash

if [ $# != 4 ]
then
	echo "Usage: $0 BASEDIR GAME Q3MAP2 OUTPUTDIR"
	echo "Extracts all .pk3 archives out of the specified game's"
	echo "directory and copies the .bsp files into OUTPUT dir."
	echo
	echo "Example: $0 $HOME/warsow \\"
	echo "             basewsw \\"
	echo "             $HOME/q3map2 \\"
	echo "             $HOME/minimaptest"
	exit 1
fi

BASEDIR=$1
GAME=$2
GAMEDIR=$BASEDIR/$GAME
Q3MAP2=$3
OUTPUTDIR=$4

for i in $GAMEDIR/*.pk3; do
	TMPDIR=`mktemp -d` || exit 1
	unzip -d $TMPDIR $i maps/*.bsp
	cp $TMPDIR/maps/*.bsp $OUTPUTDIR
	rm -r $TMPDIR
done

for i in $OUTPUTDIR/*.bsp; do
	$Q3MAP2 -fs_basepath $BASEDIR -fs_game $GAME -game qfusion -convert $i
done
