/*  
  https://gitlab.com/jousis/ads1232-library
*/

#include "ADS1232.h"




ADS1232::ADS1232(uint8_t pdwn, uint8_t sclk, uint8_t dout) //constructor
{   
  pdwnPin = pdwn;
  doutPin = dout;
  sclkPin = sclk;  
  a0Pin = 0;    
  spdPin = 0;
  gain1Pin = 0;
  gain0Pin = 0;
  tempPin = 0;

  initConfig();
} 

ADS1232::ADS1232(uint8_t pdwn, uint8_t sclk, uint8_t dout, uint8_t a0, uint8_t spd, uint8_t gain1, uint8_t gain0, uint8_t temp)
{
  pdwnPin = pdwn;
  doutPin = dout;
  sclkPin = sclk;
  spdPin = spd;
  gain1Pin = gain1;
  gain0Pin = gain0;
  a0Pin = a0;  
  tempPin = temp;

  initConfig();
}


void ADS1232::initConfig() {
  
  adcSpeed = 80;
  adcGain = 128;
  adcChannel = 0;
  adcTemp = 0;
  
  calFactor = 1400.0;
  tareOffset = 0.0;
  ignoreDiff = 0.0;
  ignoreDiffThreshold = 0.0;
  smoothing = true;
}

void ADS1232::begin(uint8_t channel, uint8_t gain, uint8_t speed) 
{
  ESP_LOGV("ADS1232","begin");
  pinMode(pdwnPin, OUTPUT);
  pinMode(doutPin, INPUT_PULLUP);
  pinMode(sclkPin, OUTPUT);

  ESP_LOGV("ADS1232","Poweron");
  powerOn();
  
  ESP_LOGV("ADS1232","SetPins");
  if (a0Pin >0) {
    pinMode(a0Pin, OUTPUT);
  }
  if (spdPin >0) {
    pinMode(spdPin, OUTPUT);
  }
  if (gain1Pin >0) {
    pinMode(gain1Pin, OUTPUT);
  }
  if (gain0Pin >0) {
    pinMode(gain0Pin, OUTPUT);
  }
  if (tempPin >0) {
    pinMode(tempPin, OUTPUT);
    digitalWrite(tempPin,LOW);
  }
  ESP_LOGV("ADS1232","setChannel");
  setChannel(channel,false);
  ESP_LOGV("ADS1232","setGain");
  setGain(gain,false);
  ESP_LOGV("ADS1232","setSpeed");
  setSpeed(speed,false);

  delay(250);
  ESP_LOGV("ADS1232","tare");
  tare(false,true);
}





bool ADS1232::isReady()
{
  return digitalRead(doutPin) == LOW;
}

bool ADS1232::safeWait()
{
  //if you want to implement a more robust and sophisticated timeout, please check out:
  //https://github.com/HamidSaffari/ADS123X    
  uint32_t elapsed;
  elapsed = millis();
  while (!isReady()) {
    if (millis() > elapsed + 2000) {
      //timeout
      ESP_LOGE("ADS1232","Error while waiting for ADC");
      return false;
    }
  }
  return true;    
}

void ADS1232::powerOn()
{
  digitalWrite(pdwnPin, LOW);
  delayMicroseconds(10);
  digitalWrite(pdwnPin, HIGH);
  digitalWrite(sclkPin, LOW);

  
//  while (!isReady()) { };  
  if (!safeWait()) {
    ESP_LOGE("ADS1232","Power on error");
    return;
  }
  calibrateADC();
}

void ADS1232::powerOff()
{
  digitalWrite(pdwnPin, LOW);
  digitalWrite(sclkPin, HIGH);
}

void ADS1232::setGain(uint8_t gain, bool calibrate) // 1/2/64/128 , default 128
{
  uint8_t adcGain1;
  uint8_t adcGain0;
  if(gain == 1) {
    adcGain1 = 0;
    adcGain0 = 0;
  } else if (gain == 2) {
    adcGain1 = 0;
    adcGain0 = 1;
  } else if (gain == 64) {
    adcGain1 = 1;
    adcGain0 = 0;
  } else {
    //default == 128
    adcGain1 = 1;
    adcGain0 = 1;
  }
  if (gain0Pin>0 && gain1Pin>0) {
    digitalWrite(gain1Pin,adcGain1);
    digitalWrite(gain0Pin,adcGain0);
  }
  if (calibrate) { calibrateADC(); }
}


void ADS1232::setSpeed(uint8_t sps, bool calibrate) //10 or 80 sps , default 10
{  
  adcSpeed = sps;    
  if (spdPin > 0 ) {
    if(adcSpeed == 80) {
      digitalWrite(spdPin,HIGH);
    } else {
      digitalWrite(spdPin,LOW);
    }    
  }
  if (calibrate) { calibrateADC(); }
}

uint8_t ADS1232::getSpeed() 
{  
  return adcSpeed;
}

void ADS1232::setChannel(uint8_t channel, bool calibrate) //0 or 1
{  
  if (a0Pin > 0 ) {
      digitalWrite(a0Pin,channel);
  }
  if (calibrate) { calibrateADC(); }
}

void ADS1232::setDataSetSize(uint8_t datasetsize) 
{
  if (datasetsize != DATA_SET) {
    DATA_SET = datasetsize;  
    resetSmoothing(0); 
    ESP_LOGV("ADS1232","dataset size changed to %d",DATA_SET);
  }
}

void ADS1232::setSmoothing(bool enable) {
  smoothing = enable;
  ESP_LOGV("ADS1232","Zeroing out our dataset");
  resetSmoothing(0);
}
bool ADS1232::getSmoothing() {
  return smoothing;
}

void ADS1232::resetSmoothing(int32_t value) {
  if (DATA_SET > DATA_SET_MAX) {
    DATA_SET = DATA_SET_MAX;
  } else if (DATA_SET < DATA_SET_MIN) {
    DATA_SET = DATA_SET_MIN;
  }  
  if (value == 0) {
    //zero is our flag for current value
    value = readADC();
  }
  readIndex = 0;
  for (uint8_t i = 0; i < DATA_SET_MAX+1; i++) {
    dataSampleSet[i]=value;
    ESP_LOGV("ADS1232","->  %d",dataSampleSet[i]);
  }
  ESP_LOGV("ADS1232","->  %d",value);
}

//void ADS1232::readInternalTemp() 
//{  
//  //Not implemented.
//  //If you need to, it is not very hard, check datasheet
//}


void ADS1232::calibrateADC() 
{  
  ESP_LOGV("ADS1232","ADC calibration init...");
  readADC();
  //readADC returns with 25th pulse completed, so immediately continue with 26th
  delayMicroseconds(1);
  digitalWrite(sclkPin, HIGH);  // 26th pulse
  delayMicroseconds(1);  
  digitalWrite(sclkPin, LOW);  // end of 26th
  //actual calibration begins... wait for dout = LOW
  if (!safeWait()) {
    //oops...time out !!!
    ESP_LOGV("ADS1232","ADC calibration error");
    return;
  }
  readADC(); //read once without saving
  ESP_LOGV("ADS1232","ADC calibration done...");
  //all done, ready to read again...
}




void ADS1232::tare(byte type, bool calibrate)
{  
  ESP_LOGI("ADS1232","ADC tare with type %d",type);
  
  if (type == 0) { //quick
    tareOffset = readRaw(1);  
  } else if (type == 1) { //rapid, not so usefull with buttons, usefull only in many successive tares.
        ESP_LOGI("ADS1232","Taring using lastAdcValue= %d",lastAdcValue);
    tareOffset = lastAdcValue;
  } else { //full tare + calibrate
    if (calibrate) {      
      ESP_LOGI("ADS1232","ADC tare + calibrate");
      calibrateADC();
    }
    tareOffset = readRaw(1);    
    tareOffset = readRaw(1);    
    tareOffset = readRaw(1);    
    tareOffset = readRaw(1);    
  }
  ESP_LOGI("ADS1232","New tare offset %d",tareOffset);
  resetSmoothing(tareOffset);
}


void ADS1232::setCalFactor(float cal) 
{
  if (cal > 0) {
    calFactor = cal;    
  } else {
    calFactor = 1.0;
  }
}

void ADS1232::setMinDiff(int32_t diff, int32_t threshold) 
{
  //changes less than abs(diff) when value < tareOffset + threshold will not be accounted.
  //useful when near 0grams, seems more accurate to the user :D 
  ignoreDiff = diff;
  ignoreDiffThreshold = threshold;
}


double ADS1232::readUnits(uint8_t samples) {
  double unitsvalue = 0.0;
  unitsvalue = (double)(readRaw(samples)-tareOffset)/(double)calFactor;
  //Can we go down to 0.001 range or more???
  //Sure, I doubt though it can be useful with 3.3V excitation and normal off the shelf load cells.
  //In that case, do we really need double here? No. You can change it to float.
  
  //Remember that Serial cannot print more than 2 decimal digits.  
  //Uncomment the following for testing
  //unitsvalue = unitsvalue*100.0;  
  //DEBUG_PRINT("Super duper resolution value*100 = ");DEBUG_PRINTLN(unitsvalue);
  if (unitsvalue == -0.00) { unitsvalue = 0.0; }
  return unitsvalue;
}


int32_t ADS1232::getSmoothedValue()
{
  //simple h/l rejection algorithm from the following library
  //https://github.com/olkal/HX711_ADC
  int32_t data = 0;
  int32_t L = 2147483647;
  int32_t H = -2147483648;
  for (int r = 0; r < DATA_SET; r++) {
    if (L > dataSampleSet[r]) L = dataSampleSet[r]; // find lowest value
    if (H < dataSampleSet[r]) H = dataSampleSet[r]; // find highest value
    data += dataSampleSet[r];
  }
  data -= L; //remove lowest value
  data -= H; //remove highest value
  //Uncomment the following to debug your load cell
  //DEBUG_PRINT(" L / H "); DEBUG_PRINT(L);DEBUG_PRINT("/");DEBUG_PRINTLN(H);
  return data/(DATA_SET-2);
}


int32_t ADS1232::readRaw(uint8_t samples)
{
  int32_t valuessum=0;
  for (uint8_t i=0;i<samples;i++){
    valuessum +=  readADC();
  }

  lastAdcValue = valuessum/samples;  
  if (!smoothing) {
    //lastAdcValue = valuessum/samples;    
  } else {
    //In any case, put the value to the array
    if (DATA_SET > DATA_SET_MAX) {
      ESP_LOGV("ADS1232","Zeroing out our dataset");
      DATA_SET = DATA_SET_MAX;
      resetSmoothing(0);
    } else if (DATA_SET < DATA_SET_MIN) {
      ESP_LOGV("ADS1232","Zeroing out our dataset");
      DATA_SET = DATA_SET_MIN;
      resetSmoothing(0);
    }
    if (readIndex > DATA_SET - 1) {
      readIndex = 0;
    } else {
      readIndex++;
    }
    dataSampleSet[readIndex] = lastAdcValue;    
    lastAdcValue = getSmoothedValue();   
  }
  

  // DEBUG_PRINT("lastAdcValue= ");DEBUG_PRINTLN(lastAdcValue);    
  readsPerSecond++;
  if (millis() - lastRateCheck > 1000) {
    actualSPS = readsPerSecond*1000/(millis()-lastRateCheck);
    readsPerSecond = 0;
    lastRateCheck = millis();
  }
  
  return lastAdcValue;
}




int32_t ADS1232::readADC()
{
  
  int32_t adcvalue = 0;
   ESP_LOGV("ADS1232","readADC()");

  if (!safeWait()) {
    ESP_LOGV("ADS1232","ADC wait ERROR");
    return 0;
  }
  //ADC ready...begin
  //manual 1-bit read
  //for esp32 this is a more reliable method than shiftin since we can add a slight delay and avoid spikes in read values  
  //each pulse must be at least 100ns = 0.1μs. We insert a delay here of 1μs to be sure
  //alternatively you can use 8bit shiftin read => adcvalue = shiftIn(doutPin, sclkPin, MSBFIRST); adcvalue <<= 8; , etc...
  int i=0;
  portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
  portENTER_CRITICAL(&mux);
  for(i=0; i < 24; i++) { //24 bits => 24 pulses
    digitalWrite(sclkPin, HIGH);
    delayMicroseconds(1);
    adcvalue = (adcvalue << 1) + digitalRead(doutPin);
    digitalWrite(sclkPin, LOW);
    delayMicroseconds(1);
  }  
  portEXIT_CRITICAL(&mux);  
  digitalWrite(sclkPin, HIGH); // keep dout high // 25th pulse
  delayMicroseconds(1);
  digitalWrite(sclkPin, LOW); 
  //wait for it to become high actually

  readTimeout = 1000;

  while(digitalRead(doutPin) != HIGH && (readTimeout > 0))
  {
    yield();
    readTimeout--;
    ESP_LOGV("ADS1232","waiting for dout %d",readTimeout);
  }

  adcvalue = (adcvalue << 8) / 256;  
  return adcvalue;
}
