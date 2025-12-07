"""ESPHome Gree AC Component - Modern unified version."""
import esphome.codegen as cg  # type: ignore
from esphome.components import climate, uart  # type: ignore

CODEOWNERS = ["@yourusername"]
# Declare runtime dependencies so ESPHome loads the needed components
DEPENDENCIES = ["climate", "uart", "sensor"]
# Auto-load helpful component modules when this component is included
AUTO_LOAD = ["climate"]

gree_ac_ns = cg.esphome_ns.namespace("gree_ac")
GreeAC = gree_ac_ns.class_(
    "GreeAC", cg.PollingComponent, climate.Climate, uart.UARTDevice
)

# Export for use in climate.py
# Optional switch/select classes were removed to avoid build-time
# dependencies; they can be reintroduced incrementally later if needed.
