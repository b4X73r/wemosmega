import esphome.codegen as cg
import esphome.config_validation as cv

from esphome.components import output, wemosmega
from esphome.const import (
    CONF_ID,
    CONF_NUMBER,
)
from esphome.components.wemosmega import (
	CONF_WEMOSMEGA,
    wemosmega_ns,
    WemosMegaComponent,
)

CODEOWNERS = [ "@b4X73r" ]
DEPENDENCIES = [ "wemosmega" ]

WemosMegaPWM = wemosmega_ns.class_(
    "WemosMegaPWM", output.FloatOutput, cg.Component
)

CONFIG_SCHEMA = cv.All(
    output.FLOAT_OUTPUT_SCHEMA.extend(
        {
            cv.GenerateID(CONF_ID): cv.declare_id(WemosMegaPWM),
			cv.Required(CONF_WEMOSMEGA): cv.use_id(WemosMegaComponent),
            cv.Required(CONF_NUMBER): cv.int_range(min=2, max=13),
        }
    )
    .extend(cv.COMPONENT_SCHEMA),
)

async def to_code(config):
    parent = await cg.get_variable(config[CONF_WEMOSMEGA])
    var = cg.new_Pvariable(config[CONF_ID], parent, config[CONF_NUMBER])
    await cg.register_component(var, config)
    await output.register_output(var, config)
