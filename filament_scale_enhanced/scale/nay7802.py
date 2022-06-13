import time
import smbus2
import PyNAU7802
from . import Scale

class NAU7802(Scale):
    """Scale subclass for NAU7802 I2C Load Cell ADC, providng a simple wrapper for PyNAU7802"""

    def __init__(
            self,
            offset: float = 0.0,
            cal_factor: float = 0.0,
            i2c_bus_id: int = 1
    ):

        super().__init__(offset, cal_factor)

        self._i2c_bus_id = int(i2c_bus_id)
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

    
    def status(self) -> str:
        return "NAU status, offset: [{}], cal_factor: [{}], is_ready: [{}], weight: [{}]".format(
            self._nau.getZeroOffset(),
            self._nau.getCalibrationFactor(),
            self._nau.available(),
            self._nau.getWeight()
        )