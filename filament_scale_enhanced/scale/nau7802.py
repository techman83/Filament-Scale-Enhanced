import time
from typing import Dict
import smbus2
import PyNAU7802
from .scale import Scale


class NAU7802(Scale):
    """Scale subclass for NAU7802 I2C Load Cell ADC, providng a simple wrapper for PyNAU7802"""

    def __init__(self, settings: Dict):

        super().__init__(offset=settings["offset"], cal_factor=settings["cal_factor"])

        self._i2c_bus_id = int(settings["nau_bus_id"])
        self._average_count = 50

        self._bus = smbus2.SMBus(self._i2c_bus_id)
        self._nau = PyNAU7802.NAU7802()
        self._nau.begin(self._bus)

        time.sleep(1)

        if self.offset > 0.0:
            self._nau.setZeroOffset(self.offset)

        if self.cal_factor > 0.0:
            self._nau.setCalibrationFactor(self.cal_factor)

    def is_ready(self) -> bool:
        return self._nau.available()

    def enable(self) -> None:
        self._nau.powerUp()

    def disable(self) -> None:
        self._nau.powerDown()

    def reset(self) -> None:
        self._nau.reset()

    def get_weight(self) -> float:
        self.last_value = round(self._nau.getWeight(False, self._average_count), 1)
        self.last_value_time = time.time()
        return self.last_value

    def tare(self) -> float:
        self._nau.calculateZeroOffset(self._average_count)
        self.offset = self._nau.getZeroOffset()
        return self.offset

    def calibrate(self, weight: float) -> float:
        self._nau.calculateCalibrationFactor(weight, self._average_count)
        self.cal_factor = self._nau.getCalibrationFactor()
        return self.cal_factor

    # pylint: disable=protected-access
    def deinit(self) -> None:
        self._nau.powerDown()
        self._nau._i2cPort.close()
        self._nau = None

    # pylint: disable=line-too-long
    def status(self) -> str:
        return f"NAU status, offset: [{self._nau.getZeroOffset()}], cal_factor: [{self._nau.getCalibrationFactor()}], is_ready: [{self._nau.available()}], weight: [{self._nau.getWeight()}]"  # noqa: E501
