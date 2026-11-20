from esphome import automation
import esphome.codegen as cg
from esphome.components import output
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_VALUE

from .. import CONF_PIPSOLAR_ID, PIPSOLAR_COMPONENT_SCHEMA, pipsolar_ns
from ..const import (
    CONF_BATTERY_BULK_VOLTAGE,
    CONF_BATTERY_FLOAT_VOLTAGE,
    CONF_CURRENT_MAX_AC_CHARGING_CURRENT,
    CONF_CURRENT_MAX_CHARGING_CURRENT,
)

DEPENDENCIES = ["pi18"]

PipsolarOutput = pipsolar_ns.class_("PipsolarOutput", output.FloatOutput)
SetOutputAction = pipsolar_ns.class_("SetOutputAction", automation.Action)

CONF_POSSIBLE_VALUES = "possible_values"


# 24 V banks: 24.0-29.2 V; 48 V banks: 48.0-58.4 V
_VOLTAGE_STEPS = [round(v * 0.1, 1) for v in range(240, 293)] + [
    round(v * 0.1, 1) for v in range(480, 585)
]

ChargeVoltage = pipsolar_ns.enum("ChargeVoltage", is_class=True)

# ^S015MCHGVmmm,nnn sets bulk and float together, in 0.1 V. These outputs have no
# template: the component fills in the other voltage from the last PIRI reply.
CHARGE_VOLTAGES = {
    CONF_BATTERY_BULK_VOLTAGE: ChargeVoltage.BULK_VOLTAGE,
    CONF_BATTERY_FLOAT_VOLTAGE: ChargeVoltage.FLOAT_VOLTAGE,
}

# printf templates take the level as a float, so they must use %f conversions
TYPES = {
    CONF_CURRENT_MAX_AC_CHARGING_CURRENT: (
        [2, 10, 20, 30, 40, 50, 60, 70, 80, 90],
        "^S014MUCHGC0,%03.0f",
    ),
    CONF_CURRENT_MAX_CHARGING_CURRENT: (
        [10, 20, 30, 40, 50, 60, 70, 80, 90],
        "^S013MCHGC0,%03.0f",
    ),
    CONF_BATTERY_BULK_VOLTAGE: (_VOLTAGE_STEPS, None),
    CONF_BATTERY_FLOAT_VOLTAGE: (_VOLTAGE_STEPS, None),
}

CONFIG_SCHEMA = PIPSOLAR_COMPONENT_SCHEMA.extend(
    {
        cv.Optional(type): output.FLOAT_OUTPUT_SCHEMA.extend(
            {
                cv.Required(CONF_ID): cv.declare_id(PipsolarOutput),
                cv.Optional(CONF_POSSIBLE_VALUES, default=values): cv.All(
                    cv.ensure_list(cv.positive_float), cv.Length(min=1)
                ),
            }
        )
        for type, (values, _) in TYPES.items()
    }
)


async def to_code(config):
    paren = await cg.get_variable(config[CONF_PIPSOLAR_ID])

    for type, (_, command) in TYPES.items():
        if type in config:
            conf = config[type]
            var = cg.new_Pvariable(conf[CONF_ID])
            await output.register_output(var, conf)
            cg.add(var.set_parent(paren))
            if command is not None:
                cg.add(var.set_set_command(command))
            if type in CHARGE_VOLTAGES:
                cg.add(var.set_charge_voltage(CHARGE_VOLTAGES[type]))
            if CONF_POSSIBLE_VALUES in conf:
                cg.add(var.set_possible_values(conf[CONF_POSSIBLE_VALUES]))


@automation.register_action(
    "output.pi18.set_level",
    SetOutputAction,
    cv.Schema(
        {
            cv.Required(CONF_ID): cv.use_id(PipsolarOutput),
            cv.Required(CONF_VALUE): cv.templatable(cv.positive_float),
        }
    ),
    synchronous=True,
)
async def output_pi18_set_level_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    template_ = await cg.templatable(config[CONF_VALUE], args, cg.float_)
    cg.add(var.set_level(template_))
    return var
