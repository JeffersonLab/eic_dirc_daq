#ifndef CALIBCTLAB_H
#define CALIBCTLAB_H 1

/*! 
  \class CalibCtLab calib_ct_lab.h
  \brief Calibration of max. 8 DAC and ADC within the c't lab project.
  
*/


class CalibCtLab
{
 private:

  vector<string> m_filename_dac;       //!< The filename of calibration data
  vector<bool>   m_filename_dac_exist; //!< Flag for read 
  vector<string> m_filename_adc;       //!< The filename of calibration data
  vector<bool>   m_filename_adc_exist; //!< Flag for read 

  double m_voltSetDac[8][1000];            /*!< volt set array for max 8 cards 
and max 1000 values. */
  double m_voltReadDac[8][8][1000];        /*!< volt set array for max 8 cards, 
8 channels, and max 1000 values. */
  double m_voltSetAdc[8][1000];            /*!< volt set array for max 8 cards 
and max 1000 values. */
  double m_voltReadAdc[8][8][1000];        /*!< volt set array for max 8 cards, 
8 channels, and max 1000 values. */

 public:

  CalibCtLab();                   //!< Empty constructor.

  /*! \brief Read calibration data file for DAC

  The file is expected to have a column of set voltage values and 
  8 columns of read out values covering the range between -10 to 10 Volts.
  Columns which are not calibrated consist of 999 values.

    \param icard The card for which the file is read.
    \param filename The filename to read.
    \return Error code: 0=success, 1=file not found.
  */
  int readDacFile(int icard, string filename);

  /*! \brief Write ADC calibration to file
    \param icard The card for which the file is written.
    \param filename The filename to write.
  */
  void writeAdcFile(int icard, string filename);

  /*! \brief Read ADC calibration from file
    \param icard The card for which the file is read.
    \param filename The filename to read.
  */
  void readAdcFile(int icard, string filename);

  /*! \brief Value to set for wanted voltage.
    \param icard The card number [0-7].
    \param ichannel The channel number [0-7].
    \param value The wanted voltage in Volts.
    \return The voltage to set in the DAC to get back the wanted voltage.
  */
  double dacSetValue(int icard, int ichannel, double value);

  /*! \brief Calibrated ADC value.
    \param icard The card number [0-7].
    \param ichannel The channel number [0-7].
    \param value The uncalibrated value.
    \return Calibrated value.
  */
  double adcValue(int icard, int ichannel, double value);

  /*! \brief Value to set for wanted voltage.
    \param icard The card number [0-7].
    \param ichannel The channel number [0-7].
    \param volt_set Voltage applied to input.
    \param volt_read Raw voltage read out.
  */
  void setAdcCalibPara(int icard, int ichannel, double volt_set, double volt_read);

};


#endif
