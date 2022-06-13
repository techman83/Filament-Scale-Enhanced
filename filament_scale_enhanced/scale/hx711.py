import time
import statistics
from typing import List
from hx711 import HX711 as _HX711

from . import Scale

class HX711(Scale):
    """Scale subclass for HX711 Load Cell ADC"""

    def __init__(
            self,
            offset: float,
            cal_factor: float,
            clock_pin: int,
            data_pin: int
    ):

        super().__init__(offset, cal_factor)

        self._clock_pin = clock_pin
        self._data_pin = data_pin
        self._average_count = 20

        self._hx = _HX711(data_pin, clock_pin)
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
        raw_mean = self.read_raw_mean()

        self.last_value = round((raw_mean - self.offset) / self.cal_factor, 1)
        self.last_value_time = time.time()
        return self.last_value

    def tare(self) -> float:
        raw_mean = self.read_raw_mean()
        
        self.offset = raw_mean
        return self.offset

    def calibrate(self, weight: float) -> float:
        raw_mean = self.read_raw_mean()

        self.cal_factor = (raw_mean - self.offset) / weight
        return self.cal_factor

    def status(self) -> str:
        return "HX711 status, offset: [{}], cal_factor: [{}], is_ready: [{}], last_value: [{}], last_value_time: [{}], time since last_value_time: [{}]".format(
            self.offset,
            self.cal_factor,
            self.is_ready(),
            self.last_value,
            self.last_value_time,
            time.time() - self.last_value_time,
        )

    # HX711 Helpers

    def read_raw_mean(self):
        raw_data = self._hx.get_raw_data(self._average_count)
        return statistics.mean(self.remove_outliers(raw_data))

    def remove_outliers(self, data: List[float], cutoff_ratio: float = 1.25):
        data_mean = statistics.mean(data)
        data_stdev = statistics.stdev(data)
        cutoff = data_stdev * cutoff_ratio
        lower = data_mean - cutoff
        upper = data_mean + cutoff
        cleaned_data = [x for x in data if lower <= x <= upper]

        return cleaned_data