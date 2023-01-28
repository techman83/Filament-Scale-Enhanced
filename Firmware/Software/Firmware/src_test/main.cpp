#include "ADS123X.h"

#define SCALE_DOUT   19
#define SCALE_SCLK   18
#define SCALE_PDWN   5
#define SCALE_GAIN0  16
#define SCALE_GAIN2  17
#define SCALE_SPEED  22
#define SCALE_A0     27
#define SCALE_A1     0


// #define ADC_PDWN_PIN 5
// #define ADC_DOUT_PIN 19
// #define ADC_SCLK_PIN 18
// #define ADC_GAIN0_PIN 16
// #define ADC_GAIN2_PIN 17
// #define ADC_SPEED_PIN 22
// #define ADC_A0_PIN 0 //tied to GND AIN2 in PRO
// #define ADC_TEMP_PIN 0 //tied to GND AIN2 in PRO

ADS123X scale;


void setup() {
	
  Serial.begin(115200);
  Serial.println("ADS123X Demo");
  pinMode(21, OUTPUT);
  digitalWrite(21,1);
  pinMode(2, OUTPUT);
  digitalWrite(2,1);
  pinMode(34, OUTPUT);
  digitalWrite(34,1);
   
  scale.begin(SCALE_DOUT, SCALE_SCLK, SCALE_PDWN, SCALE_GAIN0, SCALE_GAIN2, SCALE_SPEED, SCALE_A0, SCALE_A1);
  
  Serial.print("IsReady?: ");
  for(int s=0;s<10;s++){delay(10);Serial.print(".");}
  while (!scale.is_ready()) {
    Serial.print("");
    Serial.println("NO...");
    delay(10);
  }
  Serial.println("YES!!!");
  // Serial.println(scale.is_ready()?"YES":"NO");
  Serial.println("Before setting up the scale:");
  Serial.print("read: \t\t");


  long value_long;
  ERROR_t err = scale.read(AIN2, value_long);
  Serial.print(err);Serial.print(" ");			// print a raw reading from the ADC
  Serial.println(value_long);			// print a raw reading from the ADC

  float value_double;
  Serial.print("read average: \t\t");
  scale.read_average(AIN2,value_double,20);
  Serial.println(value_long);  	// print the average of 20 readings from the ADC

  Serial.print("get value: \t\t");
  scale.get_value(AIN2,value_double,5);
  Serial.println(value_double);		// print the average of 5 readings from the ADC minus the tare weight (not set yet)

  Serial.print("get units: \t\t");
  scale.get_units(AIN2,value_double,5);
  Serial.println(value_double, 1);	// print the average of 5 readings from the ADC minus tare weight (not set) divided 
						// by the SCALE parameter (not set yet)  

  scale.set_scale(AIN2,2280.f);        // this value is obtained by calibrating the scale with known weights; see the README for details
  scale.tare(AIN2);				        // reset the scale to 0

  Serial.println("After setting up the scale:");

  Serial.print("read: \t\t");
  scale.read(AIN2, value_long);
  Serial.println(value_long);                 // print a raw reading from the ADC

  Serial.print("read average: \t\t");
  scale.read_average(AIN2,value_double,20);
  Serial.println(value_long);       // print the average of 20 readings from the ADC

  Serial.print("get value: \t\t");
  scale.get_value(AIN2,value_double,5);
  Serial.println(value_double);		// print the average of 5 readings from the ADC minus the tare weight, set with tare()

  Serial.print("get units: \t\t");
  scale.get_units(AIN2,value_double,5);
  Serial.println(value_double, 1);        // print the average of 5 readings from the ADC minus tare weight, divided 
						// by the SCALE parameter set with set_scale

  Serial.println("Readings:");
}

void loop() {
  // scale.power_up();
  float value;
  Serial.print("one reading:\t");
  ERROR_t err = scale.get_units(AIN2,value,1,true);
  Serial.print(scale.printErroer(err));Serial.print("\t");	

 
  Serial.print(value, 1);
  float value_avg;
  // Serial.print("\t| average:\t");
  // scale.get_units(AIN2,value_avg,10,true);
  // Serial.print(value_avg, 1);
  Serial.println("");
  // scale.power_down();			        // put the ADC in sleep mode
  delay(10);
  //scale.power_up();
}