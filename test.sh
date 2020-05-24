#! /bin/sh
SRCDIR=/Users/nakaki/project/lc2020/TMW


LOG="log/"`date +%Y%m%d_%H-%M-%S`.log

uname -a > $LOG
for IMG in `ls $SRCDIR/*.pgm`
do
  DEC=$IMG"_d"
	echo $IMG >> $LOG
	./lc2020_encoder $IMG $IMG.lc2020 | tee -a $LOG
	./lc2020_decoder $IMG.lc2020 $DEC.pgm | tee -a $LOG
	if cmp -s $IMG $DEC.pgm;
	then
		echo "OK!" | tee -a $LOG
		rm $DEC.pgm
		rm $IMG.lc2020
	else
		echo "ERROR!" | tee -a $LOG
		exit
	fi
done