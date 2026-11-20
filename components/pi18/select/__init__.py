import esphome.codegen as cg
from esphome.components import select
import esphome.config_validation as cv
from esphome.const import CONF_ID

from .. import CONF_PIPSOLAR_ID, PIPSOLAR_COMPONENT_SCHEMA, pipsolar_ns
from ..const import (
    CONF_BATTERY_TYPE,
    CONF_BATTERY_UNDER_VOLTAGE,
    CONF_CHARGER_SOURCE_PRIORITY,
    CONF_CURRENT_MAX_AC_CHARGING_CURRENT,
    CONF_CURRENT_MAX_CHARGING_CURRENT,
    CONF_INPUT_VOLTAGE_RANGE,
    CONF_LCD_SLEEP_TIME,
    CONF_MACHINE_TYPE,
    CONF_OUTPUT_SOURCE_PRIORITY,
    CONF_SOLAR_POWER_PRIORITY,
)

DEPENDENCIES = ["uart"]

CONF_OPTIONSMAP = "optionsmap"
CONF_STATUSMAP = "statusmap"


PipsolarSelect = pipsolar_ns.class_("PipsolarSelect", cg.Component, select.Select)


def ensure_option_map():
    def validator(value):
        cv.check_not_templatable(value)
        options_map_schema = cv.Schema(
            {cv.All(cv.string_strict): cv.All(cv.string_strict)}
        )
        value = options_map_schema(value)
        all_values = list(value.values())
        if len(all_values) != len(set(all_values)):
            raise cv.Invalid("Mapping values must be unique.")
        return value

    return validator


TYPES = [
    CONF_OUTPUT_SOURCE_PRIORITY,
    CONF_CHARGER_SOURCE_PRIORITY,
    CONF_SOLAR_POWER_PRIORITY,
    CONF_INPUT_VOLTAGE_RANGE,
    CONF_MACHINE_TYPE,
    CONF_CURRENT_MAX_CHARGING_CURRENT,
    CONF_CURRENT_MAX_AC_CHARGING_CURRENT,
    CONF_BATTERY_UNDER_VOLTAGE,
    CONF_BATTERY_TYPE,
    CONF_LCD_SLEEP_TIME,
]

# Types that ship their option maps; a user optionsmap/statusmap replaces them.
# The status map keys are the numbers the inverter reports, as text.
DEFAULT_MAPS = {
    CONF_BATTERY_TYPE: (
        {"AGM": "^S007PBT0", "Flooded": "^S007PBT1", "User": "^S007PBT2"},
        {"0": "AGM", "1": "Flooded", "2": "User"},
    ),
    CONF_LCD_SLEEP_TIME: (
        {
            "None": "^S008LST00",
            "30 s": "^S008LST01",
            "60 s": "^S008LST02",
            "5 min": "^S008LST10",
            "10 min": "^S008LST20",
        },
        {"0": "None", "1": "30 s", "2": "60 s", "10": "5 min", "20": "10 min"},
    ),
}

PIPSELECT_SCHEMA = cv.COMPONENT_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(PipsolarSelect),
        cv.Optional(CONF_OPTIONSMAP): ensure_option_map(),
        cv.Optional(CONF_STATUSMAP): ensure_option_map(),
    }
).extend(select.select_schema(PipsolarSelect))


def require_option_maps(config):
    for type in TYPES:
        if (
            type in config
            and type not in DEFAULT_MAPS
            and CONF_OPTIONSMAP not in config[type]
        ):
            raise cv.Invalid(f"{CONF_OPTIONSMAP} is required", path=[type])
    return config


CONFIG_SCHEMA = cv.All(
    PIPSOLAR_COMPONENT_SCHEMA.extend(
        {cv.Optional(type): PIPSELECT_SCHEMA for type in TYPES}
    ),
    require_option_maps,
)


async def to_code(config):
    paren = await cg.get_variable(config[CONF_PIPSOLAR_ID])

    for type in TYPES:
        if type in config:
            conf = config[type]
            default_options, default_status = DEFAULT_MAPS.get(type, (None, None))
            options_map = conf.get(CONF_OPTIONSMAP, default_options)
            var = cg.new_Pvariable(conf[CONF_ID])
            await cg.register_component(var, conf)
            await select.register_select(var, conf, options=list(options_map.keys()))
            cg.add(getattr(paren, f"set_{type}_select")(var))
            cg.add(var.set_parent(paren))
            for mappingkey in options_map:
                cg.add(var.add_mapping(mappingkey, options_map[mappingkey]))
            if default_options is not None:
                cg.add(var.set_skip_unchanged(True))
            if type == CONF_LCD_SLEEP_TIME:
                cg.add(
                    var.set_refresh_poll(
                        cg.RawExpression("esphome::pi18::POLLING_P006LST")
                    )
                )
            status_map = conf.get(CONF_STATUSMAP, default_status)
            if status_map is not None:
                for mappingkey in status_map:
                    cg.add(var.add_status_mapping(mappingkey, status_map[mappingkey]))
