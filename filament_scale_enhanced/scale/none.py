import time
from typing import Dict
from .scale import Scale


class NoneScale(Scale):
    """Scale subclass for 'none' placeholder"""

    def __init__(self, settings: Dict):

        super().__init__(offset=settings["offset"], cal_factor=settings["cal_factor"])

        self._ready = True

    def is_ready(self) -> bool:
        # Not implemented
        return self._ready

    def enable(self) -> None:
        self._ready = True

    def disable(self) -> None:
        self._ready = False

    def reset(self) -> None:
        # Not implemented
        pass

    def get_weight(self) -> float:
        # Not implemented
        return self.last_value

    def tare(self) -> float:
        # Not implemented
        return self.offset

    def calibrate(self, weight: float) -> float:
        # Not implemented
        return self.cal_factor

    def deinit(self) -> None:
        # Not implemented
        pass

    # pylint: disable=line-too-long
    def status(self) -> str:
        return f"NoneScale status, offset: [{self.offset,}], cal_factor: [{self.cal_factor}], is_ready: [{self.is_ready()}], last_value: [{self.last_value}], last_value_time: [{self.last_value_time}], time since last_value_time: [{time.time() - self.last_value_time}]"  # noqa: E501
