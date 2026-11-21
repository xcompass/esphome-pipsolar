import esphome.codegen as cg
from esphome.components import switch
import esphome.config_validation as cv
from esphome.const import CONF_RESTORE_MODE, ICON_POWER

from .. import CONF_PIPSOLAR_ID, PIPSOLAR_COMPONENT_SCHEMA, pipsolar_ns
from ..const import (
    CONF_AC_OUTPUT,
    CONF_ALARM_ON_WHEN_PRIMARY_SOURCE_INTERRUPT,
    CONF_BACKLIGHT_ON,
    CONF_FAULT_CODE_RECORD,
    CONF_LCD_ESCAPE_TO_DEFAULT,
    CONF_OUTPUT_SOURCE_PRIORITY,
    CONF_OVER_TEMPERATURE_RESTART_FUNCTION,
    CONF_OVERLOAD_BYPASS_FUNCTION,
    CONF_OVERLOAD_RESTART_FUNCTION,
    CONF_POWER_SAVING,
    CONF_SILENCE_BUZZER_OPEN_BUZZER,
    CONF_SOLAR_POWER_PRIORITY,
)

DEPENDENCIES = ["uart"]

CONF_CHARGER_SOURCE_PRIORITY_SOLARFIRST = "charger_source_priority_solarfirst"
CONF_CHARGER_SOURCE_PRIORITY_UTILITY = "charger_source_priority_utility"
CONF_CHARGER_SOURCE_PRIORITY_SOLARONLY = "charger_source_priority_solaronly"

TYPES = {
    CONF_OUTPUT_SOURCE_PRIORITY: ("^S007POP1", "^S007POP0"),
    CONF_SOLAR_POWER_PRIORITY: ("^S007PSP1", "^S007PSP0"),
    CONF_CHARGER_SOURCE_PRIORITY_SOLARFIRST: ("^S009PCP0,0", None),
    CONF_CHARGER_SOURCE_PRIORITY_UTILITY: ("^S009PCP0,1", None),
    CONF_CHARGER_SOURCE_PRIORITY_SOLARONLY: ("^S009PCP0,2", None),
    CONF_SILENCE_BUZZER_OPEN_BUZZER: ("^S006PEA", "^S006PDA"),
    CONF_OVERLOAD_BYPASS_FUNCTION: ("^S006PEB", "^S006PDB"),
    CONF_LCD_ESCAPE_TO_DEFAULT: ("^S006PEC", "^S006PDC"),
    CONF_OVERLOAD_RESTART_FUNCTION: ("^S006PED", "^S006PDD"),
    CONF_OVER_TEMPERATURE_RESTART_FUNCTION: ("^S006PEE", "^S006PDE"),
    CONF_BACKLIGHT_ON: ("^S006PEF", "^S006PDF"),
    CONF_ALARM_ON_WHEN_PRIMARY_SOURCE_INTERRUPT: ("^S006PEG", "^S006PDG"),
    CONF_FAULT_CODE_RECORD: ("^S006PEH", "^S006PDH"),
}

PipsolarSwitch = pipsolar_ns.class_("PipsolarSwitch", switch.Switch, cg.Component)

PipsolarAcOutputSwitch = pipsolar_ns.class_(
    "PipsolarAcOutputSwitch", switch.Switch, cg.Component
)

PIPSWITCH_SCHEMA = switch.switch_schema(
    PipsolarSwitch, icon=ICON_POWER, block_inverted=True
).extend(cv.COMPONENT_SCHEMA)


def _restore_mode_disabled(config):
    if str(config[CONF_RESTORE_MODE]) != "DISABLED":
        raise cv.Invalid(
            "ac_output keeps its own 'keep off' setting; restore_mode must be DISABLED"
        )
    return config


# "Keep off" has its own preference, so ESPHome's switch restore is disabled: it would save
# the timer reading the switch publishes instead of the request.
AC_OUTPUT_SCHEMA = (
    switch.switch_schema(
        PipsolarAcOutputSwitch,
        icon="mdi:power-socket-us",
        block_inverted=True,
        default_restore_mode="DISABLED",
    )
    .extend(cv.COMPONENT_SCHEMA)
    .add_extra(_restore_mode_disabled)
)

# ^S006PEI/PDI isn't power saving: MPP Solar's SolarPower uses flag I for the machine
# type (Hybrid/Off-grid, PIRI field 20), and reads FLAG field 8 as that flag.
POWER_SAVING_REMOVED = (
    "The pi18 power_saving switch was removed: its command (^S006PEI/PDI) sets the "
    "machine type (Hybrid/Off-grid) in MPP Solar's SolarPower, not power saving. "
    "Use the machine_type select if you need to change it."
)

CONFIG_SCHEMA = PIPSOLAR_COMPONENT_SCHEMA.extend(
    {cv.Optional(type): PIPSWITCH_SCHEMA for type in TYPES}
).extend(
    {
        cv.Optional(CONF_AC_OUTPUT): AC_OUTPUT_SCHEMA,
        cv.Optional(CONF_POWER_SAVING): cv.invalid(POWER_SAVING_REMOVED),
    }
)


async def to_code(config):
    paren = await cg.get_variable(config[CONF_PIPSOLAR_ID])

    for type, (on, off) in TYPES.items():
        if type in config:
            conf = config[type]
            var = await switch.new_switch(conf)
            await cg.register_component(var, conf)
            cg.add(getattr(paren, f"set_{type}_switch")(var))
            cg.add(var.set_parent(paren))
            cg.add(var.set_on_command(on))
            if off is not None:
                cg.add(var.set_off_command(off))

    if CONF_AC_OUTPUT in config:
        conf = config[CONF_AC_OUTPUT]
        var = await switch.new_switch(conf)
        await cg.register_component(var, conf)
        cg.add(var.set_parent(paren))
        cg.add(paren.set_ac_output_switch(var))
