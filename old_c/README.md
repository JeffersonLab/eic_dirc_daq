```
Dear Greg,

 I send you the programs I use to take data with our laser setup and to analyze them.

You will see that we worked with the idea “never change a winning team”.

Thus, the directory Marvin used and handed over to me has still the name “greg”
(greg/data, greg/filter, greg/ctlab). A lot of code will be very familiar with you.

However, this means that we still (have to) work with Root 5, since Root 6 demands 
“proper C++”. One could update the code in this sense, yet it was not done, since it works.

I will write you the sequence of data taking and analysis, and you can tell me if you needed more.



Data taking:



-	You call filter_input_220918.sh to initialize the measurement and give the proper name (built out of the parameters written)
-	This starts data taking with the shell script filter_work_220918.sh that gives the positions and step sizes for the motor movement and calls the program “moni.cc” or now “moni_max_fast_info_test_Hang_Georg_Test” and starts the same time the shell script filter_220918.sh that moves the motors
-	New is, that the program “moni” searches for a maximum position and gives this back to the motor steering script.

The file for the data taking “moni_max_fast_info_Hang.cc” has to be compiled as follows:
g++ moni_max_fast_info_Hang.cc calib_ct_lab.cxx serial_line.cxx -o moni_max_fast_info_test_Hang_Georg_Test








--- filter_input_220918.sh---

-	#!/bin/sh
-	#source /u/pandadrc/.bashrc
-	#. /home/greg/.bashr
-	d=`date +%y%m%d`
-	echo $d
-	
-	echo ""
-	echo ">>>>>>>>>>>>>>>>>>>>>>>> Start at:   `date +%Y.%m.%d-%H:%M:%S`"
-	echo ""
-	filter_work_220918.sh IR Nikon 3 faces 442 $d 117.15  
 
---End of filter_input_220918.sh ----



--filter_work_220918.sh ------------

#!/bin/sh
#source /u/pandadrc/.bashrc
#. /home/greg/.bashrc
echo DATE $6
skill moni_max_fast_i
skill moni_max_fast
echo -1 -666 -666  > /tmp/filter.dat
sleep 60

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

---------------End of filter_work_220918.sh --------------------


The output files are as follows (after the run they are renamed from moni_ct.dat to the specific name you gave in filter_input_220918.sh):

Example: IR_Nikon-11_faces_442nm_230322_1.dat
13.9236 1.84149 1.50045 0 0 65 138
13.9236 1.84432 1.49195 0 0 65 138
13.9236 1.81853 1.50557 0 0 65 138
13.9236 1.83111 1.51421 0 0 65 138
13.9236 1.84778 1.491 0 0 65 138
13.9236 1.80721 1.48943 0 0 65 138
13.9236 1.81067 1.49069 0 0 65 138
13.9236 1.82451 1.48408 0 0 65 138
13.9236 1.81853 1.50941 0 0 65 138
13.9236 1.8135 1.51165 0 0 65 138
13.9572 1.84778 1.52606 2 2 65 138
13.9572 1.86074 1.51742 2 2 65 138

With time, value-diode, reference-diode, temperature (not measured, thus placeholder 0 or 2), filter (0 for “bar out” and 2 for “bar in”), bar-x position, bar y-position. The values are measured 10 times each.

These values represent the Maximum value on the diode after it was scanned over the laser-spot.

The data files “Name”_2.dat, “Name”_3.dat“,  and Name”_4.dat are used only to analyze the correctness of the diode scan.

Using Root 5:
This file IR_Nikon-11_faces_442nm_230322_1.dat is than converted to a root file via the program “glasstest.cc
 
root glasstest.cc'("IR_Nikon11_faces_442nm_230322_1.dat")'

and gives the out put file IR_Nikon11_faces_442nm_230322_1.root

This again is analyzed in Root5 with matrix.cc

.x matrix.cc("IR_Nikon-11_faces_442nm_230322_1",442,0,0,0,0,true)

and gives the out put file IR_Nikon11_faces_442nm_230322_1_plot3D.root

The parameter given to “matrix.cc” are the wavelength, that changes the mirror correction and the boundaries x_min, x_max, y_min, and y_max of the matrix that are used for the determination of the transmission. With the value “true” and “false” choose between internal reflection and bulk absorption.

The output of matrix.cc is also a plot of the voltage values of the diodes and the transmission values of the matrix.

Please ask, if I missed to explain something or to send a file. 

Best regards,

 Georg
 ```
