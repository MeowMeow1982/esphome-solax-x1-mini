#include "solax_x1.h"
#include "esphome/core/log.h"

namespace esphome {
namespace solax_x1 {

static const char *const TAG = "solax_x1";

static const uint8_t FUNCTION_DEVICE_INFO = 0x82;
static const uint8_t FUNCTION_STATUS_REPORT = 0x83;
static const uint8_t FUNCTION_CONFIG_SETTINGS = 0x84;

// SolaxPower Single Phase External Communication Protocol - X1 Series V1.2.pdf
// SolaxPower Single Phase External Communication Protocol - X1 Series V1.8.pdf
static const std::string MODE_NAMES[7] = {
    "Wait",             // 0
    "Check",            // 1
    "Normal",           // 2
    "Fault",            // 3
    "Permanent Fault",  // 4
    "Update",           // 5
    "Self Test",        // 6
};

static const char *const ERRORS[32] = {
    "Tz Protection Fault",   // Byte 0.0
    "Mains Lost Fault",      // Byte 0.1
    "Grid Voltage Fault",    // Byte 0.2
    "Grid Frequency Fault",  // Byte 0.3
    "PLL Lost Fault",        // Byte 0.4
    "Bus Voltage Fault",     // Byte 0.5
    "Error Bit 06",          // Byte 0.6
    "Oscillator Fault",      // Byte 0.7

    "DCI OCP Fault",           // Byte 1.0
    "Residual Current Fault",  // Byte 1.1
    "PV Voltage Fault",        // Byte 1.2
    "Ac10Mins Voltage Fault",  // Byte 1.3
    "Isolation Fault",         // Byte 1.4
    "Over Temperature Fault",  // Byte 1.5
    "Ventilator Fault",        // Byte 1.6
    "Error Bit 15",            // Byte 1.7

    "SPI Communication Fault",        // Byte 2.0
    "SCI Communication Fault",        // Byte 2.1
    "Error Bit 18",                   // Byte 2.2
    "Input Configuration Fault",      // Byte 2.3
    "EEPROM Fault",                   // Byte 2.4
    "Relay Fault",                    // Byte 2.5
    "Sample Consistence Fault",       // Byte 2.6
    "Residual-Current Device Fault",  // Byte 2.7

    "Error Bit 24",        // Byte 3.0
    "Error Bit 25",        // Byte 3.1
    "Error Bit 26",        // Byte 3.2
    "Error Bit 27",        // Byte 3.3
    "Error Bit 28",        // Byte 3.4
    "DCI Device Fault",    // Byte 3.5
    "Other Device Fault",  // Byte 3.6
    "Error Bit 31",        // Byte 3.7
};

void SolaxX1::on_modbus_solax_data(const uint8_t &function, const std::vector<uint8_t> &data) {
  switch (function) {
    case FUNCTION_DEVICE_INFO:
      this->decode_device_info_(data);
      break;
    case FUNCTION_STATUS_REPORT:
      this->decode_status_report_(data);
      break;
    case FUNCTION_CONFIG_SETTINGS:
      this->decode_config_settings_(data);
      break;
    default:
      ESP_LOGW(TAG, "Unhandled solax frame: %s", format_hex_pretty(&data.front(), data.size()).c_str());
  }
}

void SolaxX1::decode_device_info_(const std::vector<uint8_t> &data) {
  if (data.size() != 58) {
    ESP_LOGW(TAG, "Invalid response size: %zu", data.size());
    return;
  }

  ESP_LOGI(TAG, "Device info frame received");
  ESP_LOGI(TAG, "  Device type: %d", data[0]);
  ESP_LOGI(TAG, "  Rated power: %s", std::string(data.begin() + 1, data.begin() + 1 + 6).c_str());
  ESP_LOGI(TAG, "  Firmware version: %s", std::string(data.begin() + 7, data.begin() + 7 + 5).c_str());
  ESP_LOGI(TAG, "  Module name: %s", std::string(data.begin() + 12, data.begin() + 12 + 14).c_str());
  ESP_LOGI(TAG, "  Manufacturer: %s", std::string(data.begin() + 26, data.begin() + 26 + 14).c_str());
  ESP_LOGI(TAG, "  Serial number: %s", std::string(data.begin() + 40, data.begin() + 40 + 14).c_str());
  ESP_LOGI(TAG, "  Rated bus voltage: %s", std::string(data.begin() + 54, data.begin() + 54 + 4).c_str());

  this->no_response_count_ = 0;
}

void SolaxX1::decode_config_settings_(const std::vector<uint8_t> &data) {
  if (data.size() != 68) {
    ESP_LOGW(TAG, "Invalid response size: %zu", data.size());
    return;
  }

  auto solax_get_16bit = [&](size_t i) -> uint16_t {
    return (uint16_t(data[i + 0]) << 8) | (uint16_t(data[i + 1]) << 0);
  };

  ESP_LOGI(TAG, "Config settings frame received");
  ESP_LOGI(TAG, "  wVpvStart [9.10]: %f V", solax_get_16bit(0) * 0.1f);
  ESP_LOGI(TAG, "  wTimeStart [11.12]: %d S", solax_get_16bit(2));
  ESP_LOGI(TAG, "  wVacMinProtect [13.14]: %f V", solax_get_16bit(4) * 0.1f);
  ESP_LOGI(TAG, "  wVacMaxProtect [15.16]: %f V", solax_get_16bit(6) * 0.1f);
  ESP_LOGI(TAG, "  wFacMinProtect [17.18]: %f Hz", solax_get_16bit(8) * 0.01f);
  ESP_LOGI(TAG, "  wFacMaxProtect [19.20]: %f Hz", solax_get_16bit(10) * 0.01f);
  ESP_LOGI(TAG, "  wDciLimits [21.22]: %d mA", solax_get_16bit(12));
  ESP_LOGI(TAG, "  wGrid10MinAvgProtect [23,24]: %f V", solax_get_16bit(14) * 0.1f);
  ESP_LOGI(TAG, "  wVacMinSlowProtect [25.26]: %f V", solax_get_16bit(16) * 0.1f);
  ESP_LOGI(TAG, "  wVacMaxSlowProtect [27.28]: %f V", solax_get_16bit(18) * 0.1f);
  ESP_LOGI(TAG, "  wFacMinSlowProtect [29.30]: %f Hz", solax_get_16bit(20) * 0.01f);
  ESP_LOGI(TAG, "  wFacMaxSlowProtect [31.32]: %f Hz", solax_get_16bit(22) * 0.01f);
  ESP_LOGI(TAG, "  wSafety [33.34]: %d", solax_get_16bit(24));
  // Supported safety values:
  //
  // 0: VDE0126
  // 1: VDE4105
  // 2: AS4777
  // 3: G98
  // 4: C10_11
  // 5: TOR
  // 6: EN50438_NL
  // 7: Denmark2019_W
  // 8: CEB
  // 9: Cyprus2019
  // 10: cNRS097_2_1
  // 11: VDE0126_Greece
  // 12: UTE_C15_712_Fr
  // 13: IEC61727
  // 14: G99
  // 15: CQC
  // 16: VDE0126_Greece_is
  // 17: C15_712_Fr_island_50
  // 18: C15_712_Fr_island_60
  // 19: Guyana
  // 20: MEA_Thailand
  // 21: PEA_Thailand
  // 22: cNewZealand
  // 23: cIreland
  // 24: cCE10_21
  // 25: cRD1699
  // 26: EN50438_Sweden
  // 27: EN50549_PL
  // 28: Czech PPDS
  // 29: EN50438_Norway
  // 30: EN50438_Portug
  // 31: cCQC_WideRange
  // 32: BRAZIL
  // 33: EN50438_CEZ
  // 34: IEC_Chile
  // 35: Sri_Lanka
  // 36: BRAZIL_240
  // 37: EN50549-SK
  // 38: EN50549_EU
  // 39: G98/NI
  // 40: Denmark2019_E
  // 41: RD1699_island
  ESP_LOGI(TAG, "  wPowerfactor_mode [35]: %d", data[26]);
  ESP_LOGI(TAG, "  wPowerfactor_data [36]: %d", data[27]);
  ESP_LOGI(TAG, "  wUpperLimit [37]: %d", data[28]);
  ESP_LOGI(TAG, "  wLowerLimit [38]: %d", data[29]);
  ESP_LOGI(TAG, "  wPowerLow [39]: %d", data[30]);
  ESP_LOGI(TAG, "  wPowerUp [40]: %d", data[31]);
  ESP_LOGI(TAG, "  Qpower_set [41.42]: %d", solax_get_16bit(32));
  ESP_LOGI(TAG, "  WFreqSetPoint [43.44]: %f Hz", solax_get_16bit(34) * 0.01f);
  ESP_LOGI(TAG, "  WFreqDropRate [45.46]: %d", solax_get_16bit(36));
  ESP_LOGI(TAG, "  QuVupRate [47.48]: %d", solax_get_16bit(38));
  ESP_LOGI(TAG, "  QuVlowRate [49.50]: %d", solax_get_16bit(40));
  ESP_LOGI(TAG, "  WPowerLimitsPercent [51.52]: %d", solax_get_16bit(42));
  ESP_LOGI(TAG, "  WWgra [53.54]: %f %%", solax_get_16bit(44) * 0.01f);
  ESP_LOGI(TAG, "  wWv2 [55.56]: %f V", solax_get_16bit(46) * 0.1f);
  ESP_LOGI(TAG, "  wWv3 [57.58]: %f V", solax_get_16bit(48) * 0.1f);
  ESP_LOGI(TAG, "  wWv4 [59.60]: %f V", solax_get_16bit(50) * 0.1f);
  ESP_LOGI(TAG, "  wQurangeV1 [61.62]: %d %%", solax_get_16bit(52));
  ESP_LOGI(TAG, "  wQurangeV4 [63.64]: %d %%", solax_get_16bit(54));
  ESP_LOGI(TAG, "  BVoltPowerLimit [65.66]: %d", solax_get_16bit(56));
  ESP_LOGI(TAG, "  WPowerManagerEnable [67.68]: %d", solax_get_16bit(58));
  ESP_LOGI(TAG, "  WGlobalSearchMPPTStartFlag [69.70]: %d", solax_get_16bit(60));
  ESP_LOGI(TAG, "  WFreqProtectRestrictive [71.72]: %d", solax_get_16bit(62));
  ESP_LOGI(TAG, "  WQuDelayTimer [73.74]: %d S", solax_get_16bit(64));
  ESP_LOGI(TAG, "  WFreqActivePowerDelayTimer [75.76]: %d ms", solax_get_16bit(66));

  this->no_response_count_ = 0;
}

void SolaxX1::decode_status_report_(const std::vector<uint8_t> &data) {
  if (data.size() != 52 && data.size() != 50 && data.size() != 56) {
    // Solax X1 mini status report (data_len 0x34: 52 bytes):
    // AA.55.00.0A.01.00.11.82.34.00.1A.00.02.00.00.00.00.00.00.00.00.00.00.09.21.13.87.00.00.FF.FF.
    // 00.00.00.12.00.00.00.15.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.04.D6

    // Solax X1 mini g2 status report (data_len 0x32: 50 bytes):
    // AA.55.00.0A.01.00.11.82.32.00.21.00.02.07.EC.00.00.00.1D.00.00.00.18.09.55.13.80.02.2B.FF.FF.
    // 00.00.5D.AF.00.00.10.50.00.02.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.07.A4

    // Solax X1 mini g3 status report (data_len 0x38: 56 bytes):
    // AA.55.00.0A.01.00.11.82.38.00.1A.00.03.04.0C.00.00.00.19.00.00.00.0B.08.FC.13.8A.00.F8.FF.FF.
    // 00.00.00.2B.00.00.00.0D.00.02.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.8A.00.DE.08.5F
    ESP_LOGW(TAG, "Invalid response size: %zu", data.size());
    ESP_LOGW(TAG, "Your device is probably not supported. Please create an issue here: "
                  "https://github.com/syssi/esphome-modbus-solax-x1/issues");
    ESP_LOGW(TAG, "Please provide the following status response data: %s",
             format_hex_pretty(&data.front(), data.size()).c_str());
    return;
  }

  auto solax_get_16bit = [&](size_t i) -> uint16_t {
    return (uint16_t(data[i + 0]) << 8) | (uint16_t(data[i + 1]) << 0);
  };
  auto solax_get_32bit = [&](size_t i) -> uint32_t {
    return uint32_t((data[i] << 24) | (data[i + 1] << 16) | (data[i + 2] << 8) | data[i + 3]);
  };
  auto solax_get_error_bitmask = [&](size_t i) -> uint32_t {
    return uint32_t((data[i + 3] << 24) | (data[i + 2] << 16) | (data[i + 1] << 8) | data[i]);
  };

  ESP_LOGI(TAG, "Status frame received");

  float temperature = (int16_t) solax_get_16bit(0);

  uint16_t raw_energy_today = solax_get_16bit(2);
  float energy_today = raw_energy_today * 0.1f;

  uint16_t raw_dc1_voltage = solax_get_16bit(4);
  float dc1_voltage = raw_dc1_voltage * 0.1f;

  uint16_t raw_dc2_voltage = solax_get_16bit(6);
  float dc2_voltage = raw_dc2_voltage * 0.1f;

  uint16_t raw_dc1_current = solax_get_16bit(8);
  float dc1_current = raw_dc1_current * 0.1f;

  uint16_t raw_dc2_current = solax_get_16bit(10);
  float dc2_current = raw_dc2_current * 0.1f;

  uint16_t raw_ac_current = solax_get_16bit(12);
  float ac_current = raw_ac_current * 0.1f;

  uint16_t raw_ac_voltage = solax_get_16bit(14);
  float ac_voltage = raw_ac_voltage * 0.1f;

  uint16_t raw_ac_frequency = solax_get_16bit(16);
  float ac_frequency = raw_ac_frequency * 0.01f;

  uint16_t raw_ac_power = solax_get_16bit(18);
  float ac_power = raw_ac_power;

  // register 20 is not used

  uint32_t raw_energy_total = solax_get_32bit(22);
  // The inverter publishes a zero once per day on boot-up. This confuses the energy dashboard.
  if (raw_energy_total > 0) {
    this->publish_state_(this->energy_total_sensor_, (float) raw_energy_total * 0.1f);
  }

  uint32_t raw_runtime_total = solax_get_32bit(26);
  if (raw_runtime_total > 0) {
    this->publish_state_(this->runtime_total_sensor_, (float) raw_runtime_total);
  }

  uint8_t mode = (uint8_t) solax_get_16bit(30);

  ESP_LOGD(TAG, "  Grid voltage fault: %f V", solax_get_16bit(32) * 0.1f);
  ESP_LOGD(TAG, "  Grid frequency fault: %f Hz", solax_get_16bit(34) * 0.01f);
  ESP_LOGD(TAG, "  DC injection fault: %f A", solax_get_16bit(36) * 0.001f);
  ESP_LOGD(TAG, "  Temperature fault: %d °C", solax_get_16bit(38));
  ESP_LOGD(TAG, "  PV1 voltage fault: %f V", solax_get_16bit(40) * 0.1f);
  ESP_LOGD(TAG, "  PV2 voltage fault: %f V", solax_get_16bit(42) * 0.1f);
  ESP_LOGD(TAG, "  GFC fault: %f A", solax_get_16bit(44) * 0.001f);

  uint32_t error_bits = solax_get_error_bitmask(46);

  if (data.size() > 50) {
    ESP_LOGD(TAG, "  CT Pgrid: %d W", solax_get_16bit(50));
  }

  this->publish_state_(this->temperature_sensor_, temperature);
  this->publish_state_(this->energy_today_sensor_, energy_today);
  this->publish_state_(this->dc1_voltage_sensor_, dc1_voltage);
  this->publish_state_(this->dc2_voltage_sensor_, dc2_voltage);
  this->publish_state_(this->dc1_current_sensor_, dc1_current);
  this->publish_state_(this->dc2_current_sensor_, dc2_current);
  this->publish_state_(this->ac_current_sensor_, ac_current);
  this->publish_state_(this->ac_voltage_sensor_, ac_voltage);
  this->publish_state_(this->ac_frequency_sensor_, ac_frequency);
  this->publish_state_(this->ac_power_sensor_, ac_power);
  this->publish_state_(this->mode_sensor_, mode);
  this->publish_state_(this->error_bits_sensor_, error_bits);
  this->publish_state_(this->errors_text_sensor_, this->error_bits_to_string_(error_bits));
  this->publish_state_(this->mode_name_text_sensor_, (mode <= 9) ? MODE_NAMES[mode] : "Unknown");

  this->no_response_count_ = 0;
}

void SolaxX1::publish_device_offline_() {
  this->publish_state_(this->mode_sensor_, -1);
  this->publish_state_(this->mode_name_text_sensor_, "Offline");

  this->publish_state_(this->temperature_sensor_, NAN);
  this->publish_state_(this->dc1_voltage_sensor_, 0);
  this->publish_state_(this->dc2_voltage_sensor_, 0);
  this->publish_state_(this->dc1_current_sensor_, 0);
  this->publish_state_(this->dc2_current_sensor_, 0);
  this->publish_state_(this->ac_current_sensor_, 0);
  this->publish_state_(this->ac_voltage_sensor_, NAN);
  this->publish_state_(this->ac_frequency_sensor_, NAN);
  this->publish_state_(this->ac_power_sensor_, 0);
}

void SolaxX1::update() {
  if (this->no_response_count_ >= REDISCOVERY_THRESHOLD) {
    this->publish_device_offline_();
    ESP_LOGD(TAG, "The device is or was offline. Broadcasting discovery for address configuration...");
    this->discover_devices();
    //    this->query_device_info(this->address_);
    // Try to query live data on next update again. The device doesn't
    // respond to the discovery broadcast if it's already configured.
    this->no_response_count_ = 0;
  } else {
    this->no_response_count_++;
    this->query_status_report(this->address_);
  }
}

void SolaxX1::publish_state_(sensor::Sensor *sensor, float value) {
  if (sensor == nullptr)
    return;

  sensor->publish_state(value);
}

void SolaxX1::publish_state_(text_sensor::TextSensor *text_sensor, const std::string &state) {
  if (text_sensor == nullptr)
    return;

  text_sensor->publish_state(state);
}

void SolaxX1::dump_config() {
  ESP_LOGCONFIG(TAG, "SolaxX1:");
  ESP_LOGCONFIG(TAG, "  Address: 0x%02X", this->address_);
  LOG_SENSOR("", "Temperature", this->temperature_sensor_);
  LOG_SENSOR("", "Energy Today", this->energy_today_sensor_);
  LOG_SENSOR("", "DC1 Voltage", this->dc1_voltage_sensor_);
  LOG_SENSOR("", "DC2 Voltage", this->dc2_voltage_sensor_);
  LOG_SENSOR("", "DC1 Current", this->dc1_current_sensor_);
  LOG_SENSOR("", "DC2 Current", this->dc2_current_sensor_);
  LOG_SENSOR("", "AC Current", this->ac_current_sensor_);
  LOG_SENSOR("", "AC Voltage", this->ac_voltage_sensor_);
  LOG_SENSOR("", "AC Frequency", this->ac_frequency_sensor_);
  LOG_SENSOR("", "AC Power", this->ac_power_sensor_);
  LOG_SENSOR("", "Energy Total", this->energy_total_sensor_);
  LOG_SENSOR("", "Runtime Total", this->runtime_total_sensor_);
  LOG_SENSOR("", "Mode", this->mode_sensor_);
  LOG_SENSOR("", "Error Bits", this->error_bits_sensor_);
  LOG_TEXT_SENSOR("  ", "Mode Name", this->mode_name_text_sensor_);
  LOG_TEXT_SENSOR("  ", "Errors", this->errors_text_sensor_);
}

std::string SolaxX1::error_bits_to_string_(const uint32_t mask) {
  bool first = true;
  std::string errors_list = "";

  if (mask) {
    for (int i = 0; i < 32; i++) {
      if (mask & (1 << i)) {
        if (first) {
          first = false;
        } else {
          errors_list.append(";");
        }
        errors_list.append(ERRORS[i]);
      }
    }
  }

  return errors_list;
}

}  // namespace solax_x1
}  // namespace esphome
