#include "gree_ac.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace gree_ac {

// Byte positions in packets
static const uint8_t FORCE_UPDATE_BYTE = 7;
static const uint8_t MODE_BYTE = 8;
static const uint8_t MODE_MASK = 0xF0;
static const uint8_t FAN_MASK = 0x0F;
static const uint8_t TEMPERATURE_BYTE = 9;
static const uint8_t PRESET_BYTE = 10;
static const uint8_t SWING_BYTE = 12;
static const uint8_t DISPLAY_BYTE = 13;
static const uint8_t PLASMA_BYTE = 6;
static const uint8_t PLASMA_MASK = 0x04;
static const uint8_t SLEEP_BYTE = 4;
static const uint8_t SLEEP_MASK = 0x08;
static const uint8_t XFAN_BYTE = 6;
static const uint8_t XFAN_MASK = 0x08;
static const uint8_t INDOOR_TEMP_BYTE = 46;
static const uint8_t CRC_BYTE = 46;

void GreeAC::setup() {
  ESP_LOGI(TAG, "Gree AC component v%s starting...", VERSION);
  this->last_handshake_attempt_ = millis();
  this->last_packet_sent_ = millis();
  
  // Setup callbacks for optional components
  this->setup_select_callbacks_();
  this->setup_switch_callbacks_();
  
  // Setup external temperature sensor callback if configured
  if (this->current_temperature_sensor_ != nullptr) {
    this->current_temperature_sensor_->add_on_state_callback([this](float state) {
      this->current_temperature = state;
      this->publish_state();
    });
  }
}

void GreeAC::loop() {
  this->read_uart_data_();
  
  // Check if we need to retry handshake
  uint32_t now = millis();
  if (this->state_ == ACState::INITIALIZING) {
    if (now - this->last_handshake_attempt_ >= HANDSHAKE_RETRY_INTERVAL_MS) {
      ESP_LOGD(TAG, "Retrying handshake...");
      this->last_handshake_attempt_ = now;
      this->send_packet_();
    }
  }
  
  // Check for timeout (AC stopped responding)
  if (this->state_ == ACState::READY) {
    if (now - this->last_packet_received_ >= PACKET_TIMEOUT_MS) {
      ESP_LOGW(TAG, "AC communication timeout, waiting for response...");
      this->state_ = ACState::INITIALIZING;
      this->mark_failed();
    }
  }
}

void GreeAC::update() {
  // Periodic update - send current state
  if (this->state_ == ACState::READY) {
    this->send_packet_();
  }
}

void GreeAC::dump_config() {
  ESP_LOGCONFIG(TAG, "Gree AC:");
  ESP_LOGCONFIG(TAG, "  Update interval: %u ms", this->get_update_interval());
  this->check_uart_settings(4800, 1, uart::UART_CONFIG_PARITY_EVEN, 8);
  
  if (this->horizontal_swing_select_ != nullptr) {
    ESP_LOGCONFIG(TAG, "  Horizontal swing: configured");
  }
  if (this->vertical_swing_select_ != nullptr) {
    ESP_LOGCONFIG(TAG, "  Vertical swing: configured");
  }
  if (this->display_select_ != nullptr) {
    ESP_LOGCONFIG(TAG, "  Display: configured");
  }
  if (this->plasma_switch_ != nullptr) {
    ESP_LOGCONFIG(TAG, "  Plasma: configured");
  }
  if (this->sleep_switch_ != nullptr) {
    ESP_LOGCONFIG(TAG, "  Sleep: configured");
  }
  if (this->xfan_switch_ != nullptr) {
    ESP_LOGCONFIG(TAG, "  X-Fan: configured");
  }
}

climate::ClimateTraits GreeAC::traits() {
  auto traits = climate::ClimateTraits();
  
  // Add feature flags instead of deprecated setter (ESPHome 2025+)
  traits.add_feature_flags(climate::CLIMATE_SUPPORTS_CURRENT_TEMPERATURE);
  traits.set_visual_min_temperature(MIN_TEMPERATURE);
  traits.set_visual_max_temperature(MAX_TEMPERATURE);
  traits.set_visual_temperature_step(TEMPERATURE_STEP);
  
  traits.set_supported_modes({
      climate::CLIMATE_MODE_OFF,
      climate::CLIMATE_MODE_AUTO,
      climate::CLIMATE_MODE_COOL,
      climate::CLIMATE_MODE_DRY,
      climate::CLIMATE_MODE_FAN_ONLY,
      climate::CLIMATE_MODE_HEAT});
  
  traits.set_supported_custom_fan_modes({
      fan_modes::FAN_AUTO,
      fan_modes::FAN_LOW,
      fan_modes::FAN_MEDIUM,
      fan_modes::FAN_HIGH});
  
  // Add swing support
  traits.set_supported_swing_modes({
      climate::CLIMATE_SWING_OFF,
      climate::CLIMATE_SWING_VERTICAL,
      climate::CLIMATE_SWING_HORIZONTAL,
      climate::CLIMATE_SWING_BOTH});
  
  // Add presets
  for (auto preset : this->supported_presets_) {
    traits.add_supported_preset(preset);
  }
  traits.add_supported_preset(climate::CLIMATE_PRESET_NONE);
  
  return traits;
}

void GreeAC::control(const climate::ClimateCall &call) {
  if (this->state_ != ACState::READY) {
    ESP_LOGW(TAG, "AC not ready, ignoring control request");
    return;
  }

  ESP_LOGD(TAG, "Control called");

  // Set force update byte to signal AC firmware
  this->tx_buffer_[FORCE_UPDATE_BYTE] = 175;  // FORCE_UPDATE_VALUE from upstream
  // Show current temperature on display
  this->tx_buffer_[DISPLAY_BYTE] = 0x20;  // DISPLAY_SHOW_TEMP

  // Extract and preserve previous mode & fan values
  uint8_t new_mode = this->tx_buffer_[MODE_BYTE] & MODE_MASK;
  uint8_t new_fan_speed = this->tx_buffer_[MODE_BYTE] & FAN_MASK;

  // Apply mode if provided
  if (call.get_mode()) {
    auto mode_val = call.get_mode().value();
    switch (mode_val) {
      case climate::CLIMATE_MODE_OFF:
        new_mode = static_cast<uint8_t>(ACMode::OFF);
        break;
      case climate::CLIMATE_MODE_AUTO:
        new_mode = static_cast<uint8_t>(ACMode::AUTO);
        break;
      case climate::CLIMATE_MODE_COOL:
        new_mode = static_cast<uint8_t>(ACMode::COOL);
        break;
      case climate::CLIMATE_MODE_DRY:
        new_mode = static_cast<uint8_t>(ACMode::DRY);
        new_fan_speed = static_cast<uint8_t>(ACFanSpeed::S_LOW);  // DRY only supports low fan
        break;
      case climate::CLIMATE_MODE_FAN_ONLY:
        new_mode = static_cast<uint8_t>(ACMode::FAN_ONLY);
        break;
      case climate::CLIMATE_MODE_HEAT:
        new_mode = static_cast<uint8_t>(ACMode::HEAT);
        break;
      default:
        ESP_LOGW(TAG, "Unsupported MODE: %d", static_cast<int>(mode_val));
        break;
    }
  }

  // Apply fan speed if provided (but not in DRY mode, already set to LOW)
  if (call.get_fan_mode()) {
    auto fan_val = call.get_fan_mode().value();
    switch (fan_val) {
      case climate::CLIMATE_FAN_AUTO:
        new_fan_speed = static_cast<uint8_t>(ACFanSpeed::S_AUTO);
        break;
      case climate::CLIMATE_FAN_LOW:
        new_fan_speed = static_cast<uint8_t>(ACFanSpeed::S_LOW);
        break;
      case climate::CLIMATE_FAN_MEDIUM:
        new_fan_speed = static_cast<uint8_t>(ACFanSpeed::S_MEDIUM);
        break;
      case climate::CLIMATE_FAN_HIGH:
        new_fan_speed = static_cast<uint8_t>(ACFanSpeed::S_HIGH);
        break;
      default:
        ESP_LOGW(TAG, "Unsupported FANSPEED: %d", static_cast<int>(fan_val));
        break;
    }
  }

  // Enforce low fan speed in DRY mode
  if (new_mode == static_cast<uint8_t>(ACMode::DRY) && new_fan_speed != static_cast<uint8_t>(ACFanSpeed::S_LOW)) {
    new_fan_speed = static_cast<uint8_t>(ACFanSpeed::S_LOW);
  }

  // Apply preset if provided
  if (call.get_preset()) {
    auto preset_val = call.get_preset().value();
    switch (preset_val) {
      case climate::CLIMATE_PRESET_NONE:
        if (new_mode == static_cast<uint8_t>(ACMode::COOL)) {
          this->tx_buffer_[PRESET_BYTE] = PRESET_COOL_NORMAL;
        } else if (new_mode == static_cast<uint8_t>(ACMode::HEAT)) {
          this->tx_buffer_[PRESET_BYTE] = PRESET_HEAT_NORMAL;
        }
        break;
      case climate::CLIMATE_PRESET_BOOST:
        if (new_mode == static_cast<uint8_t>(ACMode::COOL)) {
          this->tx_buffer_[PRESET_BYTE] = PRESET_COOL_BOOST;
        } else if (new_mode == static_cast<uint8_t>(ACMode::HEAT)) {
          this->tx_buffer_[PRESET_BYTE] = PRESET_HEAT_BOOST;
        }
        break;
      default:
        break;
    }
  }

  // Apply target temperature if provided
  if (call.get_target_temperature()) {
    float target_temp = call.get_target_temperature().value();
    if (target_temp >= MIN_TEMPERATURE && target_temp <= MAX_TEMPERATURE) {
      this->tx_buffer_[TEMPERATURE_BYTE] = static_cast<uint8_t>((target_temp - MIN_TEMPERATURE) * 16);
      this->target_temperature = target_temp;
    } else {
      ESP_LOGW(TAG, "Target temperature out of range: %.1f (valid: %u-%u)", target_temp, MIN_TEMPERATURE, MAX_TEMPERATURE);
    }
  }

  // Apply swing mode if provided
  if (call.get_swing_mode()) {
    auto swing_val = call.get_swing_mode().value();
    switch (swing_val) {
      case climate::CLIMATE_SWING_OFF:
        this->tx_buffer_[SWING_BYTE] = AC_SWING_OFF;
        break;
      case climate::CLIMATE_SWING_VERTICAL:
        this->tx_buffer_[SWING_BYTE] = AC_SWING_VERTICAL;
        break;
      case climate::CLIMATE_SWING_HORIZONTAL:
        this->tx_buffer_[SWING_BYTE] = AC_SWING_HORIZONTAL;
        break;
      case climate::CLIMATE_SWING_BOTH:
        this->tx_buffer_[SWING_BYTE] = AC_SWING_BOTH;
        break;
      default:
        ESP_LOGW(TAG, "Unsupported SWING mode: %d", static_cast<int>(swing_val));
        break;
    }
    this->swing_mode = swing_val;
  }

  // Update mode byte with new mode and fan values
  this->tx_buffer_[MODE_BYTE] = new_mode | new_fan_speed;
  this->mode = static_cast<climate::ClimateMode>(new_mode);  // Update internal state

  // Compute CRC and send
  uint8_t data_length = this->tx_buffer_[2];
  uint16_t size = 3 + data_length;
  if (size > GREE_TX_BUFFER_SIZE) size = GREE_TX_BUFFER_SIZE;
  this->tx_buffer_[size - 1] = this->calculate_checksum_(this->tx_buffer_, size);
  this->write_array(this->tx_buffer_, size);
  this->packets_sent_++;
  this->last_packet_sent_ = millis();
  this->log_packet_(this->tx_buffer_, static_cast<uint8_t>(size), true);

  // Reset force_update byte to "passive" state
  this->tx_buffer_[FORCE_UPDATE_BYTE] = 0;
  this->update_state_ = UpdateState::NO_UPDATE;
}

void GreeAC::read_uart_data_() {
  while (this->available()) {
    uint8_t byte;
    this->read_byte(&byte);

    switch (this->serial_state_) {
      case SerialState::WAIT_SYNC:
        if (byte == GREE_START_BYTE) {
          this->rx_buffer_[0] = byte;
          this->rx_index_ = 1;
          this->serial_state_ = SerialState::RECEIVE;
        }
        break;

      case SerialState::RECEIVE:
        if (this->rx_index_ < GREE_RX_BUFFER_SIZE) {
          this->rx_buffer_[this->rx_index_++] = byte;
        } else {
          // Buffer overflow: reset
          this->serial_state_ = SerialState::WAIT_SYNC;
          this->rx_index_ = 0;
          break;
        }

        // Need at least 3 bytes to know full length: start,start,length
        if (this->rx_index_ >= 3) {
          uint8_t data_length = this->rx_buffer_[2];
          uint16_t full_size = 3 + data_length; // header + data length
          if (this->rx_index_ >= full_size) {
            // Full packet received
            if (this->verify_packet_(this->rx_buffer_, full_size)) {
              this->handle_packet_(this->rx_buffer_, full_size);
            }
            // Reset for next packet
            this->serial_state_ = SerialState::WAIT_SYNC;
            this->rx_index_ = 0;
          }
        }
        break;

      case SerialState::COMPLETE:
      default:
        this->serial_state_ = SerialState::WAIT_SYNC;
        this->rx_index_ = 0;
        break;
    }
    this->last_packet_received_ = millis();
  }
}

void GreeAC::handle_packet_(const uint8_t *data, uint8_t size) {
  // Parse the packet and update state. parse_state_packet_ decodes protocol fields
  // and updates internal climate state.
  this->parse_state_packet_(data, size);
  this->state_ = ACState::READY;
  this->publish_state();
}

// --- Small helper stubs required by header/linker ---
uint8_t GreeAC::calculate_checksum_(const uint8_t *message, size_t size) {
  if (size < 3) {
    return 0;
  }
  uint8_t position = static_cast<uint8_t>(size - 1);
  uint32_t sum = 0;
  // ignore first two bytes & last one
  for (uint16_t i = 2; i < position; i++)
    sum += message[i];
  uint8_t crc = static_cast<uint8_t>(sum % 256);
  return crc;
}

bool GreeAC::verify_packet_(const uint8_t *data, uint8_t size) {
  // Minimal sanity checks
  if (size < 4) {
    this->invalid_packet_errors_++;
    ESP_LOGW(TAG, "Packet too small: %u", size);
    return false;
  }

  // Validate packet type (expect unit report / state)
  if (data[3] != CMD_IN_UNIT_REPORT) {
    this->invalid_packet_errors_++;
    ESP_LOGW(TAG, "Invalid packet type. Expected: 0x%02X, Got: 0x%02X", CMD_IN_UNIT_REPORT, data[3]);
    return false;
  }

  // Check checksum (last byte)
  uint8_t received_crc = data[size - 1];
  uint8_t calc = this->calculate_checksum_(data, size);
  if (received_crc != calc) {
    this->checksum_errors_++;
    ESP_LOGW(TAG, "Invalid checksum. Received: 0x%02X, Calculated: 0x%02X (errors: %u)", received_crc, calc, this->checksum_errors_);
    return false;
  }

  return true;
}

void GreeAC::log_packet_(const uint8_t *message, uint8_t size, bool outgoing) {
  const char *title = outgoing ? "Sent message" : "Received message";
  ESP_LOGV(TAG, "%s:", title);
  constexpr size_t MAX_STR_SIZE = 250;
  if (size * 3 > MAX_STR_SIZE - 1) {
    ESP_LOGE(TAG, "Message too long to dump: %u bytes", size);
    return;
  }
  char str[MAX_STR_SIZE] = {0};
  char *pstr = str;
  for (uint8_t i = 0; i < size; i++) {
    pstr += sprintf(pstr, "%02X ", message[i]);
  }
  ESP_LOGV(TAG, "%s", str);
}

void GreeAC::parse_state_packet_(const uint8_t *data, uint8_t size) {
  // Validate minimum packet size
  constexpr uint8_t MIN_PACKET_SIZE = sizeof(gree_header_t) + 1; // header + at least 1 data byte + CRC
  if (size < MIN_PACKET_SIZE + 1) {
    ESP_LOGW(TAG, "Packet too small: size = %u, minimum = %u", size, MIN_PACKET_SIZE + 1);
    return;
  }

  // CRC check (last byte)
  uint8_t data_crc = data[size - 1];
  uint8_t calculated_crc = this->calculate_checksum_(data, size);

  if (data_crc != calculated_crc) {
    this->checksum_errors_++;
    ESP_LOGW(TAG, "Invalid checksum. Received: 0x%02X, Calculated: 0x%02X (errors: %u)", data_crc, calculated_crc, this->checksum_errors_);
    return;
  }

  // Validate packet type (expect unit report / state)
  if (size <= 3 || data[3] != CMD_IN_UNIT_REPORT) {
    this->invalid_packet_errors_++;
    ESP_LOGW(TAG, "Invalid packet type. Expected: 0x%02X, Got: 0x%02X (errors: %u)", CMD_IN_UNIT_REPORT, size > 3 ? data[3] : 0, this->invalid_packet_errors_);
    return;
  }

  // Validate bounds before accessing temperature
  if (size <= TEMPERATURE_BYTE) {
    ESP_LOGW(TAG, "Packet too small to contain temperature data");
    return;
  }

  // Extract and validate target temperature
  uint8_t temp_raw = data[TEMPERATURE_BYTE];
  float target_temp = (temp_raw / 16.0f) + MIN_TEMPERATURE;
  if (target_temp >= MIN_TEMPERATURE && target_temp <= MAX_TEMPERATURE) {
    this->target_temperature = target_temp;
  } else {
    ESP_LOGW(TAG, "Invalid target temperature: %.1f (raw: 0x%02X)", target_temp, temp_raw);
  }

  // Extract and validate current (indoor) temperature if present
  if (size > INDOOR_TEMP_BYTE) {
    int8_t current_temp_raw = static_cast<int8_t>(data[INDOOR_TEMP_BYTE]);
    float current_temp = current_temp_raw - 40.0f;
    if (current_temp >= -10.0f && current_temp <= 50.0f) {
      this->current_temperature = current_temp;
    } else {
      ESP_LOGW(TAG, "Invalid current temperature: %.1f (raw: 0x%02X)", current_temp, static_cast<uint8_t>(current_temp_raw));
    }
  }

  // Save some bytes into tx_buffer_ for subsequent commands
  this->tx_buffer_[MODE_BYTE] = data[MODE_BYTE];
  this->tx_buffer_[TEMPERATURE_BYTE] = data[TEMPERATURE_BYTE];

  // Update climate mode
  uint8_t mode_byte = data[MODE_BYTE];
  switch (mode_byte & MODE_MASK) {
    case static_cast<uint8_t>(ACMode::OFF):
      this->mode = climate::CLIMATE_MODE_OFF;
      break;
    case static_cast<uint8_t>(ACMode::AUTO):
      this->mode = climate::CLIMATE_MODE_AUTO;
      break;
    case static_cast<uint8_t>(ACMode::COOL):
      this->mode = climate::CLIMATE_MODE_COOL;
      break;
    case static_cast<uint8_t>(ACMode::DRY):
      this->mode = climate::CLIMATE_MODE_DRY;
      break;
    case static_cast<uint8_t>(ACMode::FAN_ONLY):
      this->mode = climate::CLIMATE_MODE_FAN_ONLY;
      break;
    case static_cast<uint8_t>(ACMode::HEAT):
      this->mode = climate::CLIMATE_MODE_HEAT;
      break;
    default:
      ESP_LOGW(TAG, "Unknown AC MODE: 0x%02X", mode_byte & MODE_MASK);
  }

  // Update fan mode
  switch (mode_byte & FAN_MASK) {
    case static_cast<uint8_t>(ACFanSpeed::S_AUTO):
      this->fan_mode = climate::CLIMATE_FAN_AUTO;
      break;
    case static_cast<uint8_t>(ACFanSpeed::S_LOW):
      this->fan_mode = climate::CLIMATE_FAN_LOW;
      break;
    case static_cast<uint8_t>(ACFanSpeed::S_MEDIUM):
      this->fan_mode = climate::CLIMATE_FAN_MEDIUM;
      break;
    case static_cast<uint8_t>(ACFanSpeed::S_HIGH):
      this->fan_mode = climate::CLIMATE_FAN_HIGH;
      break;
    default:
      ESP_LOGW(TAG, "Unknown AC FAN: 0x%02X", mode_byte & FAN_MASK);
  }

  // Parse preset (boost mode)
  if (size > PRESET_BYTE) {
    uint8_t preset_byte = data[PRESET_BYTE];
    switch (preset_byte) {
      case PRESET_COOL_BOOST:
      case PRESET_HEAT_BOOST:
        this->preset = climate::CLIMATE_PRESET_BOOST;
        break;
      case PRESET_COOL_NORMAL:
      case PRESET_HEAT_NORMAL:
      default:
        this->preset = climate::CLIMATE_PRESET_NONE;
        break;
    }
  } else {
    this->preset = climate::CLIMATE_PRESET_NONE;
  }

  // Parse swing mode
  if (size > SWING_BYTE) {
    uint8_t swing_byte = data[SWING_BYTE];
    switch (swing_byte) {
      case AC_SWING_OFF:
        this->swing_mode = climate::CLIMATE_SWING_OFF;
        break;
      case AC_SWING_VERTICAL:
        this->swing_mode = climate::CLIMATE_SWING_VERTICAL;
        break;
      case AC_SWING_HORIZONTAL:
        this->swing_mode = climate::CLIMATE_SWING_HORIZONTAL;
        break;
      case AC_SWING_BOTH:
        this->swing_mode = climate::CLIMATE_SWING_BOTH;
        break;
      default:
        ESP_LOGW(TAG, "Unknown swing mode: 0x%02X", swing_byte);
        break;
    }
    // Save swing mode to write buffer for next command
    this->tx_buffer_[SWING_BYTE] = swing_byte;
  }

  this->packets_received_++;
}

void GreeAC::send_packet_() {
  // Send a packet (used for handshake or periodic updates during READY state)
  uint8_t data_length = this->tx_buffer_[2];
  uint16_t size = 3 + data_length;
  if (size > GREE_TX_BUFFER_SIZE) size = GREE_TX_BUFFER_SIZE;

  // Compute and fill CRC
  this->tx_buffer_[size - 1] = this->calculate_checksum_(this->tx_buffer_, size);

  this->write_array(this->tx_buffer_, size);
  this->packets_sent_++;
  this->last_packet_sent_ = millis();
  this->log_packet_(this->tx_buffer_, static_cast<uint8_t>(size), true);
}

void GreeAC::build_state_packet_() {
  // Placeholder: prepares a standard state request/command packet.
  // Extended later if needed for advanced feature assembly.
}

void GreeAC::setup_select_callbacks_() {
  // Optional select callbacks registered here will be wired by the generated code
  // if select components are configured in YAML.
  // The setters (set_horizontal_swing_select, etc.) assign the pointers,
  // and this is called during setup() to establish the callback connections.
  ESP_LOGD(TAG, "Select callbacks setup (optional selects wired if present)");
}

void GreeAC::setup_switch_callbacks_() {
  // Optional switch callbacks registered here will be wired by the generated code
  // if switch components are configured in YAML.
  // The setters (set_plasma_switch, etc.) assign the pointers,
  // and this is called during setup() to establish the callback connections.
  ESP_LOGD(TAG, "Switch callbacks setup (optional switches wired if present)");
}

void GreeAC::update_swing_states_() {
  // No-op placeholder for swing state update logic.
}

void GreeAC::on_horizontal_swing_change_(const std::string &value) {
  ESP_LOGD(TAG, "Horizontal swing changed to: %s", value.c_str());
  // Placeholder: map value to protocol byte and send command
}

void GreeAC::on_vertical_swing_change_(const std::string &value) {
  ESP_LOGD(TAG, "Vertical swing changed to: %s", value.c_str());
  // Placeholder: map value to protocol byte and send command
}

void GreeAC::on_display_change_(const std::string &value) {
  ESP_LOGD(TAG, "Display changed to: %s", value.c_str());
  // Placeholder: map value to protocol byte and send command
}

void GreeAC::on_plasma_change_(bool state) {
  ESP_LOGD(TAG, "Plasma switch changed to: %s", state ? "on" : "off");
  // Placeholder: set plasma bit in protocol and send command
}

void GreeAC::on_sleep_change_(bool state) {
  ESP_LOGD(TAG, "Sleep switch changed to: %s", state ? "on" : "off");
  // Placeholder: set sleep bit in protocol and send command
}

void GreeAC::on_xfan_change_(bool state) {
  ESP_LOGD(TAG, "X-Fan switch changed to: %s", state ? "on" : "off");
  // Placeholder: set xfan bit in protocol and send command
}

}  // namespace gree_ac
}  // namespace esphome
