import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart, sensor
from esphome.const import CONF_ID

smart_table_ns = cg.esphome_ns.namespace("smart_table")
SmartTable = smart_table_ns.class_("SmartTable", cg.Component)

CONF_DESK_UART = "desk_uart"
CONF_CONTROLLER_UART = "controller_uart"
CONF_HEIGHT_SENSOR = "height_sensor"

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(SmartTable),
    cv.Required(CONF_DESK_UART): cv.use_id(uart.UARTComponent),
    cv.Required(CONF_CONTROLLER_UART): cv.use_id(uart.UARTComponent),
    cv.Required(CONF_HEIGHT_SENSOR): cv.use_id(sensor.Sensor),
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    desk = await cg.get_variable(config[CONF_DESK_UART])
    controller = await cg.get_variable(config[CONF_CONTROLLER_UART])
    height_sensor = await cg.get_variable(config[CONF_HEIGHT_SENSOR])
    var = cg.new_Pvariable(config[CONF_ID], desk, controller, height_sensor)
    await cg.register_component(var, config)

    