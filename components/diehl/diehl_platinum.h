#pragma once

#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/component.h"

namespace esphome {
namespace diehl {

/// CRC-16 lookup table used by the Diehl Platinum RS232 protocol.
static const uint16_t CRC16_TABLE[256] = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
    0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
    0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
    0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
    0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
    0x4864, 0x5845, 0x6826, 0x7807, 0x08E0, 0x18C1, 0x28A2, 0x38A3,
    0xC94C, 0xD96D, 0xE90E, 0xF92F, 0x89C8, 0x99E9, 0xA98A, 0xB9AB,
    0x5A75, 0x4A54, 0x7A37, 0x6A16, 0x1AF1, 0x0AD0, 0x3AB3, 0x2A92,
    0xDB7D, 0xCB5C, 0xFB3F, 0xEB1E, 0x9BF9, 0x8BD8, 0xBBBB, 0xAB9A,
    0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
    0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
    0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
    0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
    0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
    0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
    0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xB5EA, 0xA5CB, 0x95A8, 0x85A9, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
    0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
    0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xD9EC, 0xC9CD, 0xF9AE, 0xE98F, 0x9968, 0x8949, 0xB92A, 0xA90B,
    0x58E4, 0x48C5, 0x78A6, 0x6887, 0x1860, 0x0841, 0x3822, 0x2803,
    0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
    0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
    0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
    0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
    0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
    0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0,
};

/// Value type codes sent to the inverter to request specific single-value parameters.
enum DiehlValueType : uint8_t {
  VALUE_STATE = 0x00,
  VALUE_H_TOTAL = 0x01,
  VALUE_H_ON = 0x02,
  VALUE_E_TOTAL = 0x03,
  VALUE_E_DAY = 0x04,
  VALUE_P_AC = 0x05,
  VALUE_P_DC = 0x06,
  VALUE_F_AC = 0x07,
  VALUE_U_AC_1 = 0x08,
  VALUE_U_AC_2 = 0x09,
  VALUE_U_AC_3 = 0x0A,
  VALUE_I_AC_1 = 0x0B,
  VALUE_I_AC_2 = 0x0C,
  VALUE_I_AC_3 = 0x0D,
  VALUE_U_DC = 0x0E,
  VALUE_I_DC = 0x0F,
  VALUE_RED_ABSOLUT = 0x10,
  VALUE_RED_ACTIV = 0x11,
  VALUE_RED_RELATIV = 0x12,
  VALUE_RED_TYPE = 0x13,
  VALUE_T_WR_1 = 0x14,
  VALUE_T_WR_2 = 0x15,
  VALUE_T_WR_3 = 0x16,
  VALUE_T_WR_4 = 0x17,
  VALUE_T_WR_5 = 0x18,
  VALUE_T_WR_6 = 0x19,
  VALUE_R_ISO = 0x1A,
  VALUE_E1 = 0x1B,
  VALUE_E2 = 0x1C,
  VALUE_E_S = 0x1D,
  VALUE_SN = 0x1E,
};

/// Inverter operating state constants.
enum DiehlOperatingState : uint8_t {
  STATE_INIT = 0,
  STATE_WAIT = 1,
  STATE_CHK_DC = 10,
  STATE_CHK_AC = 11,
  STATE_FEED_IN = 31,
  STATE_REDUCE = 32,
  STATE_COOL_DOWN = 40,
  STATE_NIGHT = 50,
  STATE_ERROR = 60,
  STATE_DERATING = 70,
};

/// Communication phase state machine.
enum class CommPhase : uint8_t {
  IDLE,
  QUERY_VALUES,
  WAIT_RESPONSE,
};

/// The main Diehl Platinum inverter component.
class DiehlPlatinumComponent : public PollingComponent, public uart::UARTDevice {
 public:
  // --- Sensor setters ---
  void set_ac_power_sensor(sensor::Sensor *s) { ac_power_sensor_ = s; }
  void set_dc_power_sensor(sensor::Sensor *s) { dc_power_sensor_ = s; }
  void set_ac_voltage_phase1_sensor(sensor::Sensor *s) { ac_voltage_phase1_sensor_ = s; }
  void set_ac_voltage_phase2_sensor(sensor::Sensor *s) { ac_voltage_phase2_sensor_ = s; }
  void set_ac_voltage_phase3_sensor(sensor::Sensor *s) { ac_voltage_phase3_sensor_ = s; }
  void set_ac_current_phase1_sensor(sensor::Sensor *s) { ac_current_phase1_sensor_ = s; }
  void set_ac_current_phase2_sensor(sensor::Sensor *s) { ac_current_phase2_sensor_ = s; }
  void set_ac_current_phase3_sensor(sensor::Sensor *s) { ac_current_phase3_sensor_ = s; }
  void set_dc_voltage_sensor(sensor::Sensor *s) { dc_voltage_sensor_ = s; }
  void set_dc_current_sensor(sensor::Sensor *s) { dc_current_sensor_ = s; }
  void set_ac_frequency_sensor(sensor::Sensor *s) { ac_frequency_sensor_ = s; }
  void set_energy_today_sensor(sensor::Sensor *s) { energy_today_sensor_ = s; }
  void set_energy_total_sensor(sensor::Sensor *s) { energy_total_sensor_ = s; }
  void set_hours_total_sensor(sensor::Sensor *s) { hours_total_sensor_ = s; }
  void set_hours_today_sensor(sensor::Sensor *s) { hours_today_sensor_ = s; }
  void set_temperature_1_sensor(sensor::Sensor *s) { temperature_1_sensor_ = s; }
  void set_temperature_2_sensor(sensor::Sensor *s) { temperature_2_sensor_ = s; }
  void set_temperature_3_sensor(sensor::Sensor *s) { temperature_3_sensor_ = s; }
  void set_temperature_4_sensor(sensor::Sensor *s) { temperature_4_sensor_ = s; }
  void set_temperature_5_sensor(sensor::Sensor *s) { temperature_5_sensor_ = s; }
  void set_temperature_6_sensor(sensor::Sensor *s) { temperature_6_sensor_ = s; }
  void set_insulation_resistance_sensor(sensor::Sensor *s) { insulation_resistance_sensor_ = s; }
  void set_power_reduction_absolute_sensor(sensor::Sensor *s) { power_reduction_absolute_sensor_ = s; }
  void set_power_reduction_relative_sensor(sensor::Sensor *s) { power_reduction_relative_sensor_ = s; }
  void set_power_reduction_duration_sensor(sensor::Sensor *s) { power_reduction_duration_sensor_ = s; }

  // --- Text sensor setters ---
  void set_operating_state_text_sensor(text_sensor::TextSensor *s) { operating_state_text_sensor_ = s; }
  void set_serial_number_text_sensor(text_sensor::TextSensor *s) { serial_number_text_sensor_ = s; }
  void set_error_status_1_text_sensor(text_sensor::TextSensor *s) { error_status_1_text_sensor_ = s; }
  void set_error_status_2_text_sensor(text_sensor::TextSensor *s) { error_status_2_text_sensor_ = s; }
  void set_error_source_text_sensor(text_sensor::TextSensor *s) { error_source_text_sensor_ = s; }
  void set_power_reduction_type_text_sensor(text_sensor::TextSensor *s) { power_reduction_type_text_sensor_ = s; }

  // --- Binary sensor setters ---
  void set_connection_status_binary_sensor(binary_sensor::BinarySensor *s) { connection_status_binary_sensor_ = s; }

  // --- Component overrides ---
  void setup() override;
  void loop() override;
  void update() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

 protected:
  // --- Protocol helpers ---
  static uint16_t calc_crc16(const uint8_t *data, size_t len);
  void send_value_request_(DiehlValueType value_type);
  bool read_response_(uint8_t *buffer, size_t &length, uint32_t timeout_ms = 200);
  bool validate_checksum_(const uint8_t *buffer, size_t length);
  void clear_rx_buffer_();

  // --- Value parsing ---
  float parse_value_response_(const uint8_t *buffer, size_t length, DiehlValueType type);
  std::string parse_string_response_(const uint8_t *buffer, size_t length);

  // --- Query state machine ---
  void query_next_value_();
  void process_response_();
  void process_received_data_();
  void update_connection_status_();

  // --- State string conversion ---
  static const char *operating_state_to_string(uint8_t state);

  // --- Sensor pointers ---
  sensor::Sensor *ac_power_sensor_{nullptr};
  sensor::Sensor *dc_power_sensor_{nullptr};
  sensor::Sensor *ac_voltage_phase1_sensor_{nullptr};
  sensor::Sensor *ac_voltage_phase2_sensor_{nullptr};
  sensor::Sensor *ac_voltage_phase3_sensor_{nullptr};
  sensor::Sensor *ac_current_phase1_sensor_{nullptr};
  sensor::Sensor *ac_current_phase2_sensor_{nullptr};
  sensor::Sensor *ac_current_phase3_sensor_{nullptr};
  sensor::Sensor *dc_voltage_sensor_{nullptr};
  sensor::Sensor *dc_current_sensor_{nullptr};
  sensor::Sensor *ac_frequency_sensor_{nullptr};
  sensor::Sensor *energy_today_sensor_{nullptr};
  sensor::Sensor *energy_total_sensor_{nullptr};
  sensor::Sensor *hours_total_sensor_{nullptr};
  sensor::Sensor *hours_today_sensor_{nullptr};
  sensor::Sensor *temperature_1_sensor_{nullptr};
  sensor::Sensor *temperature_2_sensor_{nullptr};
  sensor::Sensor *temperature_3_sensor_{nullptr};
  sensor::Sensor *temperature_4_sensor_{nullptr};
  sensor::Sensor *temperature_5_sensor_{nullptr};
  sensor::Sensor *temperature_6_sensor_{nullptr};
  sensor::Sensor *insulation_resistance_sensor_{nullptr};
  sensor::Sensor *power_reduction_absolute_sensor_{nullptr};
  sensor::Sensor *power_reduction_relative_sensor_{nullptr};
  sensor::Sensor *power_reduction_duration_sensor_{nullptr};

  // --- Text sensor pointers ---
  text_sensor::TextSensor *operating_state_text_sensor_{nullptr};
  text_sensor::TextSensor *serial_number_text_sensor_{nullptr};
  text_sensor::TextSensor *error_status_1_text_sensor_{nullptr};
  text_sensor::TextSensor *error_status_2_text_sensor_{nullptr};
  text_sensor::TextSensor *error_source_text_sensor_{nullptr};
  text_sensor::TextSensor *power_reduction_type_text_sensor_{nullptr};

  // --- Binary sensor pointers ---
  binary_sensor::BinarySensor *connection_status_binary_sensor_{nullptr};

  // --- Internal state ---
  CommPhase comm_phase_{CommPhase::IDLE};
  bool update_requested_{false};
  uint32_t last_send_time_{0};
  uint8_t rx_buffer_[64]{};
  size_t rx_len_{0};
  uint8_t query_index_{0};
  uint8_t consecutive_errors_{0};
  bool connected_{false};

  /// List of value types to query, built dynamically based on configured sensors.
  std::vector<DiehlValueType> query_list_;
};

}  // namespace diehl
}  // namespace esphome
