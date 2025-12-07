"""Climate platform for Gree AC."""
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate, uart, sensor, select, switch as switch_component
from esphome.const import (
    CONF_ID,
    CONF_SUPPORTED_PRESETS,
    CONF_SUPPORTED_SWING_MODES,
)
from esphome.components.climate import ClimatePreset, ClimateSwingMode
from . import gree_ac_ns, GreeAC

# Configuration keys
CONF_CURRENT_TEMPERATURE_SENSOR = "current_temperature_sensor"
CONF_HORIZONTAL_SWING_SELECT = "horizontal_swing_select"
CONF_VERTICAL_SWING_SELECT = "vertical_swing_select"
CONF_DISPLAY_SELECT = "display_select"
CONF_PLASMA_SWITCH = "plasma_switch"
CONF_SLEEP_SWITCH = "sleep_switch"
CONF_XFAN_SWITCH = "xfan_switch"

# Swing options - must match C++ constants
ALLOWED_CLIMATE_SWING_MODES = {
    "OFF": ClimateSwingMode.CLIMATE_SWING_OFF,
    "BOTH": ClimateSwingMode.CLIMATE_SWING_BOTH,
    "VERTICAL": ClimateSwingMode.CLIMATE_SWING_VERTICAL,
    "HORIZONTAL": ClimateSwingMode.CLIMATE_SWING_HORIZONTAL,
}

# Presets
ALLOWED_CLIMATE_PRESETS = {
    "NONE": ClimatePreset.CLIMATE_PRESET_NONE,
    "BOOST": ClimatePreset.CLIMATE_PRESET_BOOST,
}

validate_presets = cv.enum(ALLOWED_CLIMATE_PRESETS, upper=True)
validate_swing_modes = cv.enum(ALLOWED_CLIMATE_SWING_MODES, upper=True)

CONFIG_SCHEMA = cv.All(
    climate.climate_schema(GreeAC).extend(
        {
            cv.GenerateID(): cv.declare_id(GreeAC),
            cv.Optional(CONF_CURRENT_TEMPERATURE_SENSOR): cv.use_id(sensor.Sensor),
            cv.Optional(CONF_SUPPORTED_PRESETS): cv.ensure_list(validate_presets),
            cv.Optional(CONF_SUPPORTED_SWING_MODES): cv.ensure_list(validate_swing_modes),
            # Optional select components (no forced dependencies)
            cv.Optional(CONF_HORIZONTAL_SWING_SELECT): cv.use_id(select.Select),
            cv.Optional(CONF_VERTICAL_SWING_SELECT): cv.use_id(select.Select),
            cv.Optional(CONF_DISPLAY_SELECT): cv.use_id(select.Select),
            # Optional switch components (no forced dependencies)
            cv.Optional(CONF_PLASMA_SWITCH): cv.use_id(switch_component.Switch),
            cv.Optional(CONF_SLEEP_SWITCH): cv.use_id(switch_component.Switch),
            cv.Optional(CONF_XFAN_SWITCH): cv.use_id(switch_component.Switch),
        }
    )
    .extend(cv.polling_component_schema("1s"))
    .extend(uart.UART_DEVICE_SCHEMA)
)


async def to_code(config):
    """Generate C++ code from config."""
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await climate.register_climate(var, config)
    await uart.register_uart_device(var, config)

    # External temperature sensor
    if CONF_CURRENT_TEMPERATURE_SENSOR in config:
        sens = await cg.get_variable(config[CONF_CURRENT_TEMPERATURE_SENSOR])
        cg.add(var.set_current_temperature_sensor(sens))

    # Supported presets
    if CONF_SUPPORTED_PRESETS in config:
        cg.add(var.set_supported_presets(config[CONF_SUPPORTED_PRESETS]))

    # Supported swing modes
    if CONF_SUPPORTED_SWING_MODES in config:
        cg.add(var.set_supported_swing_modes(config[CONF_SUPPORTED_SWING_MODES]))

    # Optional select components
    if CONF_HORIZONTAL_SWING_SELECT in config:
        sel = await cg.get_variable(config[CONF_HORIZONTAL_SWING_SELECT])
        cg.add(var.set_horizontal_swing_select(sel))

    if CONF_VERTICAL_SWING_SELECT in config:
        sel = await cg.get_variable(config[CONF_VERTICAL_SWING_SELECT])
        cg.add(var.set_vertical_swing_select(sel))

    if CONF_DISPLAY_SELECT in config:
        sel = await cg.get_variable(config[CONF_DISPLAY_SELECT])
        cg.add(var.set_display_select(sel))

    # Optional switch components
    if CONF_PLASMA_SWITCH in config:
        sw = await cg.get_variable(config[CONF_PLASMA_SWITCH])
        cg.add(var.set_plasma_switch(sw))

    if CONF_SLEEP_SWITCH in config:
        sw = await cg.get_variable(config[CONF_SLEEP_SWITCH])
        cg.add(var.set_sleep_switch(sw))

    if CONF_XFAN_SWITCH in config:
        sw = await cg.get_variable(config[CONF_XFAN_SWITCH])
        cg.add(var.set_xfan_switch(sw))

