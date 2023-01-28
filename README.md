# OctoPrint Filament Scale Enhanced MQTT

This plugin shows the FilamentWeight of an mqtt connected scale. Spool weight settings are in settings. Tare can requested via ui

##HW
**use PCB of jousis9** [https://github.com/jousis9/ESPresso-Scale]  

##Firmware
see firmaware folder

## Configuration

in ```filament_scale_enhanced\_init_.py``` adapate the line ```self._mqtt.connect_async("192.168.XXX.XXX", port=1883, keepalive=60)``` to your mqtt broker


## Topics
Base MQTT Topic ```filament/scale/```
### input  (subscribed topics)
send weight to plugin : MQTT Topic ```filament/scale/weight``` payload ```weight from scale```
### output  (published topics)
get filament weight : MQTT Topic ```filament/scale/filamentweight```  
request tare of scale : MQTT Topic ```filament/scale/tare```  
request calibration of scale : MQTT Topic ```filament/scale/filamentweight``` payload ```calibration weight```  
