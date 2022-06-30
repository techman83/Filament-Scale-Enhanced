from typing import Dict
from .scale import Scale

SCALE_TYPES = ["rpi_hx711", "nau7802", "none"]


# pylint: disable=import-outside-toplevel
def get_scale(settings: Dict) -> Scale:
    """Returns a fully configured Scale based on the settings provided"""

    scale_type = settings["scale_type"]

    if scale_type not in SCALE_TYPES:
        raise ValueError(f"Invalid scale_type: {scale_type}")

    if scale_type == "rpi_hx711":
        from .rpi_hx711 import RPI_HX711

        scale_hx711 = RPI_HX711(settings)
        return scale_hx711

    if scale_type == "nau7802":
        from .nau7802 import NAU7802

        scale_nau7802 = NAU7802(settings)
        return scale_nau7802

    if scale_type == "none":
        from .none import NoneScale

        scale_none = NoneScale(settings)
        return scale_none

    raise ValueError(f"Valid scale_type not yet implemented: {scale_type}")
