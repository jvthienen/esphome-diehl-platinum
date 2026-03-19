import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from . import DiehlPlatinumComponent, CONF_DIEHL_PLATINUM_ID

DEPENDENCIES = ["diehl"]

CONF_CONNECTION_STATUS = "connection_status"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_DIEHL_PLATINUM_ID): cv.use_id(DiehlPlatinumComponent),
        cv.Optional(CONF_CONNECTION_STATUS): binary_sensor.binary_sensor_schema(
            device_class="connectivity",
            icon="mdi:connection",
        ),
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_DIEHL_PLATINUM_ID])

    if CONF_CONNECTION_STATUS in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_CONNECTION_STATUS])
        cg.add(parent.set_connection_status_binary_sensor(sens))
