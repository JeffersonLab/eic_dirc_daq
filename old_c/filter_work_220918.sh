#!/bin/sh
#source /u/pandadrc/.bashrc
#. /home/greg/.bashrc
echo DATE $6
skill moni_max_fast_i
skill moni_max_fast
echo -1 -666 -666  > /tmp/filter.dat
sleep 60

#/u/pandadrc/georg/ctlab_gs/moni_max_fast matrix &
#/u/pandadrc/georg/ctlab_gs/moni_max_fast_info matrix &
#/u/pandadrc/georg/ctlab_gs/moni_max_fast_info_test_georg matrix &
/u/pandadrc/georg/ctlab_gs/moni_max_fast_info_test_Hang_Georg_Test matrix &


#(all x,y values has to be multiplied by 10)

#	     diode scan			    diode Brewster position	 "Bar scan" (only for itterations)  "Bar position"	diode pure position (-half scan width) mirror-brewsterangle-mot5
#filter.sh   x_width  y_width  xy_step      x_ref y_ref     		 xb_width yb_width xyb_step 	    xb_ref yb_ref     	x_pure_once y_pure_once





       #442nm
#====================

filter_220918.sh    50 50 5 110 2090 60 200 20 650 1380 1980 2100 $7 		# Nikon 3	faces IR	new for series shorter



echo -666 > /tmp/filter.dat # end moni_ct
sleep 2
echo -1 > /tmp/filter.dat

mv -v $HOMEGREG/ctlab/moni_ct.dat $HOMEGREG/data/$1_$2-$3_$4_$5nm_$6_1.dat
mv -v $HOMEGREG/ctlab/moni_ct2.dat $HOMEGREG/data/$1_$2-$3_$4_$5nm_$6_2.dat
mv -v $HOMEGREG/ctlab/moni_ct3.dat $HOMEGREG/data/$1_$2-$3_$4_$5nm_$6_3.dat
mv -v $HOMEGREG/ctlab/moni_ct4.dat $HOMEGREG/data/$1_$2-$3_$4_$5nm_$6_4.dat

echo ""
echo ">>>>>>>>>>>>>>>>>>>>>>>> Stop at:    `date +%Y.%m.%d-%H:%M:%S`"
echo ""
