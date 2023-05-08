#!/bin/bash
# bar matrix measurement (for even and odd number of reflections) ; need moni_ct matrix

if [ $# -ne 13 ] #7   8
#if [ $# -ne 12 ] #7   8
then
    echo "Usage GS: $0  \"x_width\"  \"y_width\" \"xy_step_size\" \"x_ref\" \"y_ref\" \"xb_width\" \"yb_width\" \"xyb_step_size\" \"xb_ref\" \"yb_ref\" \"xdpure_ref\" \"ydpure_ref\" \"angle-mot5\"in mm/10"
   exit
fi


#Definition of variables:


x_width=$1 #mm/10 
y_width=$2
echo "(mm/10)  x_width = $x_width ; y_width = $y_width"

x_step_size=$3 #mm/10 !!!Georg
y_step_size=$3
echo "(mm/10) Diode x_step_size = $x_step_size ; y_step_size = $y_step_size"

x_ref=$4 #mm/10
y_ref=$5
echo "start point(mm/10):  x_ref = $x_ref ; y_ref = $y_ref"

xb_width=$6 #mm/10 
yb_width=$7
echo "(mm/10)  xb_width = $xb_width ; yb_width = $yb_width"

xb_step_size=$8 #mm/10
yb_step_size=$8
echo "(mm/10) BAR xb_step_size = $xb_step_size ; yb_step_size = $yb_step_size"

xb_ref=$9 #mm/10
yb_ref=${10}
echo "start point(mm/10):  xb_ref = $xb_ref ; yb_ref = $yb_ref"

x_pure_scan_onces=${11} #mm/10
y_pure_scan_onces=${12}
echo "(mm/10) BAR xb_step_size = $xb_step_size ; yb_step_size = $yb_step_size"

angle_mot5=${13}
echo "angle_mot5 = $angle_mot5"

mot5=$angle_mot5
mot4=`echo "scale=1; 117.5 - 2 * ( 145 - $mot5 ) " | bc -l`
echo "mot4 = $mot4"


#Limits of the motor movements
mot4min=50
mot4max=180 
mot5min=99
mot5max=145
echo "mot4max = $mot4max"

if (( $(echo "$mot5 > $mot5max" | bc -l) )) 
then
    echo "motor5" $mot5
   exit
fi

if (( $(echo "$mot5 < $mot5min" | bc -l) )) 
then
    echo "motor5" $mot5
   exit
fi

if (( $(echo "$mot4 > $mot4max" | bc -l) ))
then
    echo "motor4" $mot4
   exit
fi

if (( $(echo "$mot4 < $mot4min" | bc -l) ))
then
    echo "motor4" $mot4
   exit
fi


#Variable to achive an order of measurements of Filter (0 2)pos1 (2 0)pos2 (0 2)pos3... 
faster=1
pure_once=0
x_scan_max=0
y_scan_max=0
scan_max=0
  
range_new_x=`echo "scale=1; ($x_width / $x_step_size) " | bc -l`
range_new_y=`echo "scale=1; ($y_width / $y_step_size) " | bc -l`
new_range=`echo "scale=1; ($range_new_x +1) * ($range_new_y +1) " | bc -l`


#Loop over Bar matrix points (xb,yb) and scan values for maximum search on the diode (x,y):
# DOUBLE LOOP over bar matrix (xb, yb)  and diode scan (x, y)     GS
# faster = 1 for pure beam(filter_number 0), -1 for no pure beam
# thus sequence is filter_numbers  0 2 2 0 0 2 2 0 0 2 2 0 0 ... 



#==============================================LOOPS =======================
width1=$xb_width
width2=$yb_width
step_size1=$xb_step_size
step_size2=$yb_step_size
if (( xb_width > yb_width ))
then
	width1=$yb_width
	width2=$xb_width
	step_size1=$yb_step_size
	step_size2=$xb_step_size
fi

#LOOPS =======================Bar LOOP x z

for(( bar_pos1 = 0 ; bar_pos1 <= width1 ; bar_pos1 += step_size1 ))
do
    for(( bar_pos2 = 0 ; bar_pos2 <= width2 ; bar_pos2 += step_size2 ))
    do
	xb=$bar_pos1
	yb=$bar_pos2
	if (( $xb_width > $yb_width ))
	then
		xb=$bar_pos2
		yb=$bar_pos1
	fi

        xb_scan=`echo "scale=1; ( $xb_ref / 10 ) + ( $xb / 10 )" | bc -l`      
        yb_scan=`echo "scale=1; ( $yb_ref / 10 ) + ( $yb / 10 )" | bc -l`


if [[ $faster == 1 ]]  #GS
then        
  if [[ $pure_once == 0 ]]  #GS
  then
  		
  #LOOPS =======================BDiode LOOP Direct Beam ONCE x z  

  
  for(( x = 0 ; x <= $x_width ; x+=$x_step_size ))
		do
			x_scan_once=`echo "scale=1; ( $x_pure_scan_onces / 10 ) + ( $x / 10 )" | bc -l`

			for (( y = 0 ; y <= $y_width ; y+=$y_step_size ))
			do
				y_scan_once=`echo "scale=1; ( $y_pure_scan_onces / 10 ) + ( $y / 10 )" | bc -l`
			
				
                   # pure beam
#$HOMEGREG/trunk/example <<EOF 
/u/pandadrc/georg/ctlab_gs/example <<EOF
            4 117.5
            0 $x_scan_once
            1 $y_scan_once
            2 56
            3 136
            5 145
            -1 -1
EOF
            filter_number=0 
            	       
echo "filter_number=  $filter_number 0 1" 
            # values given to filter.dat to be read by moni_ct.cc

            echo   $filter_number  $x_scan_once  $y_scan_once  $xb_scan  $yb_scan  10  $new_range 1 > /tmp/filter.dat

            while [ $filter_number -ne -1 ]
            do
                while read line
                do   
                    filter_number=$(echo $line | cut -d " " -f1)
                     sleep 0.002

                done < /tmp/filter.dat
            done
          done
		done
            pure_once=`echo "scale=1; $pure_once + 1" | bc -l`
            
   #LOOPS ======================= Diode LOOP Direct Beam ONCE x z    =========== END    
     #======================= Diode LOOP Direct Beam ONCE MAXIMUM    =========== END    
      
   else
   
  				
                   # pure beam


  # ------------------------ Diode  Direct Beam  Maximum Input from ONCE x z

   filter_number=0

echo "filter_number=  $filter_number 0 2" 

     echo   $filter_number  1111  213.5  $xb_scan  $yb_scan  10  $new_range 1 > /tmp/filter.dat
     
                 while [ $filter_number -ne -1 ]
            do
                while read line
                do		
                    filter_number=$(echo $line | cut -d " " -f1)
                   sleep 0.002
                done < /tmp/filter.dat
            done
     
     
   fi
fi
           
             
      faster=`echo "scale=1; $faster * (-1)" | bc -l`     
      
     #=======================Diode LOOP SCAN New MAXIMUM Position and Width    =========== END    
 
   if [[ $scan_max != 0 ]]  #GS
  then
   x_width=`echo "scale=1; 80 " | bc -l` #GS 
   y_width=`echo "scale=1; 40 " | bc -l`
   x_ref=`echo "scale=1; ($x_scan_max * 10) - ($x_width/2) " | bc -l`
   y_ref=`echo "scale=1; ($y_scan_max * 10) - ($y_width/2) " | bc -l`
 
  else
   x_width=`echo "scale=1; 120 " | bc -l` #GS 
   y_width=`echo "scale=1; 60 " | bc -l`
  fi
   x_step_size=`echo "scale=1; 10 " | bc -l` #GS
   y_step_size=`echo "scale=1; 5 " | bc -l`


   range_new_x=`echo "scale=1; ($x_width / $x_step_size) " | bc -l`
   range_new_y=`echo "scale=1; ($y_width / $y_step_size) " | bc -l`
   new_range=`echo "scale=1; ($range_new_x +1) * ($range_new_y +1) " | bc -l`
    

     
   echo "scan_max= $scan_max x_width= $x_width y_width= $y_width x_ref= $x_ref y_ref= $y_ref"  
   
   
   #LOOPS =======================Diode LOOP BREWSTER angle x z  
   
		for(( x = 0 ; x <= $x_width ; x+=$x_step_size ))
		do
			x_scan=`echo "scale=1; ( $x_ref / 10 ) + ( $x / 10 )" | bc -l`

			for (( y = 0 ; y <= $y_width ; y+=$y_step_size ))
			do
				y_scan=`echo "scale=1; ( $y_ref / 10 ) + ( $y / 10 )" | bc -l`
        
   echo "test  x_scan= $x_scan y_scan= $y_scan x_ref= $x_ref y_ref= $y_ref"  
	  
            # bar
     #x_1 266nm 116.848 325nm 117.009 405nm 117.117 442nm 117.148 532nm 117.198
            
#$HOMEGREG/trunk/example <<EOF  OLD
# IR
/u/pandadrc/georg/ctlab_gs/example <<EOF 
            4 $mot4
            0 $x_scan
            1 $y_scan
            2 $xb_scan
            3 $yb_scan
            5 $mot5
            -1 -1          
EOF
            filter_number=2 
            echo "filter_number=  $filter_number 2 1" 

            # xb_scan and yb_scan are positions of the bar-matrix GS 
            echo   $filter_number   $x_scan   $y_scan   $xb_scan   $yb_scan  10  $new_range 1 > /tmp/filter.dat

            while [ $filter_number -ne -1 ]
            do
                while read line
                do
                    filter_number=$(echo $line | cut -d " " -f1)
                    x_scan_max=$(echo $line | cut -d " " -f2)
                    y_scan_max=$(echo $line | cut -d " " -f3)
                    sleep 0.002
                done < /tmp/filter.dat
            done
 	      done
		done
echo "scan_max= $scan_max x_scan_max= $x_scan_max y_scan_max= $y_scan_max new_range  $new_range"	

   #LOOPS =======================Diode LOOP BREWSTER angle x z    ======== END


	
scan_max=`echo "scale=1; $scan_max + 1" | bc -l`

  if [[ $faster == 1 ]] #GS
  then

      # ------------------------ Diode  Direct Beam  Input from ONCE x z

                   # pure beam

            filter_number=0 
echo "filter_number=  $filter_number 0 3" 
            # xb_scan and yb_scan are positions of the bar-matrix GS

            echo    $filter_number  1111   213.5   $xb_scan  $yb_scan  10  $new_range 1 > /tmp/filter.dat

            while [ $filter_number -ne -1 ]
            do
                while read line
                do		 
                    filter_number=$(echo $line | cut -d " " -f1)
                    sleep 0.002
                done < /tmp/filter.dat
            done

  fi


   done
done
#LOOPS =======================Bar LOOP x z   ============= END
