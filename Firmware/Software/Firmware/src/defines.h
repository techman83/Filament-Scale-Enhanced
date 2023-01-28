#define ADC_LDO_EN_PIN 21
#define ADC_LDO_ENABLE 1 //high or low for enable ??? Check datasheet of your LDO.
#define ADC_LDO_DISABLE 0 //high or low for disable ??? Check datasheet of your LDO.



  
#define ADC_PDWN_PIN 5
#define ADC_DOUT_PIN 19
#define ADC_SCLK_PIN 18
#define ADC_GAIN0_PIN 16
#define ADC_GAIN1_PIN 17
#define ADC_SPEED_PIN 22

//#define ADC_A0_PIN 0 //tied to GND AIN1 in orginal PCB -> 0 means disable the Channel select
#define ADC_A0_PIN 27 //in my one tied to GPIO 27 to enable dual scale
#define ADC_TEMP_PIN 0 //tied to GND AIN1 in PRO

#define DISPLAY_WIDTH 128 // display width, in pixels
#define DISPLAY_HEIGHT 64
#define DISPLAY_MOSI_PIN 13 //SDA if using I2C
#define DISPLAY_CLK_PIN 14 //SCL is using I2C

#define INITIALISING_SECS 5 //high or low for enable ??? Check datasheet of your LDO.

#define  TARE_BUTTON_PIN 33



// #define POWERLATCH 35
#define LED 2


#define CALIBRATIONWEIGHT 100 //Calibration weight in gramm
#define CALFACTORDEFAULT 424066   //for my MARVIN NA1 better default  7101991


//UDP SETTINGS
#define UDP_SERVER_IP "192.168.178.100" // UDP LOG Server
#define UDP_SERVER_PORT 44444           // UDP Port Server

// ----------------------
// 
//  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// Further Scale Tweaking in scale.h
// 
// ----------------------


// const char* mqttServer = "m11.cloudmqtt.com";
// const int mqttPort = 12948;

#define MQTT_PORT 1883


#define MQTT_SCALE_WEIGHT "filament/scale/weight"
#define MQTT_SCALE_TARE "filament/scale/tare"
#define MQTT_SCALE_CALIBRATE "filament/scale/calibrate"



#define PIXELCOUNT 1 // this example assumes 4 pixels, making it smaller will cause a failure
#define PIXELPIN 32  // make sure to set this to the correct pin, ignored for Esp8266

#define colorSaturation 128