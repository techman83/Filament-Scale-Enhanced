# coding=utf-8
from __future__ import absolute_import

import time
import octoprint.plugin
import flask
from octoprint.util import RepeatedTimer
from .scale import SCALE_TYPES as supported_scale_types
from .scale import get_scale

from .fse_version import VERSION as __version__  # noqa: F401


# pylint: disable=too-many-ancestors
class FilamentScalePlugin(
    octoprint.plugin.SettingsPlugin,
    octoprint.plugin.AssetPlugin,
    octoprint.plugin.TemplatePlugin,
    octoprint.plugin.StartupPlugin,
    octoprint.plugin.ShutdownPlugin,
    octoprint.plugin.SimpleApiPlugin,
):

    # pylint: disable=super-init-not-called
    def __init__(self):
        self.scale = None
        self.ui_refresh_timer = None
        self.last_api_time = time.time()

    def get_settings_defaults(self):
        return dict(
            scale_type="rpi_hx711",
            offset=0.0,
            cal_factor=1.0,
            hx_clockpin=21,
            hx_datapin=20,
            nau_bus_id=1,
            lastknownweight=0,
            spool_weight=200,
            update_delay=3,
        )

    # pylint: disable=line-too-long,fixme
    def get_settings_version(self):
        # TODO: Implement settings migration
        # https://docs.octoprint.org/en/master/plugins/mixins.html#octoprint.plugin.SettingsPlugin.get_settings_version
        pass

    # pylint: disable=line-too-long,fixme
    def on_settings_migrate(self, target, current=None):
        # TODO: Implement settings migration
        # Doc: https://docs.octoprint.org/en/master/plugins/mixins.html#octoprint.plugin.SettingsPlugin.on_settings_migrate
        pass

    # fmt: off
    def get_template_configs(self):
        return [
            dict(type="settings", custom_bindings=True)
        ]
    # fmt: on

    def get_assets(self):
        return dict(
            js=["js/filament_scale.js"],
            css=["css/filament_scale.css"],
            less=["less/filament_scale.less"],
        )

    # pylint: disable=unused-argument
    def on_startup(self, host, port):
        self.last_api_time = time.time()

        scale_type = self._settings.get(["scale_type"])
        offset = float(self._settings.get(["offset"]))
        cal_factor = float(self._settings.get(["cal_factor"]))

        self._logger.info(f"Scale init, scale_type: [{scale_type}], offset: [{offset}], cal_factor: [{cal_factor}]")

        # Set up our scale with initial settings
        self.scale_init()

        # Set up our repeating timer to regularly update the UI
        self.scale_check_timer_init()

    def on_shutdown(self):
        scale_type = self._settings.get(["scale_type"])
        self._logger.info(f"Shutdown scale deinit, scale_type: [{scale_type}]")
        # Make sure to deinit scale on shutdown, freeing up any GPIOs or GPIO busses
        self.scale_deinit()

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
                pip="https://github.com/techman83/Filament-Scale-Enhanced/releases/latest/download/Filament_Scale_Enhanced.zip",  # noqa: E501
            )
        )

    # Internal Helpers

    def check_weight(self):
        """Checks weight and updates UI via plugin message"""
        v = self.scale.get_weight()

        self._settings.set(["lastknownweight"], v)
        self._settings.save()

        self._plugin_manager.send_plugin_message(self._identifier, v)

    def scale_init(self) -> None:
        """Inits self.scale based on current settings"""
        settings_dict = self._settings.get_all_data()
        scale_type = settings_dict["scale_type"]
        self._logger.debug(f"Scale init, scale_type: [{scale_type}]")
        self.scale = get_scale(settings_dict)

    def scale_deinit(self) -> None:
        """Deinits scale"""
        if self.scale:
            scale_type = self._settings.get(["scale_type"])
            self._logger.debug(f"Scale deinit: [{scale_type}]")
            self.scale.deinit()
            self.scale = None

    def scale_check_timer_init(self) -> None:
        """Inits scale check RepeatedTimer if scale exists"""
        if self.scale:
            timer_delay = float(self._settings.get(["update_delay"]))
            self._logger.debug(f"ui_refresh_timer init, delay: [{timer_delay}]")
            self.ui_refresh_timer = RepeatedTimer(timer_delay, self.check_weight)
            self.ui_refresh_timer.start()
        else:
            self.scale_check_timer_deinit()
            self._logger.error("RepatedTimer [check_weight()]: no scale present")

    def scale_check_timer_deinit(self) -> None:
        """Deinits scale check RepeatedTimer if ui_refresh_timer exists"""
        if self.ui_refresh_timer:
            self._logger.debug("ui_refresh_timer deinit")
            self.ui_refresh_timer.cancel()
            self.ui_refresh_timer = None

    # API Helpers

    def scale_tare(self) -> float:
        current_offset = self._settings.get(["offset"])
        self._logger.info(f"Tare started, current offset: [{current_offset}]")

        new_offset = self.scale.tare()

        self._settings.set(["offset"], new_offset)
        self._settings.save()
        self._logger.info(f"Tare complete, new_offset: [{new_offset}]")

        return new_offset

    def scale_calibrate(self, weight: float) -> float:
        current_cal_factor = self._settings.get(["cal_factor"])
        self._logger.info(f"Cal start, weight: [{weight}], current_cal_factor: [{current_cal_factor}]")
        new_cal_factor = self.scale.calibrate(weight)

        self._settings.set(["cal_factor"], new_cal_factor)
        self._settings.save()
        self._logger.info(f"Cal complete, weight: [{weight}], new_cal_factor: [{new_cal_factor}]")

        return new_cal_factor

    def scale_weight(self) -> float:
        self._logger.debug(f"Weight last_value: [{self.scale.last_value}], last_value_time: [{self.scale.last_value_time}]")

        update_delay = self._settings.get_float(["update_delay"])
        if time.time() - self.scale.last_value_time < (update_delay - 0.1):
            weight = self.scale.last_value
        else:
            weight = self.scale.get_weight()

        self._logger.info(f"Scale weight: [{weight}]")

        return weight

    # REST API Handling

    # pylint: disable=too-many-return-statements,too-many-branches,too-many-statements,broad-except
    def on_api_get(self, request) -> flask.Response:
        if time.time() - self.last_api_time < 0.1:
            return flask.make_response("Slow down!", 200)

        command = request.args.get("command")
        value = request.args.get("value")

        if command == "tare":
            self.last_api_time = time.time()

            new_offset = self.scale_tare()
            self._logger.debug(f"API tare, new_offset: [{new_offset}]")

            return flask.make_response(str(new_offset), 200)

        if command == "spool_weight":
            self.last_api_time = time.time()

            if value:
                try:
                    spool_weight = float(value)
                except ValueError:
                    self._logger.debug("API spool_weight value error, cannot convert value to float")
                    return flask.make_response("Invalid spool_weight value, cannot convert to float", 400)

                self._logger.debug(f"API spool_weight set, new spool_weight: [{spool_weight}]")

                self._settings.set(["spool_weight"], spool_weight)
                self._settings.save()
            else:
                spool_weight = self._settings.get(["spool_weight"])
                self._logger.debug(f"API spool_weight get, spool_weight: [{spool_weight}]")

            return flask.make_response(str(spool_weight), 200)

        if command == "scale_type":
            self.last_api_time = time.time()

            if value:
                try:
                    scale_type = str(value)
                except ValueError:
                    self._logger.debug("API scale_type value error, cannot convert value to string")
                    return flask.make_response("Invalid scale_type value, cannot convert to string", 400)

                if scale_type not in supported_scale_types:
                    self._logger.debug("API scale_type value error, not supported")
                    return flask.make_response("Invalid scale_type, not supported", 400)

                current_scale_type = self._settings.get(["scale_type"])

                if current_scale_type == scale_type:
                    self._logger.debug(f"API scale_type set, same as current: [{current_scale_type}], new: [{scale_type}]")
                else:
                    self._logger.debug(f"API scale_type set, current: [{current_scale_type}], new: [{scale_type}]")
                    self._settings.set(["scale_type"], scale_type)
                    self._settings.save()

                    try:
                        self.scale_deinit()
                        self.scale_check_timer_deinit()

                        self.scale_init()
                        self.scale_check_timer_init()
                    except Exception:
                        self._logger.error("API scale_type scale init error")
                        return flask.make_response("scale_type init error", 500)
            else:
                scale_type = self._settings.get(["scale_type"])
                self._logger.debug(f"API scale_type get, scale_type: [{scale_type}]")

            return flask.make_response(scale_type, 200)

        if command == "calibrate":
            self.last_api_time = time.time()

            try:
                value = float(value)
            except ValueError:
                self._logger.debug("API calibrate value error, cannot convert value to float")
                return flask.make_response("Invalid calbiration weight value, cannot convert to float", 400)

            if value < 0:
                self._logger.debug("API calibrate value error, cannot calibrate against zero or negative")
                return flask.make_response("Invalid calibration weight value, must be greater than zero", 400)

            new_cal_factor = self.scale_calibrate(value)
            self._logger.debug(f"API calibrate, new_cal_factor: [{new_cal_factor}]")

            return flask.make_response(str(new_cal_factor), 200)

        if command == "weight":
            self.last_api_time = time.time()

            weight = self.scale_weight()
            self._logger.debug(f"API weight: [{weight}]")

            return flask.make_response(str(weight), 200)

        if command == "status":
            self.last_api_time = time.time()

            status = self.scale.status()
            self._logger.debug(f"API status: [{status}]")

            return flask.make_response(str(status), 200)

        return flask.make_response("not found", 404)


__plugin_name__ = "Filament Scale"  # pylint: disable=global-variable-undefined
__plugin_pythoncompat__ = ">=3,<4"  # pylint: disable=global-variable-undefined


# pylint: disable=global-variable-undefined
def __plugin_load__():
    global __plugin_implementation__
    __plugin_implementation__ = FilamentScalePlugin()

    global __plugin_hooks__
    __plugin_hooks__ = {"octoprint.plugin5.softwareupdate.check_config": __plugin_implementation__.get_update_information}
