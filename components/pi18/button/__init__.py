import esphome.codegen as cg
from esphome.components import button, time
import esphome.config_validation as cv
from esphome.const import CONF_TIME_ID

from .. import CONF_PIPSOLAR_ID, PIPSOLAR_COMPONENT_SCHEMA, pipsolar_ns
from ..const import CONF_SYNC_CLOCK

DEPENDENCIES = ["uart", "time"]

PipsolarSyncClockButton = pipsolar_ns.class_("PipsolarSyncClockButton", button.Button)

CONFIG_SCHEMA = PIPSOLAR_COMPONENT_SCHEMA.extend(
    {
        cv.Optional(CONF_SYNC_CLOCK): button.button_schema(
            PipsolarSyncClockButton
        ).extend({cv.Required(CONF_TIME_ID): cv.use_id(time.RealTimeClock)}),
    }
)


async def to_code(config):
    paren = await cg.get_variable(config[CONF_PIPSOLAR_ID])

    if CONF_SYNC_CLOCK in config:
        conf = config[CONF_SYNC_CLOCK]
        var = await button.new_button(conf)
        cg.add(var.set_parent(paren))
        cg.add(var.set_time(await cg.get_variable(conf[CONF_TIME_ID])))
