# coding=utf-8
from __future__ import absolute_import

import octoprint.plugin
import flask
import time
from octoprint.access.permissions import Permissions, ADMIN_GROUP

from .fse_version import VERSION as __version__  # noqa: F401

# pylint: disable=too-many-ancestors
class FilamentScalePlugin(octoprint.plugin.SettingsPlugin,
                          octoprint.plugin.AssetPlugin,
                          octoprint.plugin.TemplatePlugin,
                          octoprint.plugin.StartupPlugin,
                          octoprint.plugin.SimpleApiPlugin):

    @staticmethod
    def get_template_configs():
        return [
            dict(type="settings", custom_bindings=True)
        ]

    @staticmethod
    def get_settings_defaults():
        return dict(
            # chips=["hx711", "nau7802", "none"],
            chip="nau7802",
            offset=0.0,
            cal_factor=1.0,
            hx_clockpin=21,
            hx_datapin=20,
            nau_bus_id=1,
            lastknownweight=0,
            spool_weight=200,
            update_delay=3.0
        )

    @staticmethod
    def get_assets():
        return dict(
            js=["js/filament_scale.js"],
            css=["css/filament_scale.css"],
            less=["less/filament_scale.less"]
        )


    def on_startup(self, host, port):  # pylint: disable=unused-argument
        self.time = time.time()
        
        chip = self._settings.get(["chip"])
        offset = float(self._settings.get(["offset"]))
        cal_factor = float(self._settings.get(["cal_factor"]))

        self._logger.info("Setting up scale, chip: [{}], offset: [{}], cal_factor: [{}]".format(chip, offset, cal_factor))

        if chip == "nau7802":
            from .scale.nau7802 import NAU7802

            bus_id = int(self._settings.get(["nau_bus_id"]))

            self._logger.debug("Initializing NAU7802, bus_id: [{}]".format(bus_id))
            self.scale = NAU7802(offset, cal_factor, bus_id)
            self._logger.debug("Initialized NAU7802, is_ready: [{}]".format(self.scale.is_ready()))
        elif chip == "hx711":
            from .scale.hx711 import HX711

            clock_pin = self._settings.get(["hx_clockpin"])
            data_pin = self._settings.get(["hx_datapin"])

            self._logger.debug("Initializing HX711, clock_pin: [{}], data_pin: [{}]".format(clock_pin, data_pin))
            self.scale = HX711(offset, cal_factor, clock_pin, data_pin)
            self._logger.debug("Initialized HX711, is_ready: [{}]".format(self.scale.is_ready()))
        else:
            self.scale = None

        if self.scale.is_ready():
            timer_delay = float(self._settings.get(["update_delay"]))
            self._logger.info("Setting up RepeatedTimer for check_weight(), delay: [{}]".format(timer_delay))
            self.t = octoprint.util.RepeatedTimer(timer_delay, self.check_weight)
            self.t.start()
        else:
            self.t = None
            self._logger.warn("Skipping RepatedTimer setup for check_weight(), scale is not ready")


    def check_weight(self):
        chip = self._settings.get(["chip"])
        if chip == "hx711":
            self.scale.enable()

        v = self.scale.get_weight()
        
        self._logger.debug("weight: [{}]".format(v))

        self._settings.set(["lastknownweight"], v)
        self._settings.save()

        self._plugin_manager.send_plugin_message(self._identifier, v)

        if chip == "hx711":
            self.scale.disable()


    # pylint: disable=line-too-long
    def get_update_information(self):
        # Define the configuration for your plugin to use with the
        # Software Update Plugin here.
        # See https://github.com/foosel/OctoPrint/wiki/Plugin:-Software-Update
        # for details.
        return dict(
            filament_scale=dict(
                displayName="Filament Scale Plugin",
                displayVersion=self._plugin_version,

                # version check: github repository
                type="github_release",
                user="techman83",
                repo="Filament-Scale-Enhanced",
                current=self._plugin_version,

                # update method: pip
                pip="https://github.com/techman83/Filament-Scale-Enhanced/releases/latest/download/Filament_Scale_Enhanced.zip"  # noqa: E501
            )
        )


    def scale_tare(self) -> float:
        current_offset = self._settings.get(["offset"])
        self._logger.info("Tare started, current offset: [{}]".format(current_offset))

        new_offset = self.scale.tare()

        self._settings.set(["offset"], new_offset)
        self._settings.save()
        self._logger.info("Tare complete, new_offset: [{}]".format(new_offset))

        return new_offset


    def scale_calibrate(self, weight: float) -> float:
        current_cal_factor = self._settings.get(["cal_factor"])
        self._logger.info("Calibration start, weight: [{}], current cal_factor: [{}]".format(weight, current_cal_factor))

        new_cal_factor = self.scale.calibrate(weight)

        self._settings.set(["cal_factor"], new_cal_factor)
        self._settings.save()
        self._logger.info("Calibration complete, weight: [{}], new_cal_factor: [{}]".format(weight, new_cal_factor))

        return new_cal_factor


    def scale_weight(self) -> float:
        self._logger.debug("Scale last_value: [{}], last_value_time: [{}]".format(self.scale.last_value, self.scale.last_value_time))

        if time.time() - self.scale.last_value_time < 5:
            weight = self.scale.last_value
        else:
            weight = self.scale.get_weight()

        self._logger.info("Scale weight: [{}]".format(weight))

        return weight


    def on_api_get(self, request) -> str:
        if time.time() - self.time < 0.1:
            return flask.make_response("Slow down!", 200)

        command = request.args.get('command')
        value = request.args.get('value')

        if command == "tare":
            self.time = time.time()

            new_offset = self.scale_tare()
            self._logger.info("API tare, new_offset: [{}]".format(new_offset))

            return flask.make_response(str(new_offset), 200)
        elif command == "calibrate":
            self.time = time.time()

            try:
                value = float(value)
            except ValueError:
                self._logger.info("API calibrate value error, cannot convert value to float")
                return flask.make_response("Invalid calbiration weight value, cannot convert to float", 400)

            if value < 0:
                self._logger.info("API calibrate value error, cannot calibrate against zero or negative")
                return flask.make_response("Invalid calibration weight value, must be greater than zero", 400)

            new_cal_factor = self.scale_calibrate(value)
            self._logger.info("API calibrate, new_cal_factor: [{}]".format(new_cal_factor))

            return flask.make_response(str(new_cal_factor), 200)
        elif command == "weight":
            self.time = time.time()

            weight = self.scale_weight()
            self._logger.info("API weight: [{}]".format(weight))

            return flask.make_response(str(weight), 200)
        elif command == "status":
            self.time = time.time()

            status = self.scale.status()
            self._logger.info("API status: [{}]".format(status))

            return flask.make_response(str(status), 200)

        return flask.make_response("ok", 200)


__plugin_name__ = "Filament Scale"  # pylint: disable=global-variable-undefined
__plugin_pythoncompat__ = ">=3,<4"  # pylint: disable=global-variable-undefined


# pylint: disable=global-variable-undefined
def __plugin_load__():
    global __plugin_implementation__
    __plugin_implementation__ = FilamentScalePlugin()

    global __plugin_hooks__
    __plugin_hooks__ = {
        "octoprint.plugin5.softwareupdate.check_config": __plugin_implementation__.get_update_information
    }
