import esphome.codegen as cg
import esphome.config_validation as cv

from esphome.components import sensor, voltage_sampler
from esphome.const import (
    CONF_ID,
    CONF_NUMBER,
    CONF_RAW,
    DEVICE_CLASS_VOLTAGE,
    DEVICE_CLASS_TEMPERATURE,
    UNIT_VOLT,
    UNIT_CELSIUS,
    STATE_CLASS_MEASUREMENT,
)
from .. import (
	CONF_WEMOSMEGA,
    wemosmega_ns,
    WemosMegaComponent,
)

CODEOWNERS = [ "@b4X73r" ]

#MULTI_CONF = True

CONF_SAMPLES = "samples"

WemosMegaADCSensor = wemosmega_ns.class_(
    "WemosMegaADCSensor", sensor.Sensor, cg.PollingComponent, voltage_sampler.VoltageSampler, cg.Component
)

CONFIG_SCHEMA = cv.All(
    sensor.sensor_schema(
        WemosMegaADCSensor,
        unit_of_measurement=UNIT_VOLT,
        accuracy_decimals=2,
        device_class=DEVICE_CLASS_VOLTAGE,
        state_class=STATE_CLASS_MEASUREMENT,
    )
    .extend(
        {
			cv.Required(CONF_WEMOSMEGA): cv.use_id(WemosMegaComponent),
            cv.Required(CONF_NUMBER): cv.int_range(min=54, max=69),
            cv.Optional(CONF_RAW, default=False): cv.boolean,
            cv.Optional(CONF_SAMPLES, default=1): cv.int_range(min=1, max=255),
        }
    )
    .extend(cv.polling_component_schema("60s")),
)

async def to_code(config):
    parent = await cg.get_variable(config[CONF_WEMOSMEGA])
    var = cg.new_Pvariable(config[CONF_ID], parent, config[CONF_NUMBER])
    await cg.register_component(var, config)
    await sensor.register_sensor(var, config)

    cg.add(var.set_output_raw(config[CONF_RAW]))
    cg.add(var.set_sample_count(config[CONF_SAMPLES]))
