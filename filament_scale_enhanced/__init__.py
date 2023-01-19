# coding=utf-8
from __future__ import absolute_import

import octoprint.plugin
import paho.mqtt.client as mqtt

from octoprint.server import admin_permission
from octoprint.events import eventManager, Events
 
from .fse_version import VERSION as __version__  # noqa: F401


# pylint: disable=too-many-ancestors
class FilamentScalePlugin(octoprint.plugin.SettingsPlugin,
                          octoprint.plugin.AssetPlugin,
                          octoprint.plugin.TemplatePlugin,
                          octoprint.plugin.SimpleApiPlugin,
                          octoprint.plugin.StartupPlugin):


    t = None
    
    def __init__(self):
        self._mqtt = None
        self._mqtt_connected = False
        self._lastWeight = 0
    
    def initialize(self):
        return
        
    @staticmethod
    def get_template_configs():
        return [
            dict(type="settings", custom_bindings=True)
        ]

    @staticmethod
    def get_settings_defaults():
        return dict(
            tare=8430152,
            reference_unit=-411,
            spool_weight=200,
            clockpin=21,
            datapin=20,
            lastknownweight=0
        )

    @staticmethod
    def get_assets():
        return dict(
            js=["js/filament_scale.js"],
            css=["css/filament_scale.css"],
            less=["less/filament_scale.less"]
        )

    def on_startup(self, host, port):  # pylint: disable=unused-argument
        self.mqtt_connect()
        self.t = octoprint.util.RepeatedTimer(3.0, self.check_weight)
        self.t.start()

    def check_weight(self):
        self._plugin_manager.send_plugin_message(self._identifier, self._lastWeight)
        
    def on_api_command(self, command, data):
       if command == "tare":
         self._logger.info("cmd: tare:")
         self._mqtt.publish("filament/scale/tare",1)
       if command == "calibrate":
         self._logger.info("cmd: calibrate: " + str(data.get("value")))
         self._mqtt.publish("filament/scale/calibrateValue",float(data.get("value")))
       if command == "filamentweight":
         self._logger.info("cmd: filamentweight: " + str(data.get("value")))
         self._mqtt.publish("filament/scale/filamentweight",float(data.get("value")))
          
     
    def get_api_commands(self):
      return dict(
          tare=[],
          calibrate=["value"],
          filamentweight=["value"]
      )    
         
    def get_update_information(self):
        return dict(
            filament_scale=dict(
                displayName="Filament Scale Plugin MQTT",
                displayVersion=self._plugin_version,

                # version check: github repository
                type="github_release",
                user="runningtoy",
                repo="Filament-Scale-Enhanced",
                current=self._plugin_version,

                # update method: pip
                pip="https://github.com/runningtoy/Filament-Scale-Enhanced-mqtt/releases/latest/download/Filament_Scale_Enhanced.zip"  # noqa: E501
            )
        )
        
    def mqtt_connect(self):
        if self._mqtt is None:
            #self._mqtt = mqtt.Client(client_id="OctoprintScale", clean_session=True, userdata=None, protocol=MQTTv311, transport="tcp")
            self._mqtt = mqtt.Client("OctoprintScale")
        else:
            self._mqtt.reinitialise() #otherwise tls_set might be called again causing the plugin to crash 
        self._mqtt.on_connect = self._on_mqtt_connect
        self._mqtt.on_disconnect = self._on_mqtt_disconnect
        self._mqtt.on_message = self._on_mqtt_message
        


        self._mqtt.connect_async("192.168.XXX.XXX", port=1883, keepalive=60)
        if self._mqtt.loop_start() == mqtt.MQTT_ERR_INVAL:
            self._logger.error("Could not start MQTT connection, loop_start returned MQTT_ERR_INVAL")

    def _on_mqtt_disconnect(self, force=False, incl_lwt=True, lwt=None):
        if self._mqtt is None:
            return

        if incl_lwt:
            if lwt is None:
                lwt = self._get_topic("lw")
            if lwt:
                _retain = self._settings.get_boolean(["broker", "retain"])
                self._mqtt.publish("filament/scale/alive",0)
                self._mqtt.publish(lwt, self.LWT_DISCONNECTED, qos=1, retain=_retain)

        self._mqtt.loop_stop()
        
        self._logger.info("Disconnect Filament Scale MQTT")
        
        if force:
            time.sleep(1)
            self._mqtt.loop_stop(force=True)

    def _on_mqtt_connect(self, client, userdata, flags, rc):
        if not client == self._mqtt:
            return

        if not rc == 0:
            reasons = [
                None,
                "Connection to mqtt broker refused, wrong protocol version",
                "Connection to mqtt broker refused, incorrect client identifier",
                "Connection to mqtt broker refused, server unavailable",
                "Connection to mqtt broker refused, bad username or password",
                "Connection to mqtt broker refused, not authorised"
            ]

            if rc < len(reasons):
                reason = reasons[rc]
            else:
                reason = None

            self._logger.error(reason if reason else "Connection to mqtt broker refused, unknown error")
            return

        self._logger.info("ScalePlugin Connected to mqtt broker")
        self._mqtt.publish("filament/scale/alive",1)
        self._mqtt.subscribe("filament/scale/weight")
        self._mqtt_connected = True
        


    def _on_mqtt_message(self, client, userdata, msg):
        #self._logger.info("ScalePlugin mqtt message: "+str(msg.payload.decode("utf-8")))
        self._lastWeight=float(str(msg.payload.decode("utf-8")))
        self._logger.info("ScalePlugin mqtt _lastWeight: "+str(self._lastWeight))


__plugin_name__ = "Filament Scale" 
__plugin_pythoncompat__ = ">=3,<4"


def __plugin_load__():
    global __plugin_implementation__
    __plugin_implementation__ = FilamentScalePlugin()

    global __plugin_hooks__
    __plugin_hooks__ = {
        "octoprint.plugin5.softwareupdate.check_config": __plugin_implementation__.get_update_information
    }