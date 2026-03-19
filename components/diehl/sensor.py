import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    CONF_ID,
    DEVICE_CLASS_POWER,
    DEVICE_CLASS_VOLTAGE,
    DEVICE_CLASS_CURRENT,
    DEVICE_CLASS_ENERGY,
    DEVICE_CLASS_FREQUENCY,
    DEVICE_CLASS_TEMPERATURE,
    DEVICE_CLASS_DURATION,
    STATE_CLASS_MEASUREMENT,
    STATE_CLASS_TOTAL_INCREASING,
    UNIT_WATT,
    UNIT_VOLT,
    UNIT_AMPERE,
    UNIT_KILOWATT_HOURS,
    UNIT_HERTZ,
    UNIT_CELSIUS,
    UNIT_HOUR,
    UNIT_OHM,
    UNIT_PERCENT,
)
from . import DiehlPlatinumComponent, CONF_DIEHL_PLATINUM_ID

DEPENDENCIES = ["diehl"]

# Sensor configuration keys
CONF_AC_POWER = "ac_power"
CONF_DC_POWER = "dc_power"
CONF_AC_VOLTAGE_PHASE1 = "ac_voltage_phase1"
CONF_AC_VOLTAGE_PHASE2 = "ac_voltage_phase2"
CONF_AC_VOLTAGE_PHASE3 = "ac_voltage_phase3"
CONF_AC_CURRENT_PHASE1 = "ac_current_phase1"
CONF_AC_CURRENT_PHASE2 = "ac_current_phase2"
CONF_AC_CURRENT_PHASE3 = "ac_current_phase3"
CONF_DC_VOLTAGE = "dc_voltage"
CONF_DC_CURRENT = "dc_current"
CONF_AC_FREQUENCY = "ac_frequency"
CONF_ENERGY_TODAY = "energy_today"
CONF_ENERGY_TOTAL = "energy_total"
CONF_HOURS_TOTAL = "hours_total"
CONF_HOURS_TODAY = "hours_today"
CONF_TEMPERATURE_1 = "temperature_1"
CONF_TEMPERATURE_2 = "temperature_2"
CONF_TEMPERATURE_3 = "temperature_3"
CONF_TEMPERATURE_4 = "temperature_4"
CONF_TEMPERATURE_5 = "temperature_5"
CONF_TEMPERATURE_6 = "temperature_6"
CONF_INSULATION_RESISTANCE = "insulation_resistance"
CONF_POWER_REDUCTION_ABSOLUTE = "power_reduction_absolute"
CONF_POWER_REDUCTION_RELATIVE = "power_reduction_relative"
CONF_POWER_REDUCTION_DURATION = "power_reduction_duration"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_DIEHL_PLATINUM_ID): cv.use_id(DiehlPlatinumComponent),
        cv.Optional(CONF_AC_POWER): sensor.sensor_schema(
            unit_of_measurement=UNIT_WATT,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_POWER,
            state_class=STATE_CLASS_MEASUREMENT,
            icon="mdi:solar-power",
        ),
        cv.Optional(CONF_DC_POWER): sensor.sensor_schema(
            unit_of_measurement=UNIT_WATT,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_POWER,
            state_class=STATE_CLASS_MEASUREMENT,
            icon="mdi:solar-panel",
        ),
        cv.Optional(CONF_AC_VOLTAGE_PHASE1): sensor.sensor_schema(
            unit_of_measurement=UNIT_VOLT,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_VOLTAGE,
            state_class=STATE_CLASS_MEASUREMENT,
            icon="mdi:flash",
        ),
        cv.Optional(CONF_AC_VOLTAGE_PHASE2): sensor.sensor_schema(
            unit_of_measurement=UNIT_VOLT,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_VOLTAGE,
            state_class=STATE_CLASS_MEASUREMENT,
            icon="mdi:flash",
        ),
        cv.Optional(CONF_AC_VOLTAGE_PHASE3): sensor.sensor_schema(
            unit_of_measurement=UNIT_VOLT,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_VOLTAGE,
            state_class=STATE_CLASS_MEASUREMENT,
            icon="mdi:flash",
        ),
        cv.Optional(CONF_AC_CURRENT_PHASE1): sensor.sensor_schema(
            unit_of_measurement=UNIT_AMPERE,
            accuracy_decimals=2,
            device_class=DEVICE_CLASS_CURRENT,
            state_class=STATE_CLASS_MEASUREMENT,
            icon="mdi:current-ac",
        ),
        cv.Optional(CONF_AC_CURRENT_PHASE2): sensor.sensor_schema(
            unit_of_measurement=UNIT_AMPERE,
            accuracy_decimals=2,
            device_class=DEVICE_CLASS_CURRENT,
            state_class=STATE_CLASS_MEASUREMENT,
            icon="mdi:current-ac",
        ),
        cv.Optional(CONF_AC_CURRENT_PHASE3): sensor.sensor_schema(
            unit_of_measurement=UNIT_AMPERE,
            accuracy_decimals=2,
            device_class=DEVICE_CLASS_CURRENT,
            state_class=STATE_CLASS_MEASUREMENT,
            icon="mdi:current-ac",
        ),
        cv.Optional(CONF_DC_VOLTAGE): sensor.sensor_schema(
            unit_of_measurement=UNIT_VOLT,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_VOLTAGE,
            state_class=STATE_CLASS_MEASUREMENT,
            icon="mdi:current-dc",
        ),
        cv.Optional(CONF_DC_CURRENT): sensor.sensor_schema(
            unit_of_measurement=UNIT_AMPERE,
            accuracy_decimals=2,
            device_class=DEVICE_CLASS_CURRENT,
            state_class=STATE_CLASS_MEASUREMENT,
            icon="mdi:current-dc",
        ),
        cv.Optional(CONF_AC_FREQUENCY): sensor.sensor_schema(
            unit_of_measurement=UNIT_HERTZ,
            accuracy_decimals=2,
            device_class=DEVICE_CLASS_FREQUENCY,
            state_class=STATE_CLASS_MEASUREMENT,
            icon="mdi:sine-wave",
        ),
        cv.Optional(CONF_ENERGY_TODAY): sensor.sensor_schema(
            unit_of_measurement="Wh",
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_ENERGY,
            state_class=STATE_CLASS_TOTAL_INCREASING,
            icon="mdi:solar-power-variant",
        ),
        cv.Optional(CONF_ENERGY_TOTAL): sensor.sensor_schema(
            unit_of_measurement=UNIT_KILOWATT_HOURS,
            accuracy_decimals=2,
            device_class=DEVICE_CLASS_ENERGY,
            state_class=STATE_CLASS_TOTAL_INCREASING,
            icon="mdi:counter",
        ),
        cv.Optional(CONF_HOURS_TOTAL): sensor.sensor_schema(
            unit_of_measurement=UNIT_HOUR,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_DURATION,
            state_class=STATE_CLASS_TOTAL_INCREASING,
            icon="mdi:timer-outline",
        ),
        cv.Optional(CONF_HOURS_TODAY): sensor.sensor_schema(
            unit_of_measurement=UNIT_HOUR,
            accuracy_decimals=2,
            device_class=DEVICE_CLASS_DURATION,
            state_class=STATE_CLASS_MEASUREMENT,
            icon="mdi:timer-sand",
        ),
        cv.Optional(CONF_TEMPERATURE_1): sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
            icon="mdi:thermometer",
        ),
        cv.Optional(CONF_TEMPERATURE_2): sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
            icon="mdi:thermometer",
        ),
        cv.Optional(CONF_TEMPERATURE_3): sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
            icon="mdi:thermometer",
        ),
        cv.Optional(CONF_TEMPERATURE_4): sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
            icon="mdi:thermometer",
        ),
        cv.Optional(CONF_TEMPERATURE_5): sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
            icon="mdi:thermometer",
        ),
        cv.Optional(CONF_TEMPERATURE_6): sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
            icon="mdi:thermometer",
        ),
        cv.Optional(CONF_INSULATION_RESISTANCE): sensor.sensor_schema(
            unit_of_measurement=UNIT_OHM,
            accuracy_decimals=0,
            state_class=STATE_CLASS_MEASUREMENT,
            icon="mdi:omega",
        ),
        cv.Optional(CONF_POWER_REDUCTION_ABSOLUTE): sensor.sensor_schema(
            unit_of_measurement=UNIT_WATT,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_POWER,
            state_class=STATE_CLASS_MEASUREMENT,
            icon="mdi:arrow-down-bold",
        ),
        cv.Optional(CONF_POWER_REDUCTION_RELATIVE): sensor.sensor_schema(
            unit_of_measurement=UNIT_PERCENT,
            accuracy_decimals=1,
            state_class=STATE_CLASS_MEASUREMENT,
            icon="mdi:percent",
        ),
        cv.Optional(CONF_POWER_REDUCTION_DURATION): sensor.sensor_schema(
            unit_of_measurement=UNIT_HOUR,
            accuracy_decimals=2,
            device_class=DEVICE_CLASS_DURATION,
            state_class=STATE_CLASS_MEASUREMENT,
            icon="mdi:timer-alert-outline",
        ),
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_DIEHL_PLATINUM_ID])

    for key, setter in [
        (CONF_AC_POWER, "set_ac_power_sensor"),
        (CONF_DC_POWER, "set_dc_power_sensor"),
        (CONF_AC_VOLTAGE_PHASE1, "set_ac_voltage_phase1_sensor"),
        (CONF_AC_VOLTAGE_PHASE2, "set_ac_voltage_phase2_sensor"),
        (CONF_AC_VOLTAGE_PHASE3, "set_ac_voltage_phase3_sensor"),
        (CONF_AC_CURRENT_PHASE1, "set_ac_current_phase1_sensor"),
        (CONF_AC_CURRENT_PHASE2, "set_ac_current_phase2_sensor"),
        (CONF_AC_CURRENT_PHASE3, "set_ac_current_phase3_sensor"),
        (CONF_DC_VOLTAGE, "set_dc_voltage_sensor"),
        (CONF_DC_CURRENT, "set_dc_current_sensor"),
        (CONF_AC_FREQUENCY, "set_ac_frequency_sensor"),
        (CONF_ENERGY_TODAY, "set_energy_today_sensor"),
        (CONF_ENERGY_TOTAL, "set_energy_total_sensor"),
        (CONF_HOURS_TOTAL, "set_hours_total_sensor"),
        (CONF_HOURS_TODAY, "set_hours_today_sensor"),
        (CONF_TEMPERATURE_1, "set_temperature_1_sensor"),
        (CONF_TEMPERATURE_2, "set_temperature_2_sensor"),
        (CONF_TEMPERATURE_3, "set_temperature_3_sensor"),
        (CONF_TEMPERATURE_4, "set_temperature_4_sensor"),
        (CONF_TEMPERATURE_5, "set_temperature_5_sensor"),
        (CONF_TEMPERATURE_6, "set_temperature_6_sensor"),
        (CONF_INSULATION_RESISTANCE, "set_insulation_resistance_sensor"),
        (CONF_POWER_REDUCTION_ABSOLUTE, "set_power_reduction_absolute_sensor"),
        (CONF_POWER_REDUCTION_RELATIVE, "set_power_reduction_relative_sensor"),
        (CONF_POWER_REDUCTION_DURATION, "set_power_reduction_duration_sensor"),
    ]:
        if key in config:
            sens = await sensor.new_sensor(config[key])
            cg.add(getattr(parent, setter)(sens))
