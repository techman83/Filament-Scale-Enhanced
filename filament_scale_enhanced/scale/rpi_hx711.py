import time
import statistics
from typing import List, Dict
from RPi import GPIO
from hx711 import HX711 as _HX711
from .scale import Scale


class RPI_HX711(Scale):
    """Scale subclass for HX711 Load Cell ADC, providing a wrapper for the HX711 driver"""

    def __init__(self, settings: Dict):

        super().__init__(offset=settings["offset"], cal_factor=settings["cal_factor"])

        self._clock_pin = int(settings["hx_clockpin"])
        self._data_pin = int(settings["hx_datapin"])
        self._average_count = 20

        self._hx = _HX711(self._clock_pin, self._data_pin)
        self._hx_ready = True

    def is_ready(self) -> bool:
        return self._hx_ready

    def enable(self) -> None:
        self._hx.power_up()
        self._hx_ready = True

    def disable(self) -> None:
        self._hx.power_down()
        self._hx_ready = False

    def reset(self) -> None:
        self._hx.reset()

    def get_weight(self) -> float:
        self.enable()
        time.sleep(0.01)
        raw_mean = self.read_raw_mean()
        offset_mean = raw_mean - self.offset

        self.last_value = offset_mean / self.cal_factor
        self.last_value_time = time.time()
        time.sleep(0.01)
        self.disable()
        return self.last_value

    def tare(self) -> float:
        raw_mean = self.read_raw_mean()

        self.offset = raw_mean
        return self.offset

    def calibrate(self, weight: float) -> float:
        raw_mean = self.read_raw_mean()

        self.cal_factor = (raw_mean - self.offset) / weight
        return self.cal_factor

    # pylint: disable=protected-access,no-member
    def deinit(self) -> None:
        self._hx.power_down()
        GPIO.cleanup([self._hx._dout, self._hx._pd_sck])
        self._hx = None

    # pylint: disable=line-too-long
    def status(self) -> str:
        return f"HX711 status, offset: [{self.offset}], cal_factor: [{self.cal_factor}], is_ready: [{self.is_ready()}], last_value: [{self.last_value}], last_value_time: [{self.last_value_time}], time since last_value_time: [{time.time() - self.last_value_time}]"  # noqa: E501

    # HX711 Helpers

    def read_raw_mean(self) -> float:
        raw_data = self._hx.get_raw_data(self._average_count)
        return statistics.mean(self.remove_outliers(raw_data))

    def remove_outliers(self, data: List[float], cutoff_ratio: float = 1.25) -> List:
        data_mean = statistics.mean(data)
        data_stdev = statistics.stdev(data)
        cutoff = data_stdev * cutoff_ratio
        lower = data_mean - cutoff
        upper = data_mean + cutoff
        cleaned_data = [x for x in data if lower <= x <= upper]

        return cleaned_data
