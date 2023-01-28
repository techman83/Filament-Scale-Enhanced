#include <Arduino.h>

#include <WiFi.h>

#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include "SPIFFS.h"
#include <rom/rtc.h>
#include <esp_int_wdt.h>
#include <esp_task_wdt.h>

#include <nvs.h>
#include <nvs_flash.h>



#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#define OTA_VERSION_STRING "____" STR(FWHASH) "____"
// #define SPIFFS_VERSION_STRING "____" STR(SPIFFHASH) "____"
#define SPIFFS_VERSION_STRING OTA_VERSION_STRING

#define TAG "esp32helper"

String getMACaddress()
{
    uint8_t mac[6];
    char macStr[18] = {0};
    WiFi.macAddress(mac);
    sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(macStr);
}

void printMacAddress()
{
    ESP_LOGI(TAG,"Mac:%s", getMACaddress().c_str());
}


void printSerialNumber(){
    ESP_LOGI(TAG,"\r\nS/N:%s:S/N\r\n", getMACaddress().c_str());
}


void verbose_print_reset_reason(RESET_REASON reason)
{
  switch ( reason)
  {
    case 1  : ESP_LOGD(TAG,"Vbat power on reset");break;
    case 3  : ESP_LOGD(TAG,"Software reset digital core");break;
    case 4  : ESP_LOGD(TAG,"Legacy watch dog reset digital core");break;
    case 5  : ESP_LOGD(TAG,"Deep Sleep reset digital core");break;
    case 6  : ESP_LOGD(TAG,"Reset by SLC module, reset digital core");break;
    case 7  : ESP_LOGD(TAG,"Timer Group0 Watch dog reset digital core");break;
    case 8  : ESP_LOGD(TAG,"Timer Group1 Watch dog reset digital core");break;
    case 9  : ESP_LOGD(TAG,"RTC Watch dog Reset digital core");break;
    case 10 : ESP_LOGD(TAG,"Instrusion tested to reset CPU");break;
    case 11 : ESP_LOGD(TAG,"Time Group reset CPU");break;
    case 12 : ESP_LOGD(TAG,"Software reset CPU");break;
    case 13 : ESP_LOGD(TAG,"RTC Watch dog Reset CPU");break;
    case 14 : ESP_LOGD(TAG,"for APP CPU, reseted by PRO CPU");break;
    case 15 : ESP_LOGD(TAG,"Reset when the vdd voltage is not stable");break;
    case 16 : ESP_LOGD(TAG,"RTC Watch dog reset digital core and rtc module");break;
    default : ESP_LOGD(TAG,"NO_MEAN");
  }
}

void printResetReason(){
    ESP_LOGD(TAG,"rtc_get_reset_reason(0)");
    verbose_print_reset_reason(rtc_get_reset_reason(0));
    ESP_LOGD(TAG,"rtc_get_reset_reason(1)");
    verbose_print_reset_reason(rtc_get_reset_reason(1));
}

void ESPWelcome()
{
    delay(2000);
    Serial.printf("VERSION: %s\r\n", (char *) OTA_VERSION_STRING);
    Serial.printf("MAC: %s\r\n", (char *) getMACaddress().c_str());
    Serial.printf("ChipID: %06X\r\n", ESP.getChipRevision());
    Serial.printf("CPU frequency: %d MHz\r\n", ESP.getCpuFreqMHz());
    //Serial.printf("Last reset reason: %s\r\n", (char *) ESP.get().c_str());
    Serial.printf("Memory size: %d bytes\r\n", ESP.getFlashChipSize());
    Serial.printf("Free heap: %d bytes\r\n", ESP.getFreeHeap());
    Serial.printf("Firmware size: %d bytes\r\n", ESP.getSketchSize());
    Serial.printf("Free firmware space: %d bytes\r\n", ESP.getFreeSketchSpace());

    Serial.printf("File system total size: %d bytes\r\n", SPIFFS.totalBytes());
    Serial.printf("            used size : %d bytes\r\n", SPIFFS.usedBytes());
    Serial.printf("\r\n\r\n");
    

}


void ESPHardRestart() {
     esp_task_wdt_init(3,true);
     esp_task_wdt_add(NULL);
     esp_restart();
     while(true){delay(5);};
  //https://iotassistant.io/esp32/enable-hardware-watchdog-timer-esp32-arduino-ide/
}

void rebootEspWithReason(String reason)
{
   ESP_LOGE(TAG,"RebootReason: %s", reason.c_str());
   ESPHardRestart();
}

void clearNVS() {
    int err;
    err=nvs_flash_init();
    ESP_LOGD(TAG,"nvs_flash_init: %d",err);
    // Serial.println("nvs_flash_init: " +);
    err=nvs_flash_erase();
    ESP_LOGD(TAG,"nvs_flash_erase: %d",err);
    // Serial.println("nvs_flash_erase: " + err);
 }