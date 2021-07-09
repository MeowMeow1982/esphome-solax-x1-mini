#include "solax_x1.h"
#include "esphome/core/log.h"

namespace esphome {
namespace solax_x1 {

static const char *const TAG = "solax_x1";

// Hybrid.X1.X3-G3.ModbusTCP.RTU.V3.21-.English.pdf
// May be some modes doesn't apply to the X1 mini
static const char *modeText[16] = {
    "Wait",             // 0
    "Check",            // 1
    "Normal",           // 2
    "Fault",            // 3
    "Permanent Fault",  // 4

    "Update",     // 5
    "EPS check",  // 6
    "EPS",        // 7
    "Self Test",  // 8
    "Idle",       // 9
};

static const char *errorText[32] = {
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

void SolaxX1::on_modbus_solax_info(const std::vector<uint8_t> &data) {
  if (data.size() != 58) {
    ESP_LOGW(TAG, "Invalid response size: %zu", data.size());
    return;
  }

  // Request info (0x11 0x03)
  // [18:13:12][VV][modbus_solax:214]: TX -> AA.55.01.00.00.0A.11.03.00.01.1E (11)
  // Response (0x11 0x83)
  // [18:13:12][VV][modbus_solax:084]: RX <- AA.55.00.0A.01.00.11.83.3A.01.00.00.00.00.00.00.56.31.2E.30.30.20.20.20.20.20.20.20.20.20.20.20.20.20.20.73.6F.6C.61.78.20.20.20.20.20.20.20.20.20.58.4D.55.30.36.32.47.43.30.39.33.35.34.30.33.36.30.30.0C.0F (69)
  //                                                                    ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
  //
  // Data  0      (device type?):      0x01 (single phase)
  // Data  1...6  (rated power):       0x00 0x00 0x00 0x00 0x00 0x00 ()
  // Data  7...11 (firmware version):  0x56 0x31 0x2E 0x30 0x30 (V1.00)
  // Data 12...25 (module name):       0x20 0x20 0x20 0x20 0x20 0x20 0x20 0x20 0x20 0x20 0x20 0x20 0x20 0x20 ( )
  // Data 26...39 (factory name):      0x73 0x6F 0x6C 0x61 0x78 0x20 0x20 0x20 0x20 0x20 0x20 0x20 0x20 0x20 (solax )
  // Data 40...53 (serial number):     0x58 0x4D 0x55 0x30 0x36 0x32 0x47 0x43 0x30 0x39 0x33 0x35 0x34 0x30
  // (XMU062GC093540) Data 54...57 (rated bus voltage): 0x33 0x36 0x30 0x30 (3600)

  ESP_LOGW(TAG, "Info: %s", hexencode(&data.front(), data.size()).c_str());
  // @TODO: Output Solax_Info_t
  this->no_response_count_ = 0;
}

void SolaxX1::on_modbus_solax_data(const std::vector<uint8_t> &data) {
  if (data.size() != 52) {
    ESP_LOGW(TAG, "Invalid response size: %zu", data.size());
    return;
  }

  // Request live data (0x11 0x02)
  // [18:15:15][VV][modbus_solax:214]: TX -> AA.55.01.00.00.0A.11.02.00.01.1D (11)
  // Response (0x11 0x82)
  // [18:15:15][VV][modbus_solax:214]: RX <-
  // AA.55.00.0A.01.00.11.82.34.00.19.00.01.02.46.00.00.00.0A.00.00.00.05.09.13.13.87.00.32.FF.FF.00.00.00.11.00.00.00.14.00.02.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.02.D0.06.21 (63)
  //                            ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
  //
  // Data  0...1  (temperature):        0x00 0x19 (25 C)
  // Data  2...3  (energy today):       0x00 0x01 (0.1 kWh)
  // Data  4...5  (pv1 voltage):        0x02 0x46 (58.2 V)
  // Data  6...7  (pv2 voltage):        0x00 0x00 (0.0 V)
  // Data  8...9  (pv1 current):        0x00 0x0A (1.0 A)
  // Data 10...11 (pv2 current):        0x00 0x00 (0.0 A)
  // Data 12...13 (ac current):         0x00 0x05 (0.5 A)
  // Data 14...15 (ac voltage):         0x09 0x13 (232.3V)
  // Data 16...17 (ac frequency):       0x13 0x87 (49.99 Hz)
  // Data 18...19 (ac power):           0x00 0x32 (50 W)
  // Data 20...21 (unused):             0xFF 0xFF
  // Data 22...25 (energy total):       0x00 0x00 0x00 0x11 (0.1 kWh)
  // Data 26...29 (runtime total):      0x00 0x00 0x00 0x14 (20 h)
  // Data 30...31 (mode):               0x00 0x02 (2: Normal)
  // Data 32...33 (grid voltage fault): 0x00 0x00 (0.0 V)
  // Data 34...35 (grid freq. fault)    0x00 0x00 (0.00 Hz)
  // Data 36...37 (dc injection fault): 0x00 0x00 (0 mA)
  // Data 38...39 (temperature fault):  0x00 0x00
  // Data 40...41 (pv1 voltage fault):  0x00 0x00
  // Data 42...43 (pv2 voltage fault):  0x00 0x00
  // Data 44...45 (gfc fault):          0x00 0x00
  // Data 46...49 (error message):      0x00 0x00 0x00 0x00
  // Data 50...52 (unknown):            0x02 0xD0

  auto solax_get_16bit = [&](size_t i) -> uint16_t {
    return (uint16_t(data[i + 0]) << 8) | (uint16_t(data[i + 1]) << 0);
  };
  auto solax_get_32bit = [&](size_t i) -> uint32_t {
    return uint32_t((data[i] << 24) | (data[i + 1] << 16) | (data[i + 2] << 8) | data[i + 3]);
  };
  auto solax_get_error_bitmask = [&](size_t i) -> uint32_t {
    return uint32_t((data[i + 3] << 24) | (data[i + 2] << 16) | (data[i + 1] << 8) | data[i]);
  };

  float temperature = solax_get_16bit(0);

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
  float energy_total = raw_energy_total * 0.1f;

  uint32_t raw_runtime_total = solax_get_32bit(26);
  float runtime_total = raw_runtime_total;

  uint8_t mode = (uint8_t) solax_get_16bit(30);

  // register 32: Grid voltage fault in 0.1V
  // register 34: Grid frequency fault in 0.01Hz
  // register 36: DC injection fault in 1mA
  // register 38: Temperature fault in °C
  // register 40: Pv1 voltage fault in 0.1V
  // register 42: Pv2 voltage fault in 0.1V
  // register 44: GFC fault

  uint32_t error_bits = solax_get_error_bitmask(46);

  if (this->temperature_sensor_ != nullptr)
    this->temperature_sensor_->publish_state(temperature);
  if (this->energy_today_sensor_ != nullptr)
    this->energy_today_sensor_->publish_state(energy_today);
  if (this->dc1_voltage_sensor_ != nullptr)
    this->dc1_voltage_sensor_->publish_state(dc1_voltage);
  if (this->dc2_voltage_sensor_ != nullptr)
    this->dc2_voltage_sensor_->publish_state(dc2_voltage);
  if (this->dc1_current_sensor_ != nullptr)
    this->dc1_current_sensor_->publish_state(dc1_current);
  if (this->dc2_current_sensor_ != nullptr)
    this->dc2_current_sensor_->publish_state(dc2_current);
  if (this->ac_current_sensor_ != nullptr)
    this->ac_current_sensor_->publish_state(ac_current);
  if (this->ac_voltage_sensor_ != nullptr)
    this->ac_voltage_sensor_->publish_state(ac_voltage);
  if (this->ac_frequency_sensor_ != nullptr)
    this->ac_frequency_sensor_->publish_state(ac_frequency);
  if (this->ac_power_sensor_ != nullptr)
    this->ac_power_sensor_->publish_state(ac_power);
  if (this->energy_total_sensor_ != nullptr)
    this->energy_total_sensor_->publish_state(energy_total);
  if (this->runtime_total_sensor_ != nullptr)
    this->runtime_total_sensor_->publish_state(runtime_total);
  if (this->mode_sensor_ != nullptr)
    this->mode_sensor_->publish_state(mode);
  if (this->error_bits_sensor_ != nullptr)
    this->error_bits_sensor_->publish_state(error_bits);

  this->no_response_count_ = 0;
}

void SolaxX1::update() {
  if (this->no_response_count_ >= REDISCOVERY_THRESHOLD) {
    ESP_LOGD(TAG, "The device is or was offline. Broadcasting discovery for address configuration...");
    this->discover_devices();
    //    this->query_info(this->address_);
    // Try to query live data on next update again. The device doesn't
    // respond to the discovery broadcast if it's already configured.
    this->no_response_count_ = 0;
  } else {
    no_response_count_++;
    this->query_live_data(this->address_);
  }
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
}

}  // namespace solax_x1
}  // namespace esphome