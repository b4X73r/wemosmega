import esphome.codegen as cg
import esphome.config_validation as cv

from esphome.components import sensor, wemosmega
from esphome.const import (
    CONF_ID,
    CONF_NUMBER,
    DEVICE_CLASS_TEMPERATURE,
    UNIT_CELSIUS,
    STATE_CLASS_MEASUREMENT,
)
from esphome.components.wemosmega import (
	CONF_WEMOSMEGA,
    wemosmega_ns,
    WemosMegaComponent,
)

CODEOWNERS = [ "@b4X73r" ]

WemosMegaMAX31865 = wemosmega_ns.class_(
    "WemosMegaMAX31865", sensor.Sensor, cg.PollingComponent
)

CONFIG_SCHEMA = cv.All(
    sensor.sensor_schema(
        WemosMegaMAX31865,
        unit_of_measurement=UNIT_CELSIUS,
        accuracy_decimals=2,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
    )
    .extend(
        {
			cv.Required(CONF_WEMOSMEGA): cv.use_id(WemosMegaComponent),
            cv.Required(CONF_NUMBER): cv.int_range(min=0, max=69),
        }
    )
    .extend(cv.polling_component_schema("60s")),
)

async def to_code(config):
    parent = await cg.get_variable(config[CONF_WEMOSMEGA])
    var = cg.new_Pvariable(config[CONF_ID], parent, config[CONF_NUMBER])
    await cg.register_component(var, config)
    await sensor.register_sensor(var, config)
