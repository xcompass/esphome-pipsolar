import esphome.codegen as cg
from esphome.components import datetime
import esphome.config_validation as cv

from .. import CONF_PIPSOLAR_ID, PIPSOLAR_COMPONENT_SCHEMA, pipsolar_ns
from ..const import (
    CONF_AC_CHARGE_END_TIME,
    CONF_AC_CHARGE_START_TIME,
    CONF_AC_OUTPUT_END_TIME,
    CONF_AC_OUTPUT_START_TIME,
)

DEPENDENCIES = ["uart"]

PipsolarTime = pipsolar_ns.class_("PipsolarTime", datetime.TimeEntity)
TimeWindowEnd = pipsolar_ns.enum("TimeWindowEnd", True)

TYPES = {
    CONF_AC_CHARGE_START_TIME: TimeWindowEnd.AC_CHARGE_START,
    CONF_AC_CHARGE_END_TIME: TimeWindowEnd.AC_CHARGE_END,
    CONF_AC_OUTPUT_START_TIME: TimeWindowEnd.AC_OUTPUT_START,
    CONF_AC_OUTPUT_END_TIME: TimeWindowEnd.AC_OUTPUT_END,
}

CONFIG_SCHEMA = PIPSOLAR_COMPONENT_SCHEMA.extend(
    {cv.Optional(key): datetime.time_schema(PipsolarTime) for key in TYPES}
)


async def to_code(config):
    paren = await cg.get_variable(config[CONF_PIPSOLAR_ID])

    for key, window_end in TYPES.items():
        if key not in config:
            continue
        var = await datetime.new_datetime(config[key])
        cg.add(var.set_parent(paren))
        cg.add(var.set_window_end(window_end))
        cg.add(paren.set_time_window_end(window_end, var))
