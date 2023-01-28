/*
  ADS1232.h - library for TI ADS1232
  Created by John Sartzetakis, Jan 2019
  Released into the public domain.
  https://gitlab.com/jousis/ads1232-library

  ADS1232
  http://www.ti.com/lit/ds/symlink/ads1232.pdf
  24-Bit ADC
  
  Smoothing data functions taken from HX711_ADC by Olav Kallhovd
  https://github.com/olkal/HX711_ADC

  Also check out ADS123X library by Hamid Saffari
  https://github.com/HamidSaffari/ADS123X  
*/

#ifndef ADS1232_h
#define ADS1232_h

#include <Arduino.h>

//For 80SPS, 20 samples (SPS/4) are enough for impressive +/-0.02g stability (lab conditions) and extremely fast response. From 100g to 0 you will need 20*12,5ms = 250ms which is barely noticable.
//10SPS is slow by default so be careful when enabling smoothing because you will lose responsiveness in sudden weight changes.
//For example, if you suddenly remove 100g from the scale and you have DATA_SET_MIN sampling with 10sps you will need 5 samples to go to 0 = 0.5s which is noticable by the user.
//Please note that the sample size will not compensate for load cell drift.
#define DATA_SET_MAX 22 // best for 80SPS // 20 samples + 1 ignore low + 1 ignore high
#define DATA_SET_MIN 5 // best for 10SPS // 3 samples + 1 ignore low + 1 ignore high




class ADS1232

{

  public:
    ADS1232(uint8_t pdwn, uint8_t sclk, uint8_t dout, uint8_t a0, uint8_t spd, uint8_t gain1, uint8_t gain0, uint8_t temp); // constructor with full control
    ADS1232(uint8_t pdwn, uint8_t sclk, uint8_t dout); // basic constructor
    void begin(uint8_t channel, uint8_t gain, uint8_t speed);
    void setGain(uint8_t gain = 128, bool calibrate = true); // 1/2/64/128 , default 128
    void setSpeed(uint8_t sps = 10, bool calibrate = true); // 10 or 80 sps , default 10
    uint8_t getSpeed(); // 10 or 80 sps , default 10
    void setChannel(uint8_t channel = 0, bool calibrate = true); // 0 or 1 , default = 0
    //void readInternalTemp(); //Not Implemented , check datasheet
    void powerOn();
    void powerOff();
    void calibrateADC(); //this is the internal calibration method of the ADC , not the calculation of the calFactor
    void tare(byte type, bool calibrate); //type 0 = read one value & tare, 1= tare using last value, 2=read 5 samples and tare and also optionally calibrate
    float calFactor; // this is the number that will help us translate voltage read from the ADC to units (grams,whatever).
    void setCalFactor(float cal = 1.0);    
    void setMinDiff(int32_t diff, int32_t threshold);  // this is a useful trick to keep the scale stable when around zero (if grams are around 0). This is applied after read raw and you should select proper value for your load cell.
    void setSmoothing(bool enable = true); //if enabled, everything is put in an table and at every read the largest and the lowest value is removed. The "value" returned is then the samples/table size.
    bool getSmoothing();
    void setDataSetSize(uint8_t datasetsize);
    uint8_t DATA_SET = DATA_SET_MIN; //DATA_SET can be set at any time up to DATA_SET_MAX. 
                                  //The actual memory allocation of the array however is static. If you have issues with RAM, change DATA_SET_MAX

    int32_t readRaw(uint8_t samples); // if smoothing is disabled, returns an average of samples chosen, if smoothing is enabled reads samples chosen and returns the smoothed value.
    double readUnits(uint8_t samples); // returns a final weight value (tareOffset + calFactor taken into account) after readRaw(samples) with/without smoothing.
    int actualSPS;

    
  protected:
//    uint8_t shiftInSlow(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder);


    void initConfig();
    bool isReady(); // checks the DOUT pin
    bool safeWait(); // returns false if waiting more than 1000 millis
    int32_t readADC(); //gets and returns a single ADC value without any processing
    void resetSmoothing(int32_t value); //replaces all values of the smoothing dataset array with value
    int32_t getSmoothedValue();
    uint8_t pdwnPin; // keep HIGH for power on, LOW for power off
    uint8_t sclkPin; // not regular SPI, read datasheet
    uint8_t doutPin; // not regular SPI, read datasheet
    uint8_t spdPin; // LOW = 10SPS , HIGH = 80SPS
    uint8_t gain1Pin; // 0|0 = 1 , 0|1 = 2 , 1|0 = 64 , 1|1 = 128
    uint8_t gain0Pin; // in our case, anything lower than 128 is not worth it. Keep both gain0/gain1 HIGH
    uint8_t tempPin; // if HIGH, AINP/AINN can be used to measure temp from internal temp diodes (datasheet, page 13)
    uint8_t a0Pin; //Select output, LOW for AIN1.   

    uint8_t adcSpeed;
    uint8_t adcGain;
    uint8_t adcTemp;
    uint8_t adcChannel;

    uint8_t readsPerSecond = 0;
    int lastRateCheck = 0; //millis before last calculation of readspersecond
    int readTimeout = 0; //millis before last calculation of readspersecond


    int32_t tareOffset;

    bool smoothing;
    int readIndex = 0;
    int32_t dataSampleSet[DATA_SET_MAX+1];

    
    int32_t lastAdcValue; //this is the absolute last adcvalue we read
    float ignoreDiff;
    float ignoreDiffThreshold;

  
};

#endif
