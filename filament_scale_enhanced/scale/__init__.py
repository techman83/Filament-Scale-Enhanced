import time

class Scale():

    def __init__(
            self,
            offset: float,
            cal_factor: float
    ):

        self.offset = offset
        self.cal_factor = cal_factor

        self.last_value = 0
        self.last_value_time = time.time()

    def is_ready(self) -> bool:
        raise NotImplementedError

    def enable(self) -> None:
        raise NotImplementedError

    def disable(self) -> None:
        raise NotImplementedError

    def reset(self) -> None:
        raise NotImplementedError

    def get_weight(self) -> float:
        raise NotImplementedError

    def tare(self) -> float:
        raise NotImplementedError

    def calibrate(self, weight: float) -> float:
        raise NotImplementedError

    def status(self) -> str:
        raise NotImplementedError