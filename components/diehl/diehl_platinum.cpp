#include "diehl_platinum.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include <cinttypes>
#include <cstring>

namespace esphome {
namespace diehl {

static const char *const TAG = "diehl";

// --- Protocol constants ---
static const uint8_t CMD_GET_VALUE_HEADER = 0x35;  // 53 decimal
static const uint8_t CMD_ADDR = 0x13;              // 19 decimal — inverter address
static const uint8_t RESPONSE_END_OF_LIST = 0x84;  // 132 decimal

// Valid response header bytes the inverter can send as byte 0.
// getValue echo/response starts with 0x35; error/end with 0x84;
// init response may start with 0xC5 or other.
// Crucially, 0x00 is NEVER a valid start byte.
static inline bool is_valid_header_byte(uint8_t b) {
  return b != 0x00;
}

// Minimum valid data response: header(3) + at least 1 payload + CRC(2) = 6
static const size_t MIN_RESPONSE_LEN = 6;

// Timeouts (ms)
static const uint32_t RESPONSE_TIMEOUT_MS = 800;
static const uint32_t INIT_TIMEOUT_MS = 2000;
// Max consecutive errors before declaring disconnected
static const uint8_t MAX_CONSECUTIVE_ERRORS = 5;
// Max init retries before giving up for this update cycle
static const uint8_t MAX_INIT_RETRIES = 3;

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
// Logging helper
// ============================================================================

void DiehlPlatinumComponent::log_hex_(const char *prefix, const uint8_t *data, size_t len) {
  if (len == 0) return;
  char hex_buf[3 * 64 + 1];
  char *ptr = hex_buf;
  size_t max_len = (len > 64) ? 64 : len;
  for (size_t i = 0; i < max_len; i++) {
    ptr += sprintf(ptr, "%02X ", data[i]);
  }
  ESP_LOGD(TAG, "%s: %s(%zu bytes)", prefix, hex_buf, len);
}

// ============================================================================
// Drain noise / zero bytes from RX
// ============================================================================

void DiehlPlatinumComponent::drain_rx_noise_() {
  uint8_t tmp;
  uint32_t drained = 0;
  while (this->available()) {
    this->read_byte(&tmp);
    drained++;
  }
  if (drained > 0) {
    ESP_LOGD(TAG, "Drained %u bytes of RX noise before transmit", (unsigned) drained);
  }
}

// ============================================================================
// Setup
// ============================================================================

void DiehlPlatinumComponent::setup() {
  ESP_LOGD(TAG, "Setting up Diehl Platinum inverter component...");

  // Build the query list based on which sensors are configured.
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

  // Publish initial disconnected state
  if (this->connection_status_binary_sensor_ != nullptr) {
    this->connection_status_binary_sensor_->publish_state(false);
  }

  this->drain_rx_noise_();
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
// Update
// ============================================================================

void DiehlPlatinumComponent::update() {
  if (this->query_list_.empty()) {
    ESP_LOGD(TAG, "No sensors configured, skipping update");
    return;
  }

  if (this->comm_phase_ != CommPhase::IDLE) {
    ESP_LOGD(TAG, "Previous update cycle still running, skipping");
    return;
  }

  ESP_LOGD(TAG, "Starting update cycle, querying %zu values", this->query_list_.size());

  this->query_index_ = 0;
  this->comm_phase_ = CommPhase::INIT_SEND;
}

// ============================================================================
// Loop
// ============================================================================

void DiehlPlatinumComponent::loop() {
  switch (this->comm_phase_) {
    case CommPhase::IDLE:
      break;

    case CommPhase::INIT_SEND:
      this->send_init_handshake_();
      this->comm_phase_ = CommPhase::INIT_WAIT_RESPONSE;
      break;

    case CommPhase::INIT_WAIT_RESPONSE:
      this->handle_init_wait_();
      break;

    case CommPhase::QUERY_VALUES:
      if (this->query_index_ < this->query_list_.size()) {
        this->query_next_value_();
      } else {
        ESP_LOGD(TAG, "Update cycle complete");
        this->comm_phase_ = CommPhase::IDLE;
      }
      break;

    case CommPhase::WAIT_RESPONSE:
      this->handle_wait_response_();
      break;
  }
}

// ============================================================================
// Init handshake
// ============================================================================

void DiehlPlatinumComponent::send_init_handshake_() {
  ESP_LOGD(TAG, "Sending init handshake to inverter...");

  this->drain_rx_noise_();

  // Init frame: 0xC5, 0x13, 0x03, 0x00, 0x00, 0xC3, CRC_H, CRC_L
  uint8_t init_frame[8];
  init_frame[0] = 0xC5;
  init_frame[1] = CMD_ADDR;
  init_frame[2] = 0x03;
  init_frame[3] = 0x00;
  init_frame[4] = 0x00;
  init_frame[5] = 0xC3;

  uint16_t crc = calc_crc16(init_frame, 6);
  init_frame[6] = (crc >> 8) & 0xFF;
  init_frame[7] = crc & 0xFF;

  this->log_hex_("TX INIT", init_frame, 8);
  this->write_array(init_frame, 8);
  this->flush();

  this->rx_len_ = 0;
  this->rx_synced_ = false;
  this->noise_zeros_discarded_ = 0;
  this->last_send_time_ = millis();
}

// ============================================================================
// Wait for init response
// ============================================================================

void DiehlPlatinumComponent::handle_init_wait_() {
  uint32_t elapsed = millis() - this->last_send_time_;

  while (this->available()) {
    uint8_t byte;
    if (!this->read_byte(&byte)) break;

    // Discard 0x00 noise bytes until we see a valid header
    if (!this->rx_synced_) {
      if (!is_valid_header_byte(byte)) {
        this->noise_zeros_discarded_++;
        continue;
      }
      // Found first valid byte — we are synced
      this->rx_synced_ = true;
      if (this->noise_zeros_discarded_ > 0) {
        ESP_LOGD(TAG, "Init: discarded %u noise zero-bytes before sync",
                 (unsigned) this->noise_zeros_discarded_);
      }
    }

    if (this->rx_len_ < sizeof(this->rx_buffer_)) {
      this->rx_buffer_[this->rx_len_++] = byte;
    }
  }

  // If we have synced data, check if response is complete
  if (this->rx_synced_ && this->rx_len_ >= 3) {
    size_t expected_len = 3 + this->rx_buffer_[2] + 2;

    if (this->rx_len_ >= expected_len) {
      this->log_hex_("INIT RX", this->rx_buffer_, this->rx_len_);

      // Validate CRC on init response
      if (this->validate_checksum_(this->rx_buffer_, this->rx_len_)) {
        ESP_LOGD(TAG, "Init handshake successful, inverter responded with valid frame");
        this->init_retry_count_ = 0;
        this->consecutive_errors_ = 0;
        this->update_connection_status_();
        this->comm_phase_ = CommPhase::QUERY_VALUES;
        return;
      } else {
        ESP_LOGD(TAG, "Init response CRC invalid, but inverter is responding — proceeding");
        this->init_retry_count_ = 0;
        this->comm_phase_ = CommPhase::QUERY_VALUES;
        return;
      }
    }
  }

  // Also accept 0x84 error response (1 byte, means inverter is alive but busy)
  if (this->rx_synced_ && this->rx_len_ >= 1 && this->rx_buffer_[0] == RESPONSE_END_OF_LIST) {
    ESP_LOGD(TAG, "Init: inverter responded with 0x84 (busy/error), but it's alive — proceeding");
    this->init_retry_count_ = 0;
    this->comm_phase_ = CommPhase::QUERY_VALUES;
    return;
  }

  // Timeout
  if (elapsed > INIT_TIMEOUT_MS) {
    this->init_retry_count_++;

    if (this->noise_zeros_discarded_ > 0 && !this->rx_synced_) {
      ESP_LOGD(TAG, "Init timeout: received only %u zero-bytes (line noise), no valid data (attempt %" PRIu8 "/%" PRIu8 ")",
               (unsigned) this->noise_zeros_discarded_, this->init_retry_count_, MAX_INIT_RETRIES);
    } else if (this->rx_len_ > 0) {
      this->log_hex_("INIT RX (incomplete)", this->rx_buffer_, this->rx_len_);
      ESP_LOGD(TAG, "Init timeout with partial data (attempt %" PRIu8 "/%" PRIu8 ")",
               this->init_retry_count_, MAX_INIT_RETRIES);
    } else {
      ESP_LOGD(TAG, "Init timeout, no response at all (attempt %" PRIu8 "/%" PRIu8 ")",
               this->init_retry_count_, MAX_INIT_RETRIES);
    }

    if (this->init_retry_count_ < MAX_INIT_RETRIES) {
      this->comm_phase_ = CommPhase::INIT_SEND;
    } else {
      ESP_LOGD(TAG, "Init handshake failed after %" PRIu8 " attempts — inverter not responding", MAX_INIT_RETRIES);
      this->init_retry_count_ = 0;
      this->consecutive_errors_ = MAX_CONSECUTIVE_ERRORS;
      this->update_connection_status_();
      this->comm_phase_ = CommPhase::IDLE;
    }
  }
}

// ============================================================================
// Send value request
// ============================================================================

void DiehlPlatinumComponent::query_next_value_() {
  DiehlValueType vtype = this->query_list_[this->query_index_];
  ESP_LOGD(TAG, "Querying value type 0x%02X (index %u/%zu)",
           static_cast<uint8_t>(vtype), this->query_index_ + 1, this->query_list_.size());

  this->drain_rx_noise_();

  uint8_t msg[6];
  msg[0] = CMD_GET_VALUE_HEADER;  // 0x35
  msg[1] = CMD_ADDR;              // 0x13
  msg[2] = 0x01;                  // payload len = 1
  msg[3] = static_cast<uint8_t>(vtype);
  uint16_t crc = calc_crc16(msg, 4);
  msg[4] = (crc >> 8) & 0xFF;
  msg[5] = crc & 0xFF;

  this->log_hex_("TX", msg, 6);
  this->write_array(msg, 6);
  this->flush();

  this->rx_len_ = 0;
  this->rx_synced_ = false;
  this->noise_zeros_discarded_ = 0;
  this->last_send_time_ = millis();
  this->comm_phase_ = CommPhase::WAIT_RESPONSE;
}

// ============================================================================
// Wait for value response
// ============================================================================

void DiehlPlatinumComponent::handle_wait_response_() {
  uint32_t elapsed = millis() - this->last_send_time_;

  while (this->available()) {
    uint8_t byte;
    if (!this->read_byte(&byte)) break;

    // Discard 0x00 noise bytes until we see a valid protocol header
    if (!this->rx_synced_) {
      if (!is_valid_header_byte(byte)) {
        this->noise_zeros_discarded_++;
        continue;
      }
      this->rx_synced_ = true;
      if (this->noise_zeros_discarded_ > 0) {
        ESP_LOGD(TAG, "Discarded %u noise zero-bytes before response",
                 (unsigned) this->noise_zeros_discarded_);
      }
    }

    if (this->rx_len_ < sizeof(this->rx_buffer_)) {
      this->rx_buffer_[this->rx_len_++] = byte;
    }
  }

  // Check for 0x84 error/end-of-list (single byte)
  if (this->rx_synced_ && this->rx_len_ >= 1 && this->rx_buffer_[0] == RESPONSE_END_OF_LIST) {
    ESP_LOGD(TAG, "Received 0x84 (error/unsupported) for value type 0x%02X",
             static_cast<uint8_t>(this->query_list_[this->query_index_]));
    // 0x84 means the inverter IS responding, just doesn't support this value
    // Don't count this as a connection error
    this->advance_to_next_query_();
    return;
  }

  // Once we have the header, check for complete frame
  if (this->rx_synced_ && this->rx_len_ >= 3) {
    size_t expected_len = 3 + this->rx_buffer_[2] + 2;

    // Sanity check: payload_len shouldn't be absurd
    if (this->rx_buffer_[2] > 50) {
      ESP_LOGD(TAG, "Suspicious payload length %u for 0x%02X, discarding",
               this->rx_buffer_[2], static_cast<uint8_t>(this->query_list_[this->query_index_]));
      this->consecutive_errors_++;
      this->update_connection_status_();
      this->advance_to_next_query_();
      return;
    }

    if (this->rx_len_ >= expected_len) {
      this->log_hex_("RX", this->rx_buffer_, this->rx_len_);
      this->process_received_data_();
      this->update_connection_status_();
      this->advance_to_next_query_();
      return;
    }
  }

  // Timeout
  if (elapsed > RESPONSE_TIMEOUT_MS) {
    if (this->rx_synced_ && this->rx_len_ > 0) {
      this->log_hex_("RX (timeout, incomplete)", this->rx_buffer_, this->rx_len_);

      if (this->rx_len_ >= MIN_RESPONSE_LEN) {
        this->process_received_data_();
      } else {
        ESP_LOGD(TAG, "Response too short (%zu bytes) for 0x%02X",
                 this->rx_len_, static_cast<uint8_t>(this->query_list_[this->query_index_]));
        this->consecutive_errors_++;
      }
    } else {
      if (this->noise_zeros_discarded_ > 0) {
        ESP_LOGD(TAG, "Response timeout for 0x%02X (only %u zero-bytes received = line noise)",
                 static_cast<uint8_t>(this->query_list_[this->query_index_]),
                 (unsigned) this->noise_zeros_discarded_);
      } else {
        ESP_LOGD(TAG, "Response timeout for 0x%02X (no data at all)",
                 static_cast<uint8_t>(this->query_list_[this->query_index_]));
      }
      this->consecutive_errors_++;
    }

    this->update_connection_status_();
    this->advance_to_next_query_();
  }
}

// ============================================================================
// Advance to next query
// ============================================================================

void DiehlPlatinumComponent::advance_to_next_query_() {
  this->query_index_++;
  this->comm_phase_ = CommPhase::QUERY_VALUES;
}

// ============================================================================
// Process received data
// ============================================================================

void DiehlPlatinumComponent::process_received_data_() {
  DiehlValueType vtype = this->query_list_[this->query_index_];

  if (this->rx_len_ >= 1 && this->rx_buffer_[0] == RESPONSE_END_OF_LIST) {
    ESP_LOGD(TAG, "Error response for value type 0x%02X", static_cast<uint8_t>(vtype));
    return;
  }

  // Validate CRC
  if (!this->validate_checksum_(this->rx_buffer_, this->rx_len_)) {
    ESP_LOGD(TAG, "CRC mismatch for value type 0x%02X", static_cast<uint8_t>(vtype));
    this->consecutive_errors_++;
    return;
  }

  // Success — reset error counter
  this->consecutive_errors_ = 0;

  switch (vtype) {
    case VALUE_STATE:
      if (this->operating_state_text_sensor_ != nullptr && this->rx_len_ >= 6) {
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
        ESP_LOGD(TAG, "Temperature 1: %.0f C", val);
        this->temperature_1_sensor_->publish_state(val);
      }
      break;

    case VALUE_T_WR_2:
      if (this->temperature_2_sensor_ != nullptr) {
        float val = this->parse_value_response_(this->rx_buffer_, this->rx_len_, vtype);
        ESP_LOGD(TAG, "Temperature 2: %.0f C", val);
        this->temperature_2_sensor_->publish_state(val);
      }
      break;

    case VALUE_T_WR_3:
      if (this->temperature_3_sensor_ != nullptr) {
        float val = this->parse_value_response_(this->rx_buffer_, this->rx_len_, vtype);
        ESP_LOGD(TAG, "Temperature 3: %.0f C", val);
        this->temperature_3_sensor_->publish_state(val);
      }
      break;

    case VALUE_T_WR_4:
      if (this->temperature_4_sensor_ != nullptr) {
        float val = this->parse_value_response_(this->rx_buffer_, this->rx_len_, vtype);
        ESP_LOGD(TAG, "Temperature 4: %.0f C", val);
        this->temperature_4_sensor_->publish_state(val);
      }
      break;

    case VALUE_T_WR_5:
      if (this->temperature_5_sensor_ != nullptr) {
        float val = this->parse_value_response_(this->rx_buffer_, this->rx_len_, vtype);
        ESP_LOGD(TAG, "Temperature 5: %.0f C", val);
        this->temperature_5_sensor_->publish_state(val);
      }
      break;

    case VALUE_T_WR_6:
      if (this->temperature_6_sensor_ != nullptr) {
        float val = this->parse_value_response_(this->rx_buffer_, this->rx_len_, vtype);
        ESP_LOGD(TAG, "Temperature 6: %.0f C", val);
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
// Validate CRC
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
// Parse numeric value from response
// ============================================================================

float DiehlPlatinumComponent::parse_value_response_(const uint8_t *buffer, size_t length, DiehlValueType type) {
  if (length < MIN_RESPONSE_LEN) {
    ESP_LOGD(TAG, "Response too short to parse: %zu bytes", length);
    return NAN;
  }

  uint8_t payload_len = buffer[2];
  const uint8_t *payload = &buffer[3];
  size_t data_bytes = length - 3 - 2;

  ESP_LOGD(TAG, "Parsing 0
