#include <Arduino.h>
#include <WiFi.h>
extern "C" {
  #include "freertos/FreeRTOS.h"
  #include "freertos/timers.h"
}

#include "esp32Helper.h"
#include <Wire.h>
#include <Ticker.h>

#include "defines.h"
#include "scale.h"
#define ADS1232
#include "ArduinoNvs.h"
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include "credentials.h"
#include "udp_logging.h"
#include "esp_log.h"
#include <AsyncMqttClient.h>
#include <NeoPixelBus.h>


AsyncWebServer server(80);

SCALE scale = SCALE(ADC_PDWN_PIN, ADC_SCLK_PIN, ADC_DOUT_PIN, ADC_A0_PIN, ADC_SPEED_PIN, ADC_GAIN1_PIN, ADC_GAIN0_PIN, ADC_TEMP_PIN);
bool timermode = false;
uint32_t start_timer = 0;
uint32_t calFactorULong = CALFACTORDEFAULT;

#include "mybuttons.h"


uint32_t calibrationTimoutTime = 120000;
bool updateRunning = false;

double currentScaleValue=0;
double lastScaleValue=0;

int count = 26;

AsyncMqttClient mqttClient;
TimerHandle_t mqttReconnectTimer;
TimerHandle_t wifiReconnectTimer;



RgbColor red(colorSaturation, 0, 0);
RgbColor green(0, colorSaturation, 0);
RgbColor blue(0, 0, colorSaturation);
RgbColor white(colorSaturation);
RgbColor black(0);

NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(PIXELCOUNT, PIXELPIN);

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  ESP_LOGV("main","Disconnected from MQTT.");
  if (WiFi.isConnected()) {
    xTimerStart(mqttReconnectTimer, 0);
  }
}

void fct_setServer()
{
    
    ESP_LOGI("main", "Setup UDP to Server: %s & PORT: %d", UDP_SERVER_IP, UDP_SERVER_PORT);

    IPAddress udpIP;
    // udpIP.fromString(UDP_SERVER_IP);
    // udp_logging_init(udpIP, UDP_SERVER_PORT, udp_logging_vprintf);
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(200, "text/plain", "Hi! I am a Scale."); });

    AsyncElegantOTA.begin(&server); // Start ElegantOTA
    server.begin();
    ESP_LOGV("main", "HTTP server started");
}

void fct_unsetServer()
{
    udp_close();
    server.end();   
    ESP_LOGV("main", "ServerClosed");
}

void connectToWifi() {
  ESP_LOGV("main","Connecting to Wi-Fi...");
  WiFi.mode(WIFI_STA);
   WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void connectToMqtt() {
  ESP_LOGV("main","Connecting to MQTT...");
  mqttClient.connect();
}

void fct_calibrateScale(float _calibrationweight)
{
  ESP_LOGD("main", "Start Calibration");
  for (int i = 0; i < 20; i++)
  {
    delay(20);
    scale.readUnits(1);
  }
  scale.calibrate(_calibrationweight, calibrationTimoutTime, 0.05);
  if (scale.getCalibrationStatus() == calibrationStatus::FINISHED)
  {
    calFactorULong = (uint32_t)(scale.getCalFactor() * 10000.0);
    NVS.setInt("calFactorULong", calFactorULong);
    NVS.setInt("validitycheck", calFactorULong);
    ESP_LOGD("main", "Calibration Done:: calFactorULong: %d", calFactorULong);
  }

  for (int i = 0; i < 20; i++)
  {
    delay(100);
  }
}


void fct_calibrateScale(){
  fct_calibrateScale(CALIBRATIONWEIGHT);
}



bool callTare=false;
float callCalibrate=-100;

void fct_callTare(){
  callTare=true;
}

void fct_callCalibrate(){
  callCalibrate=CALIBRATIONWEIGHT;
}

void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  String _topic((char*)topic);
  _topic.toUpperCase();
  ESP_LOGV("main", "topic : %s", String(_topic));


  String _MQTT_SCALE_CALIBRATE = String(MQTT_SCALE_CALIBRATE);
  _MQTT_SCALE_CALIBRATE.toUpperCase();
  if(_topic.compareTo(_MQTT_SCALE_CALIBRATE)==0){
    ESP_LOGV("main", "fct_calibrateScale -> payload: %s", String(payload));
    String _payload = String(payload);
    callCalibrate =_payload.toFloat();
  }
  
  String _MQTT_SCALE_TARE = String(MQTT_SCALE_TARE);
  _MQTT_SCALE_TARE.toUpperCase();
  if(_topic.compareTo(_MQTT_SCALE_TARE)==0){
    ESP_LOGV("main", "fct_tareScale");
    callTare=true;
  }
}


void onMqttSubscribe(uint16_t packetId, uint8_t qos) {
  Serial.println("Subscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
  Serial.print("  qos: ");
  Serial.println(qos);
}

void WiFiEvent(WiFiEvent_t event) {
  Serial.printf("[WiFi-event] event: %d\n", event);
  switch(event) {
    case SYSTEM_EVENT_STA_GOT_IP:
      ESP_LOGV("main","WiFi connected");
      connectToMqtt();
      fct_setServer();
      ESP_LOGV("main", "Connected to: %s", WIFI_SSID);
      ESP_LOGV("main", "IP address:  %s", WiFi.localIP().toString().c_str());
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      ESP_LOGV("main","WiFi lost connection");
      fct_unsetServer();
      xTimerStop(mqttReconnectTimer, 0); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
      xTimerStart(wifiReconnectTimer, 0);
      break;
  }
}






float fct_roundToDecimal(double value, int dec)
{
  double mlt = powf(10.0f, dec);
  value = roundf(value * mlt) / mlt;
  return (float)value;
}


bool inRange(uint32_t val, uint32_t value, double range)
{
  uint32_t minimum = (uint32_t)(value * (1 - range));
  uint32_t maximum = (uint32_t)(value * (1 + range));
  return ((val >= minimum) && (val <= maximum));
}

void fct_initScale()
{
  pinMode(ADC_LDO_EN_PIN, OUTPUT);
  digitalWrite(ADC_LDO_EN_PIN, ADC_LDO_ENABLE);
  ESP_LOGI("main", "ADC_LDO_EN_PIN Enabled");

  const uint8_t DEFAULT_ADC_SPEED = 10; // 10 or 80
  //--------
  ESP_LOGI("main", "set DEFAULT_ADC_SPEED");
  scale.begin(1, 128, DEFAULT_ADC_SPEED);
  ESP_LOGI("main", "DEFAULT_ADC_SPEED set and start calibrateADC");
  scale.calibrateADC();
  ESP_LOGI("main", "calibrateADC done");

  uint32_t _calFactorULong_ = NVS.getInt("calFactorULong");
  // set CalcFactor only if in a plausible range
  if (inRange(_calFactorULong_, NVS.getInt("validitycheck"), 0.05))
  {
    calFactorULong = _calFactorULong_;
    scale.setCalFactor(calFactorULong / 10000.0);
    ESP_LOGI("main", "setCalFactor: valid");
  }
  else
  {
    ESP_LOGI("main", "setCalFactor: not valid");
  }

//  scale.setSensitivity(120);
//  scale.setSmoothing(5);
  
 scale.setSensitivity(255);
 scale.setSmoothing(1);
 scale.setSpeed(DEFAULT_ADC_SPEED);
}





void fct_tareScale()
{
  for (int i = 1; i < 5; i++)
  {
    scale.readUnits(1);
  }
  scale.tare(2, false, true, false);
}




void onMqttConnect(bool sessionPresent) {
  Serial.println("Connected to MQTT.");
  uint16_t packetIdSub = mqttClient.subscribe(MQTT_SCALE_CALIBRATE, 2);
  uint16_t packetIdSub2 = mqttClient.subscribe(MQTT_SCALE_TARE, 2);
}


void setup()
{
  Serial.begin(115200);

  strip.Begin();
  strip.SetPixelColor(0, green);
  strip.Show();

  esp_log_level_set("*", CORE_DEBUG_LEVEL);
  ESP_LOGV("main", "Setup: fct_setupDisplay");
  //---------------------
  mqttReconnectTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(2000), pdFALSE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(connectToMqtt));
  wifiReconnectTimer = xTimerCreate("wifiTimer", pdMS_TO_TICKS(2000), pdFALSE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(connectToWifi));


  WiFi.onEvent(WiFiEvent);

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  //mqttClient.onSubscribe(onMqttSubscribe);
  //mqttClient.onUnsubscribe(onMqttUnsubscribe);
  // mqttClient.onPublish(onMqttPublish);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.onSubscribe(onMqttSubscribe);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  // If your broker requires authentication (username and password), set them below
  //mqttClient.setCredentials("REPlACE_WITH_YOUR_USER", "REPLACE_WITH_YOUR_PASSWORD");
  connectToWifi();
  //---------------------
  ESP_LOGV("main", "Button: setup");
  button_setup();
  
  // ---------------------
  NVS.begin();
  // ---------------------
  fct_initScale();
  ESP_LOGV("main", "Setup: fct_initScale");
  //---------------------

  scale.tare(2, false, true, true);
  ESP_LOGV("main", "Setup: scale.tare");
  strip.SetPixelColor(0, black);
  strip.Show();
  
  
}

uint32_t u_time = 0;
uint32_t lastVinRead = 0;
uint32_t read_time = 0;

void loop()
{
  //---- Button  Loop
  button_loop();
  
  if ((millis() - read_time >= 100))
  {
    ESP_LOGV("main", "currentScaleValue: %.3f",currentScaleValue);
    currentScaleValue=scale.readUnits(1);
    read_time = millis();
  }

  //---- Scale  Loop
  if ((millis() - u_time >= 1000))
  {
    ESP_LOGV("main", "publish: %.3f",currentScaleValue);
    uint16_t packetIdPub1 = mqttClient.publish(MQTT_SCALE_WEIGHT, 1, true, String(currentScaleValue,3).c_str());    
    u_time = millis();
  }

  if(callTare){
    callTare=false;
    strip.SetPixelColor(0, blue);
    strip.Show();
    fct_tareScale();
    strip.SetPixelColor(0, black);
    strip.Show();
  }
  if(callCalibrate>-1){
    float _callCalibrate=callCalibrate;
    callCalibrate=-100;
    strip.SetPixelColor(0, red);
    strip.Show();
    fct_calibrateScale(_callCalibrate);
    strip.SetPixelColor(0, black);
    strip.Show();
  }
}