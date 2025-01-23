#pragma once

#include "esphome.h"
#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/output/float_output.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/voltage_sampler/voltage_sampler.h"

#include <map>
#include <vector>

namespace esphome {
namespace wemosmega {

class WemosMegaComponent;
class WemosMegaGPIOPin;

class WemosMegaComponent : public Component, public uart::UARTDevice {
  friend class WemosMegaGPIOPin;
public:
  WemosMegaComponent() = default;

  void setup() override;
  void dump_config() override;
  void loop() override;

  uint16_t analog_read(uint8_t pin);
  bool digital_read(uint8_t pin);
  void digital_write(uint8_t pin, bool value);
  void pin_mode(uint8_t pin, gpio::Flags flags);
  void pwm_set(uint8_t pin, uint8_t value);
  float MAX31865_read(uint8_t pin);

  float get_setup_priority() const override { return setup_priority::HARDWARE; }
  float get_loop_priority() const override { return 9.0f; }

protected:
  void init_serial_();
  std::string read_serial_(bool lock = false);
  std::string write_serial_(char label, uint8_t value);

  bool serial_initialized_ = false;
  const size_t mask_size_ = 32;
  uint32_t last_received_ = 0;
  using mask_t = std::map<uint8_t, uint8_t>;
  mask_t input_mask_;
  using reference_t = std::map<uint8_t, uint8_t>;
  reference_t temp_ref_;
};

/// Helper class to expose a Mega pin as an internal input GPIO pin.
class WemosMegaGPIOPin : public GPIOPin {
public:
  explicit WemosMegaGPIOPin(WemosMegaComponent *parent, uint8_t pin) : parent_(parent), pin_(pin) { }
 
  void setup() override;

  void pin_mode(gpio::Flags flags) override;
  bool digital_read() override;
  void digital_write(bool value) override;
  std::string dump_summary() const override;

  void set_inverted(bool inverted) { inverted_ = inverted; }
  void set_flags(gpio::Flags flags) { flags_ = flags; }

protected:
  WemosMegaComponent *parent_ = nullptr;
  uint8_t pin_;
  bool inverted_;
  gpio::Flags flags_;
};

class WemosMegaPWM : public output::FloatOutput, public Component {
public:
  explicit WemosMegaPWM(WemosMegaComponent *parent, uint8_t pin) : parent_(parent), pin_(pin) {}

  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::HARDWARE; }
protected:
  void write_state(float state) override;

  WemosMegaComponent *parent_ = nullptr;
  uint8_t pin_;
};

class WemosMegaADCSensor : public sensor::Sensor, public PollingComponent, public voltage_sampler::VoltageSampler {
public:
  explicit WemosMegaADCSensor(WemosMegaComponent *parent, uint8_t pin) : parent_(parent), pin_(pin) {}

  void update() override;
  void setup() override;
  void dump_config() override;

  float sample() override;

  void set_output_raw(bool output_raw) { this->output_raw_ = output_raw; }
  void set_sample_count(uint8_t sample_count);

protected:
  WemosMegaComponent *parent_ = nullptr;
  uint8_t pin_;
  bool output_raw_{false};
  uint8_t sample_count_{1};
};

class WemosMegaMAX31865 : public sensor::Sensor, public PollingComponent {
public:
  explicit WemosMegaMAX31865(WemosMegaComponent *parent, uint8_t pin) : parent_(parent), pin_(pin) {}

  void update() override;
  void setup() override;
  void dump_config() override;  

  float get_setup_priority() const override { return setup_priority::DATA; }

protected:
  void read_data_();

  WemosMegaComponent *parent_ = nullptr;
  uint8_t pin_;
};

}  // namespace wemosmega
}  // namespace esphome
