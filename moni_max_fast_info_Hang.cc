#include <cstdlib>
#include <cstdio>

#include <cmath>
using std::sqrt;
#include <string>
using std::string;

#include <iostream>
using std::cout;
using std::cin;
using std::cerr;
using std::endl;

#include <iomanip>
using std::setw;
using std::setfill;

#include <fstream>
using std::fstream;

#include <ctime>

#include <vector>
using std::vector;

#include <sstream>
using std::ostringstream;

#include "calib_ct_lab.h"

// calibrate a ADC channel with an DAC channel
#include "serial_line.hxx"

// measure a routine in usec
//#include "runtime_measure.hh"  /////// CS TIPP!!!!!!!!!!!!!!!!!! change sleep 1 to 1000000 and than to 500000 or so


int fverbosity=0;


//----------------------------------------------------------------------
void sWrite(int fd, std::string str)
{
	char buffer[255];
	int nsize = str.size();
	for (int i=0; i<nsize; i++)
	{
		buffer[i] = *((str.substr(i,1)).c_str());
	}
	buffer[nsize] = 13;                 // terminate string with <CR> 
	int i = write(fd, buffer, nsize+1); 
	if (i<nsize+1) 
		cerr<<" *** write: only "<<i<<" characters written of " <<str<<endl;
	if (fverbosity>1)
	{
		std::cout<<" put: "<<str<<std::endl;
	}
	if(i == -1)
		cerr<<" *** probably one have to change the serial port name (USBX) in moni_ct.cc "<<endl; 
}
//----------------------------------------------------------------------
//replaced bz Carstens new faster way// GS April2021:
std::string sRead(int fd)
{
	char buffer[255];
	char *bufptr;      /* Current char in buffer */
	int  nbytes;       /* Number of bytes read */

	//int i=system("sleep 1");
	for (int i=0; i<255;i++) buffer[i]=0;

	bufptr = buffer;
	//while (nbytes=read(fd, bufptr, buffer + sizeof(buffer) - bufptr - 1), nbytes>0)
	//{
	//  bufptr += nbytes;
	//  //if (bufptr[-1] == '\n' || bufptr[-1] == '\r') break;
	//}
	while (true)
	{
		nbytes=read(fd, bufptr, buffer + sizeof(buffer) - bufptr - 1);
		bufptr += nbytes;
		if (bufptr[-1] == '\n') break;
	}

	/* nul terminate the string at last character which was \n or \r */
	*(bufptr) = '\0'; // not necessary.

	if (fverbosity>1)
	{
		std::cout<<" get: ";
		for (int i=0; i<80;i++) if (int(buffer[i])!=0) std::cout<<int(buffer[i])<<".";
		std::cout<<std::endl;    
		std::cout<<" get: "<<buffer<<std::endl;
	}

	return buffer;  
}

class Ctlab
{
	private: 

		int m_fd;

	public:
		Ctlab(int fd){m_fd=fd;};

		void write(string str){sWrite(m_fd,str);};
		float read()
		{
			string str = sRead(m_fd);
			int limiter = str.find("=");
			str.erase(0,limiter+1);

			return atof(str.data());
		};


};



//---------------------------------------------------------------------------------
int main( int argc, char *argv[] )
{    
	// 2 options: normal or matrix measurment (need x- & y-motor position)
	if( argc != 2 )
	{
		cerr << " *** Usage: \"moni_ct parameter\" ; parameter: either \"normal\" or \"matrix\"" << endl;
		exit( 0 );
	}

	string par =argv[1];

	if( par != "normal" && par != "matrix" )
	{
		cerr << " *** parameter is either \"normal\" or \"matrix\"" << endl;
		exit( 0 );
	}
	bool matrixMode = false;

	if( par == "matrix" )
		matrixMode = true;


	time_t curtime = time (NULL); //Get the current time.
	struct tm* loctime = localtime (&curtime); //Convert it to local time representation.

	float time_offset=loctime->tm_hour+loctime->tm_min/60.+loctime->tm_sec/3600.;
	time_t t0=curtime;

	const int iadc = 0;
	double volts[8];

	CalibCtLab calib;
	calib.readAdcFile(iadc,"/u/pandadrc/dirc_test/trunk/ctlab/calib_adc01.dat");

	SerialLine serial1("/dev/ttyUSB0",fverbosity,CTLAB); // due to a USB-mouse the ADC is either USB0 or USB1
	int fd = serial1.fileHandle();
	Ctlab ctlab(serial1.fileHandle());

	string sret;

	// out classical file
	fstream out;
	out.open("/u/pandadrc/dirc_test/trunk/ctlab/moni_ct.dat",std::ios::out);
	//GS ---- out2 file for shift of laser spot on diode depending on the position of the beam on the bar end
	fstream out2;
	out2.open("/u/pandadrc/dirc_test/trunk/ctlab/moni_ct2.dat",std::ios::out); 
	fstream outmax;
	outmax.open("/u/pandadrc/dirc_test/trunk/ctlab/moni_ct3.dat",std::ios::out);
	fstream outdiodenscan;
	outdiodenscan.open("/u/pandadrc/dirc_test/trunk/ctlab/moni_ct4.dat",std::ios::out);
	// ------


	int filter_number = -1; // default: -1 (break)
	double x_scan = -666; // motor position Diode for the matrix measurement
	double y_scan = -666;
	double xb_scan = -666; // motor position Bar for the matrix measurement //GS
	double yb_scan = -666; //GS

	int sleep_timer = 0; // parameter to optimize the time to wait
	int n_data = 0; // number of taken data points (0 means moni doesn't change filter.dat)
	// data points at one position
	int u=0;		// number of scan positions on te diode
	int v=0;		// to have only once the value for pure beam    
	int w=0;		// to have new range for Brewster scan
	int range=0; //285  // first for large range to find maximum !!!!!!!!! Test Georg 20.09.2022
	int new_range=0;   // for zoomed range to find maximum
	if( y_scan == -666 ) u=0;
	int f0=0;
	int f2=0;
	double mean_max = 0; //GS

	double buffer_max_time[10] = {0};//GS
	double buffer_max_ch0[10] = {0};//GS
	double buffer_max_ch1[10] = {0};//GS
	double buffer_max_ch2[10] = {0};//temp//GS

	double buffer_max_x_scan= 0;//GS
	double buffer_max_y_scan= 0;//GS

	double buffer_max_time_once[10] = {0};//GS  to have only once the values for pure beam
	double buffer_max_ch0_once[10] = {0};//GS
	double buffer_max_ch1_once[10] = {0};//GS
	double buffer_max_ch2_once[10] = {0};//temp//GS
	double buffer_max_x_scan_once= 0;//GS
	double buffer_max_y_scan_once= 0;//GS



	// for runtime measurement
	//  RuntimeMeasure *runtime = new RuntimeMeasure();


	try
	{
		while (true)
		{
			fstream in;

			do
			{
				in.open("/tmp/filter.dat",std::ios::in);
				if (in)
				{
					if( matrixMode )
					{
						in >> filter_number >> x_scan >> y_scan  >> xb_scan >> yb_scan >> n_data   >> new_range  >> sleep_timer; 

						//                         if( y_scan == -666 )>>
						//                         {
						//                             cerr << " *** filter.dat is not prepared for matrix mode" << endl;
						//                             exit( 0 );
						//                         }
					}
					else in >> filter_number >> n_data;


					// if filter.dat includes less parameter then error state flag will be set
					in.clear(); // clear error state flag
					in.close(); // strange: clear only works in combination with close
				}
				else
				{  
					cerr << " *** not possible to open filter.dat" << endl;

					in.clear(); // clear error state flag
					in.close();
					filter_number = -1;
				}

				//  if(sleep_timer == 0) usleep( 1000000 ); // to avoid permanently access to the hard disk
				//   else usleep( 50000 ); // to avoid permanently access to the hard disk
				usleep( 2000 );
			}   
			while( filter_number == -1 );


			if( filter_number == -666 )
				break;

			bool filter_change = true;
			if( n_data < 1 ) // moni runs w/o changing filter.dat
			{
				n_data = 1; // to avoid infinite loop
				filter_change = false;
			}
			// Fix
			// error: variable-sized object may not be initialized
			// by using memset to initialize arrays
			double buffer_time[n_data];
			memset(buffer_time, 0, sizeof(buffer_time));
			double buffer_ch0[n_data];
			memset(buffer_ch0, 0, sizeof(buffer_ch0));
			double buffer_ch1[n_data];
			memset(buffer_ch1, 0, sizeof(buffer_ch1));
			double buffer_ch2[n_data];//temp
			memset(buffer_ch2, 0, sizeof(buffer_ch2));


			if(x_scan<1111)
			{ 
				for( int j = 0; j < n_data; j++ )
				{
					volts[0] = 0; // value
					volts[1] = 0; // reference
					//                volts[2] = 0; // temperature
					int used_channels = 2; // old: 2


					for (int ich_adc = 0; ich_adc < used_channels; ich_adc++)
					{     
						ostringstream schannel;
						schannel<<ich_adc;
						string sout="0:VAL 1"+schannel.str()+"?";                  
						ctlab.write("0:VAL 1"+schannel.str()+"?"); //as carsten fast
						volts[ich_adc] +=ctlab.read();           //as carsten fast
						cout << "     volts=   " << volts[ich_adc] << endl;
					}
					cout << "     voltsratio=   " << volts[0]/volts[1] << endl;	 


					float time1 = time_offset + (time(NULL)-t0)/3600.0;




					if( matrixMode )
					{                   
						outmax << " " << time1 << " "
							//<< calib.adcValue(iadc,0,volts[0]) << " "
							//    << calib.adcValue(iadc,1,volts[1]) << " "
							<< calib.adcValue(iadc,0,volts[0]) << " "
							<< calib.adcValue(iadc,1,volts[1]) << " "
							<< calib.adcValue(iadc,2,volts[2]) << " "//temp
							//                             << calib.adcValue(iadc,7,volts[7]) << " "
							<< filter_number << " " 
							<< xb_scan << " " 
							<< yb_scan << " "
							<< x_scan << " " 
							<< y_scan 
							<< endl;


					}
					else 
					{
						cout << "time=" << time1 << "     volts=   "
							<< calib.adcValue(iadc,0,volts[0]) << " "
							<< calib.adcValue(iadc,1,volts[1]) << " "
							//                            << calib.adcValue(iadc,2,volts[2]) << " "//temp
							//                             << calib.adcValue(iadc,7,volts[7]) << " "
							<< filter_number 
							<< endl;
					}

					// test reference fluctuation
					buffer_time[j%n_data] = time1;
					buffer_ch0[j%n_data] = volts[0]; // VAL
					buffer_ch1[j%n_data] = volts[1]; // REF
					//               buffer_ch2[j%n_data] = volts[2]; // temp
					//                 buffer_ch7[j%10] = volts[2];

					if( j%n_data == n_data-1 )
					{
						double tot_num = 0, tot_den = 0, tot2 =0;
						for( int i = 0; i < n_data; i++ ){
							tot_num += calib.adcValue(iadc,0,buffer_ch0[i]);
							tot_den += calib.adcValue(iadc,1,buffer_ch1[i]);
						}

						double mean = tot_num / tot_den;

						if(mean > mean_max) //GS
						{	
							mean_max = mean;
							for( int m = 0; m < n_data; m++ )
							{
								buffer_max_time[m]= buffer_time[m];
								buffer_max_ch0[m]= buffer_ch0[m];
								buffer_max_ch1[m]= buffer_ch1[m];
								//                       buffer_max_ch2[m]= buffer_ch2[m];
							}
							buffer_max_x_scan= x_scan;
							buffer_max_y_scan= y_scan;
						}

						for( int l = 0; l < n_data; l++ )
						{
							outdiodenscan << buffer_time[l] << " "
								<< calib.adcValue(iadc,0,buffer_ch0[l]) << " "
								<< calib.adcValue(iadc,1,buffer_ch1[l]) << " "
								<< calib.adcValue(iadc,2,buffer_ch2[l]) << " "//temp
								//                                         << calib.adcValue(iadc,7,buffer_ch7[k]) << " "
								<< filter_number << " "
								<< xb_scan << " " 
								<< yb_scan << " "
								<< x_scan << " " 
								<< y_scan << " " 
								<< mean
								<< endl;                         
						}
						cout<<" ++++MEAN++++    =    " << mean <<endl;
						u=u+1; //GS
						
//						if( filter_number==2&&w>0) range=new_range;//GS!
						range=new_range;//GS! Oct22
						cout<<" range =    " << range <<endl;
						if( u==range )
						{  
							if( filter_number==2) w=w+1;
							if( filter_number==0)
							{ 
								if( v==0)
								{ 
									v=v+1; //GS for pure_once only
									for( int m = 0; m < n_data; m++ )
									{                        
										buffer_max_time_once[m]= buffer_max_time[m];
										buffer_max_ch0_once[m]= buffer_max_ch0[m];
										buffer_max_ch1_once[m]= buffer_max_ch1[m];
										buffer_max_ch2_once[m]= buffer_max_ch2[m];

									}                        
									buffer_max_x_scan_once= buffer_max_x_scan;
									buffer_max_y_scan_once= buffer_max_y_scan;                       
								}
								else
								{
									for( int m = 0; m < n_data; m++ )
									{
										buffer_max_time[m] = buffer_max_time_once[m];
										buffer_max_ch0[m]  = buffer_max_ch0_once[m];
										buffer_max_ch1[m]  = buffer_max_ch1_once[m];
										buffer_max_ch2[m]  = buffer_max_ch2_once[m];
									}                         
									buffer_max_x_scan = buffer_max_x_scan_once;
									buffer_max_y_scan = buffer_max_y_scan_once;
								}
							} 			   

							for( int k = 0; k < n_data; k++ )
							{
								if( matrixMode )
								{                  
									out << buffer_max_time[k] << " "
										<< calib.adcValue(iadc,0,buffer_max_ch0[k]) << " "
										<< calib.adcValue(iadc,1,buffer_max_ch1[k]) << " "
										//                                  << calib.adcValue(iadc,2,buffer_max_ch2[k]) << " "//temp
										//                                         << calib.adcValue(iadc,7,buffer_ch7[k]) << " "
										<< filter_number << " " //to replace the temp value
										<< filter_number << " "
										<< xb_scan << " " 
										<< yb_scan 
										<< endl;  

									out2 << buffer_max_time[k] << " "
										<< calib.adcValue(iadc,0,buffer_max_ch0[k]) << " "
										<< calib.adcValue(iadc,1,buffer_max_ch1[k]) << " "
										<< calib.adcValue(iadc,2,buffer_max_ch2[k]) << " "//temp
										//                                         << calib.adcValue(iadc,7,buffer_ch7[k]) << " "
										<< filter_number << " "
										<< xb_scan << " " 
										<< yb_scan << " "
										<< buffer_max_x_scan << " " 
										<< buffer_max_y_scan << " " 
										<< mean_max
										<< endl;                         


								}
								else
								{
									out << buffer_time[k] << " "
										<< calib.adcValue(iadc,0,buffer_ch0[k]) << " "
										<< calib.adcValue(iadc,1,buffer_ch1[k]) << " "
										//                                   << calib.adcValue(iadc,2,buffer_ch2[k]) << " "//temp
										//                                     << calib.adcValue(iadc,7,buffer_ch7[k]) << " "
										<< filter_number
										<< endl;
								}
								if(f0>0) f2=f2+1;
								f0=f0+1;
							}   
							for( int m = 0; m < n_data; m++ )
							{
								buffer_max_time[m]= 0;
								buffer_max_ch0[m]= 0;
								buffer_max_ch1[m]= 0;
								//                       buffer_max_ch2[m]= 0;
							}  
							u=0; 
							mean_max=0;
				//			if( filter_number==2) range=new_range;//GS  
							range=new_range;//GS  Oct22

							fstream out_filter;
							out_filter.open("/tmp/filter.dat",std::ios::out);
							out_filter << 3 << " " << buffer_max_x_scan << " " << buffer_max_y_scan << endl;
							out_filter.close();
						}                         

					}
				}   
			}       
			//Pure beam second, GS

			else 
			{
				for( int k = 0; k < n_data; k++ )
				{

					out << buffer_max_time_once[k] << " "
						<< calib.adcValue(iadc,0,buffer_max_ch0_once[k]) << " "
						<< calib.adcValue(iadc,1,buffer_max_ch1_once[k]) << " "
						//                                  << calib.adcValue(iadc,2,buffer_max_ch2[k]) << " "//temp
						//                                         << calib.adcValue(iadc,7,buffer_ch7[k]) << " "
						<< filter_number << " " //to replace the temp value
						<< filter_number << " " 
						<< xb_scan << " " 
						<< yb_scan 
						<< endl;

				}
				f0=f0+1;
			}




			//  }
			if( filter_change )
			{
				fstream out_filter;

				out_filter.open("/tmp/filter.dat",std::ios::out);
				out_filter << -1 << " " << buffer_max_x_scan << " " << buffer_max_y_scan << endl;
				out_filter.close();

			}
			else
				n_data = 0; // no change in filter.dat

	}

	out.close();
	out2.close();  
	outmax.close();                 
	outdiodenscan.close();




}
catch (...)
{
	std::cerr<<" *** exception was thrown, something is wrong"<<std::endl;
	exit(EXIT_FAILURE);
}

//calib.writeAdcFile(iadc,"calib_adc01.dat");

return EXIT_FAILURE;

}
