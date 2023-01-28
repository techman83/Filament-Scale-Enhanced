/*  
  https://gitlab.com/jousis/espresso-scale
*/

#include "SCALE.h"
//
//#define SERIAL_IF
//#define DEBUG

SCALE::SCALE(uint8_t pdwn, uint8_t sclk, uint8_t dout) : adc( new ADS1232(pdwn, sclk, dout))
{
  
} 

SCALE::SCALE(uint8_t pdwn, uint8_t sclk, uint8_t dout, uint8_t a0, uint8_t spd, uint8_t gain1, uint8_t gain0, uint8_t temp) : adc ( new ADS1232(pdwn, sclk, dout, a0, spd, gain1, gain0, temp))
{
  
}



void SCALE::begin(uint8_t channel, uint8_t gain, uint8_t speed) 
{
  adc->begin(channel,gain,speed);  
}




void SCALE::powerOn()
{
  adc->powerOn();
}

void SCALE::powerOff()
{
  adc->powerOff();
}


void SCALE::setSpeed(uint8_t sps) //10 or 80 sps , default 10
{
  adc->setSpeed(sps);
}
uint8_t SCALE::getSpeed()
{
  return(adc->getSpeed());
}


int SCALE::getAdcActualSPS()
{  
  return adc->actualSPS;
}

void SCALE::setGain(uint8_t gain)
{  
  adc->setGain(gain);
}


void SCALE::setSensitivity(uint8_t sensitivity)
{
  int datasetsize = 1;
  if (sensitivity <= 0) { sensitivity = 1; }
  datasetsize = (int) DATA_SET_MAX/sensitivity;
  adc->setDataSetSize(datasetsize);
}

void SCALE::setSmoothing(uint8_t smoothing) {
    adc->setSmoothing(smoothing == 1);    
}

uint8_t SCALE::getSmoothing() {
  if (adc->getSmoothing()) {
    return 1;    
  } else {
    return 0;
  }
}


bool  SCALE::tare(byte type, bool saveWeight, bool autoTare, bool calibrate) 
{ 
  ESP_LOGV("SCALE", "will tare adc with type/save %d/%d",type,saveWeight);
  if (lastUnitsRead > 0 && saveWeight) {
    lastTareWeight = lastUnitsRead;
    lastTareWeightRounded = roundToDecimal(lastUnitsRead,decimalDigits);
  }
  
  adc->tare(type,calibrate);
  lastUnitsRead = 0;
  hasSettled = true;
  hasSettledQuick = true;  
  stableWeightCounter=65535; //we could find the proper max value as we do in readUnits, but no need to do it. Just set an arbitary huge number.
  stableWeightCounterQuick = 65535;
  lastStableWeight = 0;
  if (!autoTare) {
    // autoTareUsed = false;  // enable this line if autotare should be allowed after each manual tare
    zeroTrackingUntil = millis() + 1000; //enable zeroTracking for 1s
  }
  return true;
}

double SCALE::roundToDecimal(double value, int dec)
{
  double mlt = powf( 10.0f, dec );
  value = roundf( value * mlt ) / mlt;
  return value;
}

int32_t  SCALE::readRaw(uint8_t samples){
  return adc->readRaw(samples);
}


bool SCALE::gethasSettled(){
  return hasSettled;
}

double SCALE::readUnits(uint8_t samples)
{
  double finalUnits = 0.0;
  double units = 0.0;
  uint8_t adcSpeed = 10;
  double absUnits = 0.0;
  
  if (samples <=0) {
    samples=1;
  }
  units = adc->readUnits(samples);  
  absUnits = fabs(units);
  adcSpeed = adc->getSpeed();
  uint16_t sampleSize = adcSpeed*stableWeightSampleSizeMultiplier;
  uint16_t sampleSizeQuick = adcSpeed*stableWeightSampleSizeMultiplierQuick;
  
  if (fabs(absUnits - fabs(lastUnitsRead)) <= stableWeightDiff) {
    //ok...stable weight, increase counter and change hasSettled flag
    if (stableWeightCounterQuick < sampleSizeQuick) {
      stableWeightCounterQuick++;
    }
    if (stableWeightCounter < sampleSize) { 
      stableWeightCounter++;
    }
    if (stableWeightCounterQuick >= sampleSizeQuick) {
      if (!hasSettledQuick) {
        hasSettledQuick = true;   
      }
    }
    if (stableWeightCounter >= sampleSize) {
      if (!hasSettled) {
        //1st time
        hasSettled = true;
        lastStableWeight = units;        
      }
    }
  } else {
    //oops...
    hasSettled = false;
    hasSettledQuick = false;   
    stableWeightCounter=0;
    stableWeightCounterQuick = 0;
    lastStableWeight = units + fakeStabilityRange*2;
  }
  
  if (!calibrationMode && autoTare && !autoTareUsed && (absUnits > autoTareMinWeight)) {
    if (hasSettled) {
      ESP_LOGD("SCALE", "Auto tare is enabled, taring...");   
         
      tare(0,true,true,false); //do a quick tare, save the weight to history, but without adc calibration
      
      
      autoTareUsed = true;
      units = 0.0;
    } else {      
      //we are waiting for hasSettled to do auto tare, reset timer.
      // resetTimer();
    }
  }

  if (autoTareNegative && units < 0) {
      ESP_LOGD("SCALE", "Auto tare on negative value is enabled is enabled, taring...");   
      tare(0,false,true,false);
  }
  
  finalUnits = units;
  if (fakeStabilityRange > 0) {
    if (fabs(absUnits - fabs(lastFakeRead)) <= fakeStabilityRange) {
      if ((millis() - lastFakeRefresh) < fakeDisplayLimit*1000){
        //we are fine...show the fake
        finalUnits = lastFakeRead;
      } else {
        //nope...not any more
        lastFakeRead = units;
        lastFakeRefresh = millis();
      }
    } else {
      lastFakeRead = units;
      lastFakeRefresh = millis();
    }
  }
  lastUnitsRead = units;
  if (!calibrationMode) {
    // updateTimer();
  }

  bool zTrTare = false;
  if (!calibrationMode && hasSettled && absUnits > 0 && (absUnits < zeroTracking)){
    zTrTare = true;
  } else if (zeroTrackingUntil > 0 && (millis() < zeroTrackingUntil)) {
    zTrTare = true;    
  }

  if (zTrTare) {
    ESP_LOGD("SCALE","Zero tracking is enabled, taring... units = %f",units);
    tare(1,false,true,false);
    units = 0.0;
    finalUnits = 0.0;
  }


//  if (fabs(units) <= zeroRange) { units = 0.00; }  
  if (fabs(roc) <= zeroRange) { roc = 0.00; }
//  if (fabs(fabs(units)-fabs(lastStableWeight)) <= fakeStabilityRange) { units = lastStableWeight; }
  if (fabs(finalUnits) <= zeroRange) { finalUnits = 0.00; }
  finalUnits = roundToDecimal(finalUnits,decimalDigits);




  
  uint32_t rocMillis = millis();  
    roc = (finalUnits-lastUnits)*1000/(rocMillis - rocLastCheck);  
    rocLastCheck = rocMillis;

  
  lastUnits = finalUnits;
  return finalUnits;
}





void SCALE::calibrateADC() 
{  
  adc->calibrateADC();
}






void SCALE::calibrate(float targetWeight, u_int32_t maxMillis, float targetDiff) 
{  
  calibrationMode = true;
  setCalibrationStatus(calibrationStatus::START);  
  ESP_LOGI("SCALE","calibrating...");
  for (uint8_t i=0;i<10;i++){
    delay(100);
  }
  for (uint8_t i=0;i<5;i++){
    tare(0,false,true,true);
    delay(100);
  }
  uint32_t calibrationStartTime = millis();
  float weight = 0.0;
  float oldstableWeightDiff = stableWeightDiff;
  //increase stableWeightDiff to 1g  
  stableWeightDiff = 1.0;
  //loop until settled
  uint32_t elapsedTime = millis() - calibrationStartTime;
  setCalibrationStatus(calibrationStatus::PLACE);  
  while (!hasSettled || abs(weight) < 10.0) {
    weight = readUnits(1);
    ESP_LOGD("SCALE","weight:%f",weight); 
    elapsedTime = millis() - calibrationStartTime;
    if (elapsedTime > maxMillis) {
       ESP_LOGE("SCALE","calibration failed");
       setCalibrationStatus(calibrationStatus::ERROR);  
      return;
    }
  }
   ESP_LOGD("SCALE","calibration stage 1");
   setCalibrationStatus(calibrationStatus::STAGE_1);
  float switchModeThreshold = targetWeight*0.05; //5%
  bool initAutoCalibrationComplete = false;
  bool fineTuneAutoCalibrationComplete = false;
  bool slowTuneAutoCalibrationComplete = false;
  uint8_t oldSpeed = adc->getSpeed();
  bool oldSmoothing = adc->getSmoothing();
  adc->setSmoothing(false);  
  adc->setSpeed(80);
  //let's get close enough very fast
  float newCalFactor = adc->calFactor;
  while (!initAutoCalibrationComplete && elapsedTime<maxMillis) {
    weight = readUnits(1);
    if ( fabs(weight-targetWeight) <= switchModeThreshold) {
      //continue in low speed
      initAutoCalibrationComplete = true;
    } else {
      if (weight > targetWeight) {
        //increase calfactor
        newCalFactor = newCalFactor + 50;
      } else {
        //decrease calfactor
        newCalFactor = newCalFactor - 50;
      }    
      adc->setCalFactor(newCalFactor);
      ESP_LOGD("SCALE","new calfactor/weight %f/%f",newCalFactor,weight);
    }
    elapsedTime = millis() - calibrationStartTime;  
  }
  ESP_LOGD("SCALE","stage 1 completed...");
  switchModeThreshold = targetWeight*0.01; //1%
  ESP_LOGD("SCALE","calibration stage 2");
   setCalibrationStatus(calibrationStatus::STAGE_2);
  while (!fineTuneAutoCalibrationComplete && elapsedTime<maxMillis) {
    weight = readUnits(1);
    if ( fabs(weight-targetWeight) <= switchModeThreshold) {
      fineTuneAutoCalibrationComplete = true;
    } else {
      if (weight > targetWeight) {
        //increase calfactor
        newCalFactor = newCalFactor + 1;
      } else {
        //decrease calfactor
        newCalFactor = newCalFactor - 1;
      }    
      adc->setCalFactor(newCalFactor);
      ESP_LOGD("SCALE","new calfactor/weight %f/%f",newCalFactor,weight);
    }
    elapsedTime = millis() - calibrationStartTime;  
  }
  stableWeightDiff = oldstableWeightDiff;
  ESP_LOGD("SCALE","stage 2 completed...");

  bool above = false;
  bool below = false;
  //finally, switch to 10SPS and do the final approach
  ESP_LOGD("SCALE","calibration stage 3");
   setCalibrationStatus(calibrationStatus::STAGE_3);
   ESP_LOGD("SCALE","swithing to 10SPS");
  adc->setSpeed(10);
  bool resetStableWeightCounter = true;
  while (!slowTuneAutoCalibrationComplete && elapsedTime<maxMillis) {
    weight = readUnits(1);
    if ( (fabs(weight-targetWeight) <= targetDiff) && above && below) {
      //almost done...wait until settled
      if (resetStableWeightCounter) {
        hasSettled = false;
        resetStableWeightCounter = false;
        stableWeightCounter = 0;
        ESP_LOGD("SCALE","Almost there...reseting hasSettled status");
      }
      if (hasSettled) { slowTuneAutoCalibrationComplete = true; }
    } else {
      if (weight > targetWeight) {
        //increase calfactor
        above = true;
        newCalFactor = newCalFactor + finetuneCalibrationAdj;
      } else {
        //decrease calfactor
        below=true;
        newCalFactor = newCalFactor - finetuneCalibrationAdj;
      }    
      adc->setCalFactor(newCalFactor);
      ESP_LOGD("SCALE","new calfactor/weight %f/%f",newCalFactor,weight);
    }
    elapsedTime = millis() - calibrationStartTime;  
  }

  if (elapsedTime>=maxMillis){
     ESP_LOGE("SCALE","calibration timed out...please increase time");
     setCalibrationStatus(calibrationStatus::ERROR);
     return;
  } else {
    ESP_LOGI("SCALE","final calibration completed...");
  }
  adc->setSpeed(oldSpeed);  
  adc->setSmoothing(oldSmoothing);  
  calibrationMode = false;
  autoTareUsed = true;
  setCalibrationStatus(calibrationStatus::FINISHED);
  ESP_LOGD("SCALE","Done");
}

void SCALE::setCalFactor(float calFactor)
{
  adc->setCalFactor(calFactor);
}


float SCALE::getCalFactor()
{
  return(adc->calFactor);
}


void SCALE::setCalibrationStatus(calibrationStatus loc){
   _calibrationStatus=loc;
}


calibrationStatus SCALE::getCalibrationStatus(){
  return _calibrationStatus;
}