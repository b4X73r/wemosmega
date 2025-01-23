import esphome.codegen as cg
import esphome.config_validation as cv

from esphome import pins
from esphome.components import uart
from esphome.const import (
    CONF_ID,
    CONF_INPUT,
    CONF_NUMBER,
    CONF_MODE,
    CONF_INVERTED,
    CONF_OUTPUT,
)

CODEOWNERS = [ "@b4X73r" ]
DEPENDENCIES = [ "uart" ]
AUTO_LOAD = [ "voltage_sampler" ]

wemosmega_ns = cg.esphome_ns.namespace("wemosmega")

WemosMegaComponent = wemosmega_ns.class_(
    "WemosMegaComponent", cg.Component, uart.UARTDevice
)

CONF_UART = "uart"

CONF_WEMOSMEGA = "wemosmega"
CONFIG_SCHEMA = (
	cv.Schema(
		{
			cv.GenerateID(): cv.declare_id(WemosMegaComponent),
		}
	)
	.extend(cv.COMPONENT_SCHEMA)
	.extend(uart.UART_DEVICE_SCHEMA)
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

WemosMegaGPIOPin = wemosmega_ns.class_(
    "WemosMegaGPIOPin", cg.GPIOPin, cg.Parented.template(WemosMegaComponent)
)

WEMOSMEGA_PIN_SCHEMA = pins.gpio_base_schema(
    pin_type=WemosMegaGPIOPin,
    number_validator=cv.int_range(min=0, max=69),
    modes=[CONF_INPUT, CONF_OUTPUT],
).extend(
    {
        cv.Required(CONF_WEMOSMEGA): cv.use_id(WemosMegaComponent),
    }
)

@pins.PIN_SCHEMA_REGISTRY.register(CONF_WEMOSMEGA, WEMOSMEGA_PIN_SCHEMA)

async def wemosmega_pin_to_code(config):
    parent = await cg.get_variable(config[CONF_WEMOSMEGA])
    var = cg.new_Pvariable(config[CONF_ID], parent, config[CONF_NUMBER])
    cg.add(var.set_inverted(config[CONF_INVERTED]))
    cg.add(var.set_flags(pins.gpio_flags_expr(config[CONF_MODE])))
    return var
