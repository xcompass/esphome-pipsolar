import esphome.codegen as cg
from esphome.components import number
import esphome.config_validation as cv
from esphome.const import (
    CONF_MAX_VALUE,
    CONF_MIN_VALUE,
    CONF_STEP,
    DEVICE_CLASS_CURRENT,
    DEVICE_CLASS_VOLTAGE,
    UNIT_AMPERE,
    UNIT_VOLT,
)

from .. import CONF_PIPSOLAR_ID, PIPSOLAR_COMPONENT_SCHEMA, pipsolar_ns
from ..const import (
    CONF_BATTERY_BULK_VOLTAGE,
    CONF_BATTERY_FLOAT_VOLTAGE,
    CONF_BATTERY_RATED_VOLTAGE,
    CONF_BATTERY_RECHARGE_VOLTAGE,
    CONF_BATTERY_REDISCHARGE_VOLTAGE,
    CONF_BATTERY_UNDER_VOLTAGE,
    CONF_CURRENT_MAX_AC_CHARGING_CURRENT,
    CONF_CURRENT_MAX_CHARGING_CURRENT,
)

DEPENDENCIES = ["uart"]

PipsolarNumber = pipsolar_ns.class_("PipsolarNumber", number.Number)
NumberSetting = pipsolar_ns.enum("NumberSetting", is_class=True)

TYPES = {
    CONF_BATTERY_UNDER_VOLTAGE: NumberSetting.BATTERY_UNDER_VOLTAGE,
    CONF_BATTERY_BULK_VOLTAGE: NumberSetting.BATTERY_BULK_VOLTAGE,
    CONF_BATTERY_FLOAT_VOLTAGE: NumberSetting.BATTERY_FLOAT_VOLTAGE,
    CONF_BATTERY_RECHARGE_VOLTAGE: NumberSetting.BATTERY_RECHARGE_VOLTAGE,
    CONF_BATTERY_REDISCHARGE_VOLTAGE: NumberSetting.BATTERY_REDISCHARGE_VOLTAGE,
    CONF_CURRENT_MAX_CHARGING_CURRENT: NumberSetting.MAX_CHARGING_CURRENT,
    CONF_CURRENT_MAX_AC_CHARGING_CURRENT: NumberSetting.MAX_AC_CHARGING_CURRENT,
}

# (min, max, step) in volts for a 24 V or a 48 V battery bank. Back to battery is
# either 0 ("full") or between min and max; min is the smallest non-zero value.
VOLTAGE_RANGES = {
    24: {
        CONF_BATTERY_UNDER_VOLTAGE: (20.4, 24.0, 0.1),
        CONF_BATTERY_BULK_VOLTAGE: (24.0, 29.2, 0.1),
        CONF_BATTERY_FLOAT_VOLTAGE: (24.0, 29.2, 0.1),
        CONF_BATTERY_RECHARGE_VOLTAGE: (22.0, 25.5, 0.5),
        CONF_BATTERY_REDISCHARGE_VOLTAGE: (24.0, 29.0, 0.5),
    },
    48: {
        CONF_BATTERY_UNDER_VOLTAGE: (40.8, 48.0, 0.1),
        CONF_BATTERY_BULK_VOLTAGE: (48.0, 58.4, 0.1),
        CONF_BATTERY_FLOAT_VOLTAGE: (48.0, 58.4, 0.1),
        CONF_BATTERY_RECHARGE_VOLTAGE: (44.0, 51.0, 1.0),
        CONF_BATTERY_REDISCHARGE_VOLTAGE: (48.0, 58.0, 1.0),
    },
}

# (min, max, step) in amperes; the inverter's own list of allowed values narrows these
CURRENT_RANGES = {
    CONF_CURRENT_MAX_CHARGING_CURRENT: (10.0, 90.0, 10.0),
    CONF_CURRENT_MAX_AC_CHARGING_CURRENT: (2.0, 90.0, 1.0),
}

_VOLTAGE_KEYS = [key for key in TYPES if key not in CURRENT_RANGES]


def _number_schema(key):
    if key in CURRENT_RANGES:
        base = number.number_schema(
            PipsolarNumber,
            unit_of_measurement=UNIT_AMPERE,
            device_class=DEVICE_CLASS_CURRENT,
        )
    else:
        base = number.number_schema(
            PipsolarNumber,
            unit_of_measurement=UNIT_VOLT,
            device_class=DEVICE_CLASS_VOLTAGE,
        )
    return base.extend(
        {
            cv.Optional(CONF_MIN_VALUE): cv.float_,
            cv.Optional(CONF_MAX_VALUE): cv.float_,
            cv.Optional(CONF_STEP): cv.positive_float,
        }
    )


def _require_rated_voltage(config):
    if CONF_BATTERY_RATED_VOLTAGE not in config:
        used = [key for key in _VOLTAGE_KEYS if key in config]
        if used:
            raise cv.Invalid(
                f"{used[0]} needs battery_rated_voltage (24 or 48) to pick its "
                "default range"
            )
    return config


CONFIG_SCHEMA = cv.All(
    PIPSOLAR_COMPONENT_SCHEMA.extend(
        {
            cv.Optional(CONF_BATTERY_RATED_VOLTAGE): cv.one_of(24, 48, int=True),
            **{cv.Optional(key): _number_schema(key) for key in TYPES},
        }
    ),
    _require_rated_voltage,
)


def _range(config, key):
    defaults = CURRENT_RANGES.get(key)
    if defaults is None:
        defaults = VOLTAGE_RANGES[config[CONF_BATTERY_RATED_VOLTAGE]][key]
    conf = config[key]
    return (
        conf.get(CONF_MIN_VALUE, defaults[0]),
        conf.get(CONF_MAX_VALUE, defaults[1]),
        conf.get(CONF_STEP, defaults[2]),
    )


async def to_code(config):
    paren = await cg.get_variable(config[CONF_PIPSOLAR_ID])

    for key, setting in TYPES.items():
        if key not in config:
            continue
        min_value, max_value, step = _range(config, key)
        is_back_to_battery = key == CONF_BATTERY_REDISCHARGE_VOLTAGE
        var = await number.new_number(
            config[key],
            # Back to battery also takes 0, "full"; the gap below min is rejected
            min_value=0.0 if is_back_to_battery else min_value,
            max_value=max_value,
            step=step,
        )
        cg.add(var.set_parent(paren))
        cg.add(var.set_setting(setting))
        if is_back_to_battery:
            cg.add(var.set_min_nonzero_value(min_value))
        cg.add(paren.set_number(setting, var))
