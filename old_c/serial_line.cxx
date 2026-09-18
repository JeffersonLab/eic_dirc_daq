#include "serial_line.hxx"

//-------------------------------------------------------------------
SerialLine::SerialLine(std::string line, int verbosity, device dev)
{
  fverbosity = verbosity;
  fline      = line;

  // O_RDWR   read/write
  // O_NOCTTY don't be the controlling terminal
  // O_NDELAY don't care about DCD and wait until chars are present for read
  fd = open(line.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
  //fd = open(line.c_str(), O_RDWR | O_NOCTTY);

  
  if (fverbosity>0)
    {
      std::cout<<" SerialLine: opened serial line "<<line
       <<" with file handle "<<fd<<std::endl;
    }
  
  if (fd == -1) 
    {
      std::cerr<<" *** open_port: Unable to open "<<line<<std::endl;
      exit(EXIT_FAILURE);
    }
  else 
    {
      fcntl(fd, F_SETFL, 0);
    }

  struct termios options;


  tcgetattr(fd, &options);  // get current options
  foptionsOld = options;    // save them for later


  switch (dev)
    {
    case LSTEP:
    
      // 8N2 at 57600 in canonical mode

      cfsetispeed(&options, B57600);
      cfsetospeed(&options, B57600);
      // set parameters 
      // see documentation in ./doc/serial/serial.html
      options.c_cflag &= ~PARENB; // do not enable parity bit
      options.c_cflag |= CSTOPB;  // two stop bits
      options.c_cflag &= ~CSIZE;  // no bit mask for data bits
      options.c_cflag |= CS8;     // 8 data bits
      options.c_cflag |= CRTSCTS; // rts/cts hand shake
      options.c_cflag     |= (CLOCAL | CREAD); 
      options.c_lflag     |= (ICANON | ECHO |  ECHOE); // canonical input
      options.c_oflag     = 0;
      //options.c_cc[VMIN]  = 0;
      //options.c_cc[VTIME] = 2;
      options.c_cc[VMIN]  = 1;//GS
      options.c_cc[VTIME] = 0;//GS 21.04.2021

      break;

    case PEAKTECH:

      // 7E1 at 1200 in modem mode
      
      options.c_cflag=0;
      cfsetispeed(&options, B1200);
      cfsetospeed(&options, B1200);
      options.c_cflag |= CREAD;
      options.c_cflag |= PARENB;
      options.c_cflag |= CS7;
      options.c_cflag |= CRTSCTS;
      options.c_cc[VMIN]  = 0;
      options.c_cc[VTIME] = 2;
      options.c_lflag     &= ~(ICANON | ECHO |  ECHOE |  ISIG); // raw (modem)
      options.c_oflag     &= ~OPOST;                            // modem mode
      options.c_cc[VMIN]  = 0;
      options.c_cc[VTIME] = 1; //unit=1/10 seconds
      options.c_iflag = IGNBRK | IGNPAR;
      break;
    
    case CTLAB:

      // 8N1 at B38400 in canonical mode
      
      // options.c_cflag=0; // never initialize cflag
      cfsetispeed(&options, B38400);
      cfsetospeed(&options, B38400);
      options.c_cflag |= CREAD;  // should be always set
      options.c_cflag |= CLOCAL; // should be always set

      // parity 8N1
      options.c_cflag &= ~PARENB;
      options.c_cflag &= ~CSTOPB;
      options.c_cflag &= ~CSIZE;  // Mask the character size bits
      options.c_cflag |= CS8;     // 8 bit
      
      options.c_cflag &= ~CRTSCTS;                // disable hardware flow control

      //options.c_lflag |= (ICANON | ECHO | ECHOE); // canonical input (line oriented with CR | LF)
      options.c_lflag &= ~(ICANON | ECHO |  ECHOE |  ISIG); // raw (modem)

      options.c_iflag &= ~(IXON | IXOFF | IXANY); // without-> no return from read
      //options.c_iflag = IGNBRK | IGNPAR;

      options.c_oflag &= ~OPOST;  // raw mode, no processed output

      //options.c_cc[VMIN]  = 0;
      //options.c_cc[VTIME] = 1; //unit=1/10 seconds
      options.c_cc[VMIN]  = 1; //before 1 GS
      options.c_cc[VTIME] = 0; //unit=1/10 seconds
      break;
      
//    case HAMEG:

      
//      // 8N1 at 19200 in canonical mode
 //     // 8N1 at 115200 in canonical mode
      
//      options.c_cflag=0;
//      cfsetispeed(&options, B19200);
//      cfsetospeed(&options, B19200);
 //     options.c_cflag |= CREAD;
 //     options.c_cflag &= ~PARENB;
 //     options.c_cflag |= CS8;
 //     options.c_iflag &= ~(IXON | IXOFF | IXANY); // without-> no return from read
  //    options.c_cflag &= ~CRTSCTS;                //###
 //     options.c_cc[VMIN]  = 0;
 //     options.c_cflag     &= ~CLOCAL; 
  //    options.c_cc[VTIME] = 2;
  //    options.c_lflag &= ~(ICANON | ECHO |  ECHOE |  ISIG); // raw (modem)
  //    options.c_oflag &= ~OPOST;                            // modem mode
  //    options.c_cc[VMIN]  = 0;
  //    options.c_cc[VTIME] = 1; //unit=1/10 seconds
  //    options.c_iflag = IGNBRK | IGNPAR;
  //    break;
  
    








    default:

      std::cerr<<" *** SerialLine: device not known.\n";
      // close interface
      if (fd!=-1)
{
  tcsetattr(fd, TCSANOW, &foptionsOld);
  close(fd);
}
      exit(EXIT_FAILURE);
      break;
    }


  /* set the options */
  tcsetattr(fd, TCSANOW, &options);
 
}

SerialLine::~SerialLine()
{
  // close interface
  if (fd!=-1)
    {
      tcsetattr(fd, TCSANOW, &foptionsOld);
      close(fd);
    }

  if (fverbosity>0)
    {
      std::cout<<" SerialLine: closed serial line "<<fline
       <<" with file handle "<<fd<<std::endl;
    }
}
