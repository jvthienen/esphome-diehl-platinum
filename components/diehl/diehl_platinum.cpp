#include "diehl_platinum.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include <cinttypes>
#include <cstring>

namespace esphome {
namespace diehl {

static const char *const TAG = "diehl";

// --- Protocol constants ---
// getValue request: [0x35, 0x13, 0x01, valueType, CRC_H, CRC_L]  (6 bytes)
// Response: [header..., payload..., CRC_H, CRC_L]
// End-of-list marker: first byte == 0x84 (132)

static const uint8_t CMD_GET_VALUE_HEADER_0 = 0x35;  // 53 decimal
static const uint8_t CMD_GET_VALUE_HEADER_1 = 0x13;  // 19 decimal
static const uint8_t CMD_GET_VALUE_LEN = 0x01;
static const uint8_t RESPONSE_END_OF_LIST = 0x84;     // 132 decimal
static const uint8_t RESPONSE_ERROR = 0x84;

// Minimum valid response length: header(3) + at least 1 byte payload + 2 CRC = 6
static const size_t MIN_RESPONSE_LEN = 6;
// Maximum expected response length for value queries
static const size_t MAX_RESPONSE_LEN = 48;

// Timeout between sending a request and expecting a full response (ms)
static const uint32_t RESPONSE_TIMEOUT_MS = 500;
// Delay between consecutive value queries to avoid overwhelming the inverter (ms)
static const uint32_t INTER_QUERY_DELAY_MS = 100;
// Number of consecutive errors before marking connection as failed
static const uint8_t MAX_CONSECUTIVE_ERRORS = 5;

// ============================================================================
// CRC-16 Calculation
// ============================================================================

uint16_t DiehlPlatinumComponent::calc_crc16(const uint8_t *data, size_t len) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < len; i++) {
    uint8_t index = (uint8_t)(data[i] ^ (crc >> 8));
    crc = CRC16_TABLE[index] ^ (crc << 8);
  }
  return crc;
}

// ============================================================================
// Setup
// ============================================================================

void DiehlPlatinumComponent::setup() {
  ESP_LOGD(TAG, "Setting up Diehl Platinum inverter component...");

  // Build the query list based on which sensors are configured.
  // This ensures we only request data the user actually wants.

  if (this->operating_state_text_sensor_ != nullptr)
    this->query_list_.push_back(VALUE_STATE);
  if (this->hours_total_sensor_ != nullptr)
    this->query_list_.push_back(VALUE_H_TOTAL);
  if (this->hours_today_sensor_ != nullptr)
    this->query_list_.push_back(VALUE_H_ON);
  if (this->energy_total_sensor_ != nullptr)
    this->query_list_.push_back(VALUE_E_TOTAL);
  if (this->energy_today_sensor_ != nullptr)
    this->query_list_.push_back(VALUE_E_DAY);
  if (this->ac_power_sensor_ != nullptr)
    this->query_list_.push_back(VALUE_P_AC);
  if (this->dc_power_sensor_ != nullptr)
    this->query_list_.push_back(VALUE_P_DC);
  if (this->ac_frequency_sensor_ != nullptr)
    this->query_list_.push_back(VALUE_F_AC);
  if (this->ac_voltage_phase1_sensor_ != nullptr)
    this->query_list_.push_back(VALUE_U_AC_1);
  if (this->ac_voltage_phase2_sensor_ != nullptr)
    this->query_list_.push_back(VALUE_U_AC_2);
  if (this->ac_voltage_phase3_sensor_ != nullptr)
    this->query_list_.push_back(VALUE_U_AC_3);
  if (this->ac_current_phase1_sensor_ != nullptr)
    this->query_list_.push_back(VALUE_I_AC_1);
  if (this->ac_current_phase2_sensor_ != nullptr)
    this->query_list_.push_back(VALUE_I_AC_2);
  if (this->ac_current_phase3_sensor_ != nullptr)
    this->query_list_.push_back(VALUE_I_AC_3);
  if (this->dc_voltage_sensor_ != nullptr)
    this->query_list_.push_back(VALUE_U_DC);
  if (this->dc_current_sensor_ != nullptr)
    this->query_list_.push_back(VALUE_I_DC);
  if (this->power_reduction_absolute_sensor_ != nullptr)
    this->query_list_.push_back(VALUE_RED_ABSOLUT);
  if (this->power_reduction_duration_sensor_ != nullptr)
    this->query_list_.push_back(VALUE_RED_ACTIV);
  if (this->power_reduction_relative_sensor_ != nullptr)
    this->query_list_.push_back(VALUE_RED_RELATIV);
  if (this->power_reduction_type_text_sensor_ != nullptr)
    this->query_list_.push_back(VALUE_RED_TYPE);
  if (this->temperature_1_sensor_ != nullptr)
    this->query_list_.push_back(VALUE_T_WR_1);
  if (this->temperature_2_sensor_ != nullptr)
    this->query_list_.push_back(VALUE_T_WR_2);
  if (this->temperature_3_sensor_ != nullptr)
    this->query_list_.push_back(VALUE_T_WR_3);
  if (this->temperature_4_sensor_ != nullptr)
    this->query_list_.push_back(VALUE_T_WR_4);
  if (this->temperature_5_sensor_ != nullptr)
    this->query_list_.push_back(VALUE_T_WR_5);
  if (this->temperature_6_sensor_ != nullptr)
    this->query_list_.push_back(VALUE_T_WR_6);
  if (this->insulation_resistance_sensor_ != nullptr)
    this->query_list_.push_back(VALUE_R_ISO);
  if (this->error_status_1_text_sensor_ != nullptr)
    this->query_list_.push_back(VALUE_E1);
  if (this->error_status_2_text_sensor_ != nullptr)
    this->query_list_.push_back(VALUE_E2);
  if (this->error_source_text_sensor_ != nullptr)
    this->query_list_.push_back(VALUE_E_S);
  if (this->serial_number_text_sensor_ != nullptr)
    this->query_list_.push_back(VALUE_SN);

  ESP_LOGD(TAG, "Configured %zu value queries", this->query_list_.size());

  // Flush any stale data in the UART RX buffer
  this->clear_rx_buffer_();
}

// ============================================================================
// Dump Config
// ============================================================================

void DiehlPlatinumComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "Diehl Platinum Inverter:");
  ESP_LOGCONFIG(TAG, "  Configured sensors: %zu", this->query_list_.size());
  LOG_UPDATE_INTERVAL(this);

  if (this->is_failed()) {
    ESP_LOGE(TAG, "  Communication with Diehl Platinum inverter FAILED!");
  }

  LOG_SENSOR("  ", "AC Power", this->ac_power_sensor_);
  LOG_SENSOR("  ", "DC Power", this->dc_power_sensor_);
  LOG_SENSOR("  ", "AC Voltage Phase 1", this->ac_voltage_phase1_sensor_);
  LOG_SENSOR("  ", "AC Voltage Phase 2", this->ac_voltage_phase2_sensor_);
  LOG_SENSOR("  ", "AC Voltage Phase 3", this->ac_voltage_phase3_sensor_);
  LOG_SENSOR("  ", "AC Current Phase 1", this->ac_current_phase1_sensor_);
  LOG_SENSOR("  ", "AC Current Phase 2", this->ac_current_phase2_sensor_);
  LOG_SENSOR("  ", "AC Current Phase 3", this->ac_current_phase3_sensor_);
  LOG_SENSOR("  ", "DC Voltage", this->dc_voltage_sensor_);
  LOG_SENSOR("  ", "DC Current", this->dc_current_sensor_);
  LOG_SENSOR("  ", "AC Frequency", this->ac_frequency_sensor_);
  LOG_SENSOR("  ", "Energy Today", this->energy_today_sensor_);
  LOG_SENSOR("  ", "Energy Total", this->energy_total_sensor_);
  LOG_SENSOR("  ", "Hours Total", this->hours_total_sensor_);
  LOG_SENSOR("  ", "Hours Today", this->hours_today_sensor_);
  LOG_SENSOR("  ", "Temperature 1", this->temperature_1_sensor_);
  LOG_SENSOR("  ", "Temperature 2", this->temperature_2_sensor_);
  LOG_SENSOR("  ", "Temperature 3", this->temperature_3_sensor_);
  LOG_SENSOR("  ", "Temperature 4", this->temperature_4_sensor_);
  LOG_SENSOR("  ", "Temperature 5", this->temperature_5_sensor_);
  LOG_SENSOR("  ", "Temperature 6", this->temperature_6_sensor_);
  LOG_SENSOR("  ", "Insulation Resistance", this->insulation_resistance_sensor_);
  LOG_SENSOR("  ", "Power Reduction Absolute", this->power_reduction_absolute_sensor_);
  LOG_SENSOR("  ", "Power Reduction Relative", this->power_reduction_relative_sensor_);
  LOG_SENSOR("  ", "Power Reduction Duration", this->power_reduction_duration_sensor_);
  LOG_TEXT_SENSOR("  ", "Operating State", this->operating_state_text_sensor_);
  LOG_TEXT_SENSOR("  ", "Serial Number", this->serial_number_text_sensor_);
  LOG_TEXT_SENSOR("  ", "Error Status 1", this->error_status_1_text_sensor_);
  LOG_TEXT_SENSOR("  ", "Error Status 2", this->error_status_2_text_sensor_);
  LOG_TEXT_SENSOR("  ", "Error Source", this->error_source_text_sensor_);
  LOG_TEXT_SENSOR("  ", "Power Reduction Type", this->power_reduction_type_text_sensor_);
  LOG_BINARY_SENSOR("  ", "Connection Status", this->connection_status_binary_sensor_);

  this->check_uart_settings(19200);
}

// ============================================================================
// Update (called by PollingComponent on the configured interval)
// ============================================================================

void DiehlPlatinumComponent::update() {
  if (this->query_list_.empty()) {
    ESP_LOGD(TAG, "No sensors configured, skipping update");
    return;
  }

  ESP_LOGD(TAG, "Starting update cycle, querying %zu values", this->query_list_.size());

  // Signal the loop() state machine to start querying
  this->update_requested_ = true;
  this->query_index_ = 0;
  this->comm_phase_ = CommPhase::QUERY_VALUES;
}

// ============================================================================
// Loop (non-blocking state machine for UART communication)
// ============================================================================

void DiehlPlatinumComponent::loop() {
  switch (this->comm_phase_) {
    case CommPhase::IDLE:
      // Nothing to do
      break;

    case CommPhase::QUERY_VALUES:
      // Send the next query
      if (this->query_index_ < this->query_list_.size()) {
        this->query_next_value_();
        this->comm_phase_ = CommPhase::WAIT_RESPONSE;
        this->last_send_time_ = millis();
      } else {
        // All queries sent and processed for this update cycle
        ESP_LOGD(TAG, "Update cycle complete");
        this->comm_phase_ = CommPhase::IDLE;
        this->update_requested_ = false;
      }
      break;

    case CommPhase::WAIT_RESPONSE:
      this->process_response_();
      break;
  }
}

// ============================================================================
// Query State Machine
// ============================================================================

void DiehlPlatinumComponent::query_next_value_() {
  DiehlValueType vtype = this->query_list_[this->query_index_];
  ESP_LOGD(TAG, "Querying value type 0x%02X (index %u/%zu)",
           static_cast<uint8_t>(vtype), this->query_index_ + 1, this->query_list_.size());

  this->clear_rx_buffer_();
  this->send_value_request_(vtype);
  this->rx_len_ = 0;
}

void DiehlPlatinumComponent::process_response_() {
  uint32_t elapsed = millis() - this->last_send_time_;

  // Try to read available bytes
  while (this->available() && this->rx_len_ < sizeof(this->rx_buffer_)) {
    uint8_t byte;
    if (this->read_byte(&byte)) {
      this->rx_buffer_[this->rx_len_++] = byte;
    }
  }

  // Check for timeout
  if (elapsed > RESPONSE_TIMEOUT_MS) {
    if (this->rx_len_ == 0) {
      ESP_LOGD(TAG, "Response timeout for value type 0x%02X (no data received)",
               static_cast<uint8_t>(this->query_list_[this->query_index_]));
      this->consecutive_errors_++;
    } else if (this->rx_len_ < MIN_RESPONSE_LEN) {
      ESP_LOGD(TAG, "Response too short for value type 0x%02X (%zu bytes)",
               static_cast<uint8_t>(this->query_list_[this->query_index_]), this->rx_len_);
      this->consecutive_errors_++;
    } else {
      // We have enough data, process it
      this->process_received_data_();
    }

    // Update connection status
    this->update_connection_status_();

    // Move to next query (with inter-query delay)
    this->query_index_++;
    this->comm_phase_ = CommPhase::QUERY_VALUES;
    return;
  }

  // If we have received enough bytes, check if the message is complete.
  // The Diehl protocol has a fixed structure: response contains a header, payload, and 2 CRC bytes.
  // For getValue responses, we expect at minimum MIN_RESPONSE_LEN bytes.
  // We use a heuristic: if no new data arrives for 50ms after first byte, assume message is complete.
  if (this->rx_len_ >= MIN_RESPONSE_LEN && !this->available()) {
    // Small delay to allow any trailing bytes to arrive
    if (elapsed > 100) {
      this->process_received_data_();
      this->update_connection_status_();
      this->query_index_++;
      this->comm_phase_ = CommPhase::QUERY_VALUES;
    }
  }
}

void DiehlPlatinumComponent::process_received_data_() {
  DiehlValueType vtype = this->query_list_[this->query_index_];

  // Log raw response at debug level
  if (this->rx_len_ > 0) {
    char hex_buf[sizeof(this->rx_buffer_) * 3 + 1];
    char *ptr = hex_buf;
    for (size_t i = 0; i < this->rx_len_; i++) {
      ptr += sprintf(ptr, "%02X ", this->rx_buffer_[i]);
    }
    ESP_LOGD(TAG, "RX [0x%02X]: %s(%zu bytes)", static_cast<uint8_t>(vtype), hex_buf, this->rx_len_);
  }

  // Check for error/end-of-list response
  if (this->rx_buffer_[0] == RESPONSE_END_OF_LIST) {
    ESP_LOGD(TAG, "Received error/end-of-list response for value type 0x%02X",
             static_cast<uint8_t>(vtype));
    this->consecutive_errors_++;
    return;
  }

  // Validate CRC
  if (!this->validate_checksum_(this->rx_buffer_, this->rx_len_)) {
    ESP_LOGD(TAG, "CRC mismatch for value type 0x%02X", static_cast<uint8_t>(vtype));
    this->consecutive_errors_++;
    return;
  }

  // Successful response — reset error counter
  this->consecutive_errors_ = 0;

  // Parse and publish the value based on type
  switch (vtype) {
    case VALUE_STATE:
      if (this->operating_state_text_sensor_ != nullptr && this->rx_len_ >= 5) {
        // Payload byte at index 3 contains the state code
        uint8_t state_code = this->rx_buffer_[3];
        const char *state_str = operating_state_to_string(state_code);
        ESP_LOGD(TAG, "Operating state: %s (code %u)", state_str, state_code);
        this->operating_state_text_sensor_->publish_state(state_str);
      }
      break;

    case VALUE_P_AC:
      if (this->ac_power_sensor_ != nullptr) {
        float val = this->parse_value_response_(this->rx_buffer_, this->rx_len_, vtype);
        ESP_LOGD(TAG, "AC Power: %.1f W", val);
        this->ac_power_sensor_->publish_state(val);
      }
      break;

    case VALUE_P_DC:
      if (this->dc_power_sensor_ != nullptr) {
        float val = this->parse_value_response_(this->rx_buffer_, this->rx_len_, vtype);
        ESP_LOGD(TAG, "DC Power: %.1f W", val);
        this->dc_power_sensor_->publish_state(val);
      }
      break;

    case VALUE_U_AC_1:
      if (this->ac_voltage_phase1_sensor_ != nullptr) {
        float val = this->parse_value_response_(this->rx_buffer_, this->rx_len_, vtype);
        ESP_LOGD(TAG, "AC Voltage Phase 1: %.1f V", val);
        this->ac_voltage_phase1_sensor_->publish_state(val);
      }
      break;

    case VALUE_U_AC_2:
      if (this->ac_voltage_phase2_sensor_ != nullptr) {
        float val = this->parse_value_response_(this->rx_buffer_, this->rx_len_, vtype);
        ESP_LOGD(TAG, "AC Voltage Phase 2: %.1f V", val);
        this->ac_voltage_phase2_sensor_->publish_state(val);
      }
      break;

    case VALUE_U_AC_3:
      if (this->ac_voltage_phase3_sensor_ != nullptr) {
        float val = this->parse_value_response_(this->rx_buffer_, this->rx_len_, vtype);
        ESP_LOGD(TAG, "AC Voltage Phase 3: %.1f V", val);
        this->ac_voltage_phase3_sensor_->publish_state(val);
      }
      break;

    case VALUE_I_AC_1:
      if (this->ac_current_phase1_sensor_ != nullptr) {
        float val = this->parse_value_response_(this->rx_buffer_, this->rx_len_, vtype);
        ESP_LOGD(TAG, "AC Current Phase 1: %.2f A", val);
        this->ac_current_phase1_sensor_->publish_state(val);
      }
      break;

    case VALUE_I_AC_2:
      if (this->ac_current_phase2_sensor_ != nullptr) {
        float val = this->parse_value_response_(this->rx_buffer_, this->rx_len_, vtype);
        ESP_LOGD(TAG, "AC Current Phase 2: %.2f A", val);
        this->ac_current_phase2_sensor_->publish_state(val);
      }
      break;

    case VALUE_I_AC_3:
      if (this->ac_current_phase3_sensor_ != nullptr) {
        float val = this->parse_value_response_(this->rx_buffer_, this->rx_len_, vtype);
        ESP_LOGD(TAG, "AC Current Phase 3: %.2f A", val);
        this->ac_current_phase3_sensor_->publish_state(val);
      }
      break;

    case VALUE_U_DC:
      if (this->dc_voltage_sensor_ != nullptr) {
        float val = this->parse_value_response_(this->rx_buffer_, this->rx_len_, vtype);
        ESP_LOGD(TAG, "DC Voltage: %.0f V", val);
        this->dc_voltage_sensor_->publish_state(val);
      }
      break;

    case VALUE_I_DC:
      if (this->dc_current_sensor_ != nullptr) {
        float val = this->parse_value_response_(this->rx_buffer_, this->rx_len_, vtype);
        ESP_LOGD(TAG, "DC Current: %.2f A", val);
        this->dc_current_sensor_->publish_state(val);
      }
      break;

    case VALUE_F_AC:
      if (this->ac_frequency_sensor_ != nullptr) {
        float val = this->parse_value_response_(this->rx_buffer_, this->rx_len_, vtype);
        ESP_LOGD(TAG, "AC Frequency: %.2f Hz", val);
        this->ac_frequency_sensor_->publish_state(val);
      }
      break;

    case VALUE_E_DAY:
      if (this->energy_today_sensor_ != nullptr) {
        float val = this->parse_value_response_(this->rx_buffer_, this->rx_len_, vtype);
        ESP_LOGD(TAG, "Energy Today: %.0f Wh", val);
        this->energy_today_sensor_->publish_state(val);
      }
      break;

    case VALUE_E_TOTAL:
      if (this->energy_total_sensor_ != nullptr) {
        float val = this->parse_value_response_(this->rx_buffer_, this->rx_len_, vtype);
        ESP_LOGD(TAG, "Energy Total: %.2f kWh", val);
        this->energy_total_sensor_->publish_state(val);
      }
      break;

    case VALUE_H_TOTAL:
      if (this->hours_total_sensor_ != nullptr) {
        float val = this->parse_value_response_(this->rx_buffer_, this->rx_len_, vtype);
        ESP_LOGD(TAG, "Hours Total: %.0f h", val);
        this->hours_total_sensor_->publish_state(val);
      }
      break;

    case VALUE_H_ON:
      if (this->hours_today_sensor_ != nullptr) {
        float val = this->parse_value_response_(this->rx_buffer_, this->rx_len_, vtype);
        ESP_LOGD(TAG, "Hours Today: %.2f h", val);
        this->hours_today_sensor_->publish_state(val);
      }
      break;

    case VALUE_T_WR_1:
      if (this->temperature_1_sensor_ != nullptr) {
        float val = this->parse_value_response_(this->rx_buffer_, this->rx_len_, vtype);
        ESP_LOGD(TAG, "Temperature 1: %.0f °C", val);
        this->temperature_1_sensor_->publish_state(val);
      }
      break;

    case VALUE_T_WR_2:
      if (this->temperature_2_sensor_ != nullptr) {
        float val = this->parse_value_response_(this->rx_buffer_, this->rx_len_, vtype);
        ESP_LOGD(TAG, "Temperature 2: %.0f °C", val);
        this->temperature_2_sensor_->publish_state(val);
      }
      break;

    case VALUE_T_WR_3:
      if (this->temperature_3_sensor_ != nullptr) {
        float val = this->parse_value_response_(this->rx_buffer_, this->rx_len_, vtype);
        ESP_LOGD(TAG, "Temperature 3: %.0f °C", val);
        this->temperature_3_sensor_->publish_state(val);
      }
      break;

    case VALUE_T_WR_4:
      if (this->temperature_4_sensor_ != nullptr) {
        float val = this->parse_value_response_(this->rx_buffer_, this->rx_len_, vtype);
        ESP_LOGD(TAG, "Temperature 4: %.0f °C", val);
        this->temperature_4_sensor_->publish_state(val);
      }
      break;

    case VALUE_T_WR_5:
      if (this->temperature_5_sensor_ != nullptr) {
        float val = this->parse_value_response_(this->rx_buffer_, this->rx_len_, vtype);
        ESP_LOGD(TAG, "Temperature 5: %.0f °C", val);
        this->temperature_5_sensor_->publish_state(val);
      }
      break;

    case VALUE_T_WR_6:
      if (this->temperature_6_sensor_ != nullptr) {
        float val = this->parse_value_response_(this->rx_buffer_, this->rx_len_, vtype);
        ESP_LOGD(TAG, "Temperature 6: %.0f °C", val);
        this->temperature_6_sensor_->publish_state(val);
      }
      break;

    case VALUE_R_ISO:
      if (this->insulation_resistance_sensor_ != nullptr) {
        float val = this->parse_value_response_(this->rx_buffer_, this->rx_len_, vtype);
        ESP_LOGD(TAG, "Insulation Resistance: %.0f Ohm", val);
        this->insulation_resistance_sensor_->publish_state(val);
      }
      break;

    case VALUE_RED_ABSOLUT:
      if (this->power_reduction_absolute_sensor_ != nullptr) {
        float val = this->parse_value_response_(this->rx_buffer_, this->rx_len_, vtype);
        ESP_LOGD(TAG, "Power Reduction Absolute: %.0f W", val);
        this->power_reduction_absolute_sensor_->publish_state(val);
      }
      break;

    case VALUE_RED_ACTIV:
      if (this->power_reduction_duration_sensor_ != nullptr) {
        float val = this->parse_value_response_(this->rx_buffer_, this->rx_len_, vtype);
        ESP_LOGD(TAG, "Power Reduction Duration: %.2f h", val);
        this->power_reduction_duration_sensor_->publish_state(val);
      }
      break;

    case VALUE_RED_RELATIV:
      if (this->power_reduction_relative_sensor_ != nullptr) {
        float val = this->parse_value_response_(this->rx_buffer_, this->rx_len_, vtype);
        ESP_LOGD(TAG, "Power Reduction Relative: %.1f %%", val);
        this->power_reduction_relative_sensor_->publish_state(val);
      }
      break;

    case VALUE_RED_TYPE:
      if (this->power_reduction_type_text_sensor_ != nullptr) {
        std::string val = this->parse_string_response_(this->rx_buffer_, this->rx_len_);
        ESP_LOGD(TAG, "Power Reduction Type: %s", val.c_str());
        this->power_reduction_type_text_sensor_->publish_state(val);
      }
      break;

    case VALUE_E1:
      if (this->error_status_1_text_sensor_ != nullptr) {
        std::string val = this->parse_string_response_(this->rx_buffer_, this->rx_len_);
        ESP_LOGD(TAG, "Error Status 1: %s", val.c_str());
        this->error_status_1_text_sensor_->publish_state(val);
      }
      break;

    case VALUE_E2:
      if (this->error_status_2_text_sensor_ != nullptr) {
        std::string val = this->parse_string_response_(this->rx_buffer_, this->rx_len_);
        ESP_LOGD(TAG, "Error Status 2: %s", val.c_str());
        this->error_status_2_text_sensor_->publish_state(val);
      }
      break;

    case VALUE_E_S:
      if (this->error_source_text_sensor_ != nullptr) {
        std::string val = this->parse_string_response_(this->rx_buffer_, this->rx_len_);
        ESP_LOGD(TAG, "Error Source: %s", val.c_str());
        this->error_source_text_sensor_->publish_state(val);
      }
      break;

    case VALUE_SN:
      if (this->serial_number_text_sensor_ != nullptr) {
        std::string val = this->parse_string_response_(this->rx_buffer_, this->rx_len_);
        ESP_LOGD(TAG, "Serial Number: %s", val.c_str());
        this->serial_number_text_sensor_->publish_state(val);
      }
      break;

    default:
      ESP_LOGD(TAG, "Unhandled value type 0x%02X", static_cast<uint8_t>(vtype));
      break;
  }
}

// ============================================================================
// Connection status tracking
// ============================================================================

void DiehlPlatinumComponent::update_connection_status_() {
  bool was_connected = this->connected_;

  if (this->consecutive_errors_ >= MAX_CONSECUTIVE_ERRORS) {
    this->connected_ = false;
  } else if (this->consecutive_errors_ == 0) {
    this->connected_ = true;
  }

  if (this->connection_status_binary_sensor_ != nullptr && was_connected != this->connected_) {
    this->connection_status_binary_sensor_->publish_state(this->connected_);
    if (this->connected_) {
      ESP_LOGD(TAG, "Connection to inverter established");
    } else {
      ESP_LOGD(TAG, "Connection to inverter lost (%" PRIu8 " consecutive errors)", this->consecutive_errors_);
    }
  }
}

// ============================================================================
// Protocol: Send value request
// ============================================================================

void DiehlPlatinumComponent::send_value_request_(DiehlValueType value_type) {
  uint8_t msg[6];
  msg[0] = CMD_GET_VALUE_HEADER_0;  // 0x35
  msg[1] = CMD_GET_VALUE_HEADER_1;  // 0x13
  msg[2] = CMD_GET_VALUE_LEN;       // 0x01
  msg[3] = static_cast<uint8_t>(value_type);

  uint16_t crc = calc_crc16(msg, 4);
  msg[4] = (crc >> 8) & 0xFF;  // CRC high byte
  msg[5] = crc & 0xFF;         // CRC low byte

  ESP_LOGD(TAG, "TX: %02X %02X %02X %02X %02X %02X", msg[0], msg[1], msg[2], msg[3], msg[4], msg[5]);

  this->write_array(msg, 6);
  this->flush();
}

// ============================================================================
// Protocol: Validate response checksum
// ============================================================================

bool DiehlPlatinumComponent::validate_checksum_(const uint8_t *buffer, size_t length) {
  if (length < 3) return false;

  uint16_t calc = calc_crc16(buffer, length - 2);
  uint16_t received = (static_cast<uint16_t>(buffer[length - 2]) << 8) | buffer[length - 1];

  if (calc != received) {
    ESP_LOGD(TAG, "CRC mismatch: calculated=0x%04X received=0x%04X", calc, received);
    return false;
  }

  ESP_LOGD(TAG, "CRC OK: 0x%04X", calc);
  return true;
}

// ============================================================================
// Protocol: Parse numeric value from response
// ============================================================================

float DiehlPlatinumComponent::parse_value_response_(const uint8_t *buffer, size_t length, DiehlValueType type) {
  // Response format: [echo_header, ...header bytes..., payload..., CRC_H, CRC_L]
  // The getValue response typically has the structure:
  //   Byte 0: Response type indicator
  //   Byte 1: Address (0x13 = 19)
  //   Byte 2: Payload length
  //   Byte 3..N: Payload data
  //   Byte N+1, N+2: CRC16
  //
  // The payload contains the requested value. Length and encoding depend on the value type.

  if (length < MIN_RESPONSE_LEN) {
    ESP_LOGD(TAG, "Response too short to parse: %zu bytes", length);
    return NAN;
  }

  uint8_t payload_len = buffer[2];
  const uint8_t *payload = &buffer[3];

  // Determine number of payload data bytes (excluding CRC)
  size_t data_bytes = length - 3 - 2;  // total - header(3) - crc(2)

  ESP_LOGD(TAG, "Parsing value type 0x%02X, declared payload_len=%u, actual data_bytes=%zu",
           static_cast<uint8_t>(type), payload_len, data_bytes);

  switch (type) {
    // ---- Power values: 2-byte big-endian, divided by 10 ----
    case VALUE_P_AC:
    case VALUE_P_DC:
    case VALUE_F_AC:
    case VALUE_RED_RELATIV:
      if (data_bytes >= 2) {
        uint16_t raw = (static_cast<uint16_t>(payload[0]) << 8) | payload[1];
        return static_cast<float>(raw) / 10.0f;
      }
      break;

    // ---- Voltage values: 2-byte big-endian, integer ----
    case VALUE_U_AC_1:
    case VALUE_U_AC_2:
    case VALUE_U_AC_3:
    case VALUE_U_DC:
    case VALUE_RED_ABSOLUT:
      if (data_bytes >= 2) {
        uint16_t raw = (static_cast<uint16_t>(payload[0]) << 8) | payload[1];
        return static_cast<float>(raw);
      }
      break;

    // ---- Current values: 1 or 2-byte, divided by 10 ----
    case VALUE_I_AC_1:
    case VALUE_I_AC_2:
    case VALUE_I_AC_3:
    case VALUE_I_DC:
      if (data_bytes >= 2) {
        uint16_t raw = (static_cast<uint16_t>(payload[0]) << 8) | payload[1];
        return static_cast<float>(raw) / 10.0f;
      } else if (data_bytes >= 1) {
        return static_cast<float>(payload[0]) / 10.0f;
      }
      break;

    // ---- Energy today: 4-byte big-endian, integer Wh ----
    case VALUE_E_DAY:
      if (data_bytes >= 4) {
        uint32_t raw = (static_cast<uint32_t>(payload[0]) << 24) |
                       (static_cast<uint32_t>(payload[1]) << 16) |
                       (static_cast<uint32_t>(payload[2]) << 8) |
                       payload[3];
        return static_cast<float>(raw);
      } else if (data_bytes >= 2) {
        uint16_t raw = (static_cast<uint16_t>(payload[0]) << 8) | payload[1];
        return static_cast<float>(raw);
      }
      break;

    // ---- Energy total: 4-byte big-endian, in Wh, convert to kWh ----
    case VALUE_E_TOTAL:
      if (data_bytes >= 4) {
        uint32_t raw = (static_cast<uint32_t>(payload[0]) << 24) |
                       (static_cast<uint32_t>(payload[1]) << 16) |
                       (static_cast<uint32_t>(payload[2]) << 8) |
                       payload[3];
        return static_cast<float>(raw) / 1000.0f;
      }
      break;

    // ---- Hours: 2-byte or 4-byte big-endian ----
    case VALUE_H_TOTAL:
      if (data_bytes >= 4) {
        uint32_t raw = (static_cast<uint32_t>(payload[0]) << 24) |
                       (static_cast<uint32_t>(payload[1]) << 16) |
                       (static_cast<uint32_t>(payload[2]) << 8) |
                       payload[3];
        return static_cast<float>(raw);
      } else if (data_bytes >= 2) {
        uint16_t raw = (static_cast<uint16_t>(payload[0]) << 8) | payload[1];
        return static_cast<float>(raw);
      }
      break;

    case VALUE_H_ON:
    case VALUE_RED_ACTIV:
      if (data_bytes >= 2) {
        uint16_t raw = (static_cast<uint16_t>(payload[0]) << 8) | payload[1];
        return static_cast<float>(raw) / 10.0f;
      }
      break;

    // ---- Temperature: 1-byte, integer °C ----
    case VALUE_T_WR_1:
    case VALUE_T_WR_2:
    case VALUE_T_WR_3:
    case VALUE_T_WR_4:
    case VALUE_T_WR_5:
    case VALUE_T_WR_6:
      if (data_bytes >= 1) {
        return static_cast<float>(payload[0]);
      }
      break;

    // ---- Insulation resistance: 2-byte big-endian ----
    case VALUE_R_ISO:
      if (data_bytes >= 2) {
        uint16_t raw = (static_cast<uint16_t>(payload[0]) << 8) | payload[1];
        return static_cast<float>(raw);
      }
      break;

    // ---- State: 1-byte ----
    case VALUE_STATE:
      if (data_bytes >= 1) {
        return static_cast<float>(payload[0]);
      }
      break;

    default:
      break;
  }

  ESP_LOGD(TAG, "Could not parse value for type 0x%02X (insufficient data)", static_cast<uint8_t>(type));
  return NAN;
}

// ============================================================================
// Protocol: Parse string value from response
// ============================================================================

std::string DiehlPlatinumComponent::parse_string_response_(const uint8_t *buffer, size_t length) {
  if (length < MIN_RESPONSE_LEN) return "";

  size_t data_bytes = length - 3 - 2;
  const uint8_t *payload = &buffer[3];

  // For numeric codes presented as text (error codes, state codes), format as decimal
  if (data_bytes == 1) {
    return str_snprintf("%u", 4, payload[0]);
  } else if (data_bytes == 2) {
    uint16_t raw = (static_cast<uint16_t>(payload[0]) << 8) | payload[1];
    return str_snprintf("%u", 6, raw);
  } else if (data_bytes >= 3) {
    // Could be a string (e.g., serial number) — try to interpret as ASCII
    bool is_ascii = true;
    for (size_t i = 0; i < data_bytes; i++) {
      if (payload[i] < 0x20 || payload[i] > 0x7E) {
        is_ascii = false;
        break;
      }
    }

    if (is_ascii) {
      return std::string(reinterpret_cast<const char *>(payload), data_bytes);
    }

    // Fall back to hex representation for binary serial numbers
    std::string result;
    for (size_t i = 0; i < data_bytes; i++) {
      if (i > 0) result += ":";
      char buf[4];
      sprintf(buf, "%02X", payload[i]);
      result += buf;
    }
    return result;
  }

  return "";
}

// ============================================================================
// Operating State to String
// ============================================================================

const char *DiehlPlatinumComponent::operating_state_to_string(uint8_t state) {
  switch (state) {
    case STATE_INIT:
      return "Initializing";
    case STATE_WAIT:
      return "Waiting";
    case STATE_CHK_DC:
      return "Checking DC";
    case STATE_CHK_AC:
      return "Checking AC";
    case STATE_FEED_IN:
      return "Feeding In";
    case STATE_REDUCE:
      return "Reduced Power";
    case STATE_COOL_DOWN:
      return "Cooling Down";
    case STATE_NIGHT:
      return "Night Mode";
    case STATE_ERROR:
      return "Error";
    case STATE_DERATING:
      return "Derating";
    default:
      return "Unknown";
  }
}

// ============================================================================
// UART Helpers
// ============================================================================

void DiehlPlatinumComponent::clear_rx_buffer_() {
  uint8_t tmp;
  while (this->available()) {
    this->read_byte(&tmp);
  }
}

}  // namespace diehl
}  // namespace esphome
