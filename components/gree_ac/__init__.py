"""ESPHome Gree AC Component - Modern unified version."""
import esphome.codegen as cg
from esphome.components import climate

CODEOWNERS = ["@yourusername"]
DEPENDENCIES = ["climate", "uart"]
AUTO_LOAD = ["climate"]

gree_ac_ns = cg.esphome_ns.namespace("gree_ac")
GreeAC = gree_ac_ns.class_(
    "GreeAC", cg.PollingComponent, climate.Climate, cg.uart.UARTDevice
)

# Export for use in climate.py
GreeACSwitch = gree_ac_ns.class_("GreeACSwitch", cg.switch_.Switch, cg.Component)
GreeACSelect = gree_ac_ns.class_("GreeACSelect", cg.select.Select, cg.Component)
