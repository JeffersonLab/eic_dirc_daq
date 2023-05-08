#ifndef __SERIAL_LINE_HXX__
#define __SERIAL_LINE_HXX__

#include <cstdlib>
//#include <cstdio>
//#include <cmath>
//#include <string>
#include <iostream> // cout cerr cin
//#include <fstream>
//#include <ctime>
//#include <list>

#include <unistd.h>  /* UNIX standard function definitions */
#include <fcntl.h>   /* File control definitions */
#include <errno.h>   /* Error number definitions */
#include <termios.h> /* POSIX terminal control definitions */

#include <string>

/*! 
  \class SerialLine serial_line.hxx
  \brief Serial line control.

  Administration of a serial line
*/



//! Enumeration of devices.
/*! This parameter determines the setting of the serial line
  and the way of reading the serial line. */
enum device {
  LSTEP,         /*!< LSTEP controller card. */  
  PEAKTECH,      /*!< PeakTech451RS Multimeter (DMM). */
  CTLAB          /*!< C't Lab interfaces */
};

class SerialLine
{
public:
  /*! Non empty constructor
    Explanation of serial line parameters can be found 
    <A HREF="../serial/serial.html">here.</A>
    \param line string with name eg. /dev/ttyS0 for controller
    \param verbosity verbosity 0=quite
    \param dev (LSTEP,PEAKTECH)
  */
  SerialLine(std::string line,
     int         verbosity=0,
     device      dev=LSTEP);
  
  
  //! Empty destructor
  ~SerialLine();
  
  /*! \brief File handle
    \return file handle for serial line
  */
  int fileHandle(){return fd;}
  
private:
  int            fd;                           //!< File descriptor for interface.
  struct termios foptionsOld;                  //!< Old status of interface.
  int            fverbosity;                   //!< Verbosity 0=quiet.
  std::string    fline;                        //!< Name of interface.
};
#endif
