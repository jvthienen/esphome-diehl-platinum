import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from . import DiehlPlatinumComponent, CONF_DIEHL_PLATINUM_ID

DEPENDENCIES = ["diehl"]

CONF_OPERATING_STATE = "operating_state"
CONF_SERIAL_NUMBER = "serial_number"
CONF_ERROR_STATUS_1 = "error_status_1"
CONF_ERROR_STATUS_2 = "error_status_2"
CONF_ERROR_SOURCE = "error_source"
CONF_POWER_REDUCTION_TYPE = "power_reduction_type"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_DIEHL_PLATINUM_ID): cv.use_id(DiehlPlatinumComponent),
        cv.Optional(CONF_OPERATING_STATE): text_sensor.text_sensor_schema(
            icon="mdi:state-machine",
        ),
        cv.Optional(CONF_SERIAL_NUMBER): text_sensor.text_sensor_schema(
            icon="mdi:identifier",
        ),
        cv.Optional(CONF_ERROR_STATUS_1): text_sensor.text_sensor_schema(
            icon="mdi:alert-circle-outline",
        ),
        cv.Optional(CONF_ERROR_STATUS_2): text_sensor.text_sensor_schema(
            icon="mdi:alert-circle-outline",
        ),
        cv.Optional(CONF_ERROR_SOURCE): text_sensor.text_sensor_schema(
            icon="mdi:alert-decagram-outline",
        ),
        cv.Optional(CONF_POWER_REDUCTION_TYPE): text_sensor.text_sensor_schema(
            icon="mdi:arrow-down-bold-circle-outline",
        ),
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_DIEHL_PLATINUM_ID])

    for key, setter in [
        (CONF_OPERATING_STATE, "set_operating_state_text_sensor"),
        (CONF_SERIAL_NUMBER, "set_serial_number_text_sensor"),
        (CONF_ERROR_STATUS_1, "set_error_status_1_text_sensor"),
        (CONF_ERROR_STATUS_2, "set_error_status_2_text_sensor"),
        (CONF_ERROR_SOURCE, "set_error_source_text_sensor"),
        (CONF_POWER_REDUCTION_TYPE, "set_power_reduction_type_text_sensor"),
    ]:
        if key in config:
            sens = await text_sensor.new_text_sensor(config[key])
            cg.add(getattr(parent, setter)(sens))
