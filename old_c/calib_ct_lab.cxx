#include <cstdlib>
#include <cstdio>

#include <cmath>
#include <string>
using std::string;

#include <iostream>
using std::cout;
using std::cin;
using std::cerr;
using std::endl;

#include <fstream>
using std::fstream;

#include <ctime>

#include <vector>
using std::vector;

#include "calib_ct_lab.h"


//----------------------------------------------------------------------
CalibCtLab::CalibCtLab()
{
  m_filename_dac.resize(8);
  m_filename_dac_exist.resize(8);
  m_filename_adc.resize(8);
  m_filename_adc_exist.resize(8);

  for (int icard=0; icard<8; icard++)
    {
      m_filename_dac[icard] = "none";
      m_filename_dac_exist[icard] = false;
      m_filename_adc[icard] = "none";
      m_filename_adc_exist[icard] = false;
    }

  for (int icard=0; icard<8; icard++)
    {
      for (int n=0; n<391; n++)
{
  m_voltSetAdc[icard][n] = -9.5 + 0.05*n;
  for (int ich=0; ich<8; ich++)
    {
      m_voltReadAdc[icard][ich][n]=999;
    }
} 
    }
}
//----------------------------------------------------------------------
int CalibCtLab::readDacFile(int icard, string filename)
{
  fstream in;
  in.open(filename.c_str(),std::ios::in);

  if (!in) 
    {
      return 1; // file doesn't exist
    }
  else
    {
      int n=0;
      while(in)
{
  in>>m_voltSetDac[icard][n];
  for (int ich=0; ich<8; ich++)
    {
      in>>m_voltReadDac[icard][ich][n];
    }
  n++;

}
      m_filename_dac_exist[icard]=true;

    }
  in.close();
  return 0;


}
//----------------------------------------------------------------------
void CalibCtLab::writeAdcFile(int icard, string filename)
{
  fstream out;
  out.open(filename.c_str(),std::ios::out | std::ios::trunc);

  if (!out) 
    {
      cerr<<" *** CalibCtLab::writeAdcFile: cannot open file, exit."<<endl;
      exit(EXIT_FAILURE);
    }
  else
    {
      for (int n=0; n<391; n++)
{
  out<<m_voltSetAdc[icard][n]<<" ";
  for (int ich=0; ich<8; ich++)
    {
      out<<m_voltReadAdc[icard][ich][n]<<" ";
    }
  out<<endl;
}
    }
  out.close();

}
//----------------------------------------------------------------------
void CalibCtLab::readAdcFile(int icard, string filename)
{
  fstream in;
  in.open(filename.c_str(),std::ios::in);

  if (!in) 
    {
      cerr<<" *** CalibCtLab::readAdcFile: cannot open file, exit."<<endl;
      exit(EXIT_FAILURE);
    }
  else
    {
      for (int n=0; n<391; n++)
{
  in>>m_voltSetAdc[icard][n];
  for (int ich=0; ich<8; ich++)
    {
      in>>m_voltReadAdc[icard][ich][n];
    }
}
      m_filename_adc[icard]=filename;
      m_filename_adc_exist[icard]=true;
    }
  in.close();
}
//----------------------------------------------------------------------
double CalibCtLab::dacSetValue(int icard, int ichannel, double value)
{
  double frac,volt_diff,volt_diff1,volt_diff2;


  if (m_filename_dac_exist[icard])
    {
        if (value<-9.5)
{
  cout<<" CalibCtLab::dacSetValue: value<-9.6 Volt. "
      <<" Return hardware calibration"<<endl;
  return value;
}
        if (value>10.0)
{
  cout<<" CalibCtLab::dacSetValue: value>10 Volt. "
      <<" Return hardware calibration"<<endl;
  return value;
}
      if (m_voltReadDac[icard][ichannel][0]>900)
{
  cout<<" CalibCtLab::dacSetValue: channel not calibrated "
      <<" Return hardware calibration"<<endl;
  return value;
}
      for (int n=0; n<1000; n++)
{

  //cout<<m_voltSet[icard][n]<<" "<<value<<endl;

  if (value >= m_voltSetDac[icard][n] && value <= m_voltSetDac[icard][n+1])
    {
      //cout<<" found"<<endl;

      frac = (m_voltSetDac[icard][n+1]-value)
/ (m_voltSetDac[icard][n+1]-m_voltSetDac[icard][n]);
      volt_diff1 = 
m_voltReadDac[icard][ichannel][n  ]-m_voltSetDac[icard][n  ];
      volt_diff2 = 
m_voltReadDac[icard][ichannel][n+1]-m_voltSetDac[icard][n+1];
      volt_diff = volt_diff1 + frac * (volt_diff2-volt_diff1);

      return value-volt_diff;
    }
}
      cerr<<" *** CalibCtLab::dacSetValue: value not fond. Exit."<<endl;
      exit(EXIT_FAILURE);
    }
  else
    {
      cerr<<" *** CalibCtLab::dacSetValue: no calib data for card. Exit."<<endl;
      exit(EXIT_FAILURE);
    }
}
//----------------------------------------------------------------------
double CalibCtLab::adcValue(int icard, int ichannel, double value)
{
  double frac;
  if (m_filename_adc_exist[icard])
    {
        if (value<-9.5)
{
  cout<<" CalibCtLab::adcValue: value<-9.5 Volt. "
      <<" Return hardware calibration"<<endl;
      return -9.5; // old: value
}
    if (value>9.5) // old: 10
{
  cout<<" CalibCtLab::adcValue: value>9.5 Volt. "
      <<" Return hardware calibration"<<endl;
      return 9.5; // old: value
}
      if (m_voltReadAdc[icard][ichannel][0]>900)
{
  cout<<" CalibCtLab::adcValue: channel not calibrated "
      <<" Return hardware calibration"<<endl;
  return value;
}
      for (int n=0; n<391; n++)
{
  if (value >= m_voltReadAdc[icard][ichannel][n] && 
      value <= m_voltReadAdc[icard][ichannel][n+1])
    {
      frac = (value-m_voltReadAdc[icard][ichannel][n])
/ (m_voltReadAdc[icard][ichannel][n+1]-
   m_voltReadAdc[icard][ichannel][n]);

      //cout<<" n  ,read="<<n<<" "<<m_voltReadAdc[icard][ichannel][n]<<endl;
      //cout<<" n+1,read="<<n<<" "<<m_voltReadAdc[icard][ichannel][n+1]<<endl;
      //cout<<" n  , set="<<n<<" "<<m_voltSetAdc[icard][n]<<endl;
      //cout<<" n+1,read="<<n<<" "<<m_voltSetAdc[icard][n+1]<<endl;
      //cout<<"     frac="<<frac<<endl;

      return m_voltSetAdc[icard][n]+ 
frac*(m_voltSetAdc[icard][n+1]-m_voltSetAdc[icard][n]);
    }
}
      cerr<<" *** CalibCtLab::adcValue: value not fond. Exit."<<endl;
      exit(EXIT_FAILURE);
    }
  else
    {
      cerr<<" *** CalibCtLab::valueToSet: no calib data for card. Exit."<<endl;
      exit(EXIT_FAILURE);      
    }
  return 999;
}
//----------------------------------------------------------------------
void CalibCtLab::setAdcCalibPara(int    icard, 
int    ichannel, 
double volt_set, 
double volt_read)
{
  for (int n=0; n<391; n++)
    {
      if (fabs(volt_set - m_voltSetAdc[icard][n])<0.01)
{
  m_voltReadAdc[icard][ichannel][n]=volt_read;
  return;
}
    }


}
