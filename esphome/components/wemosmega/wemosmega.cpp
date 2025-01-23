#include "wemosmega.h"
#include "esphome/core/log.h"

namespace esphome {
namespace wemosmega {

static const char *const TAG = "wemosmega";

void WemosMegaComponent::setup() {
  this->serial_initialized_ = false;
}
void WemosMegaComponent::dump_config() {
  std::string res = write_serial_('?', 0);
  while(res[0] != '?') {
    ESP_LOGCONFIG(TAG, "%s", res.c_str());
	res = this->read_serial_(true);
  }
}
void WemosMegaComponent::loop() {
  std::string in = this->read_serial_();
  if(in.length() > 1 && in[0] == '>')
    last_received_ = atoi(&in[1]);
}

#define GEN_ERROR(x) { \
  this->status_set_error(x); \
  ESP_LOGE(TAG, "Error during %s: %s", __FUNCTION__, x); \
}

#define CHECK_ERROR_RET(x, v) { \
  if(x[0] == '!') { \
    GEN_ERROR(&x[2]); \
    return v; \
  } \
}
#define CHECK_ERROR(x) { \
  if(x[0] == '!') { \
    GEN_ERROR(&x[2]); \
    return; \
  } \
}

uint16_t WemosMegaComponent::analog_read(uint8_t pin) {
  std::string res = this->write_serial_('A', pin);
  CHECK_ERROR_RET(res, 0);
  return atoi(res.c_str());
}
bool WemosMegaComponent::digital_read(uint8_t pin) {
  if(this->input_mask_.find(pin) != this->input_mask_.end())
    return (last_received_ & (1 << this->input_mask_[pin])) != 0;
  std::string res = this->write_serial_('R', pin);
  CHECK_ERROR_RET(res, false);
  return res[0] != '0';
}
void WemosMegaComponent::digital_write(uint8_t pin, bool value) {
  std::string res = this->write_serial_(value ? 'H': 'L', pin);
  CHECK_ERROR(res);
}
void WemosMegaComponent::pin_mode(uint8_t pin, gpio::Flags flags) {
  std::string res = this->write_serial_(flags == gpio::FLAG_OUTPUT ? 'O' : 'I', pin);
  CHECK_ERROR(res);
  if(flags != gpio::FLAG_OUTPUT && this->input_mask_.find(pin) == this->input_mask_.end()) {
    res = this->write_serial_('W', pin);
    CHECK_ERROR(res);
    this->input_mask_.emplace(pin, atoi(res.c_str()));
  }
}
void WemosMegaComponent::pwm_set(uint8_t pin, uint8_t value) {
  std::string res = this->write_serial_('P', pin);
  CHECK_ERROR(res);
  res = this->write_serial_('V', value);
  CHECK_ERROR(res);
}
float WemosMegaComponent::MAX31865_read(uint8_t pin) {
  std::string res;
  if(this->temp_ref_.find(pin) == this->temp_ref_.end()) {
    std::string res = this->write_serial_('C', pin);
    CHECK_ERROR_RET(res, 0.);
    this->temp_ref_.emplace(pin, atoi(res.c_str()));
  }
  res = this->write_serial_('T', temp_ref_.at(pin));
  CHECK_ERROR_RET(res, 0.);
  return atof(res.c_str());
}

#undef CHECK_ERROR
#undef CHECK_ERROR_RET
#undef GEN_ERROR

void WemosMegaComponent::init_serial_() {
  if(this->serial_initialized_)
	return;

  this->serial_initialized_ = true;
  ESP_LOGCONFIG(TAG, "Setting up Wemos Mega...");
  this->check_uart_settings(115200);
 
  this->last_received_ = 0;

  // Test to see if device is ready
  auto test = [this] () -> bool {
    for(size_t i = 0; i < 3; i++) {
      if(this->write_serial_('-', 0)[0] == '-')
        return true;
    }
    return false;
  };
  
  this->write_serial_('#', 0); // reset mega
  if(!test()) {
    ESP_LOGE(TAG, "Mega not ready");
    this->mark_failed();
    this->serial_initialized_ = false;
    return;
  }

  ESP_LOGD(
    TAG,
    "Initialization complete. Warning: %d, Error: %d",
    this->status_has_warning(),
    this->status_has_error()
  );
}
std::string WemosMegaComponent::read_serial_(bool lock) {
  this->init_serial_();
  std::string res;
  if(!lock && this->available() == 0)
    return res;
  uint8_t c = '\0';
  while(c != '\n') {
	while(this->available() != 0 && c != '\n') {
	  this->read_byte(&c);
	  if(c != '\n')
		res += c;
	}
  }
  switch(res[0]) {
  case '!':
    ESP_LOGD(TAG, "Error: %s", &res[1]);
	break;
  case '>':
    ESP_LOGD(TAG, "Watch: %s", &res[1]);
	break;
  default:
    ESP_LOGV(TAG, "Recv: %s", res.c_str());
	break;
  }
  return res;
}
std::string WemosMegaComponent::write_serial_(char label, uint8_t value) {
  this->init_serial_();
  char s[7];
  snprintf(s, 7, "%c%d\n", label, value);
  this->write_str(s);
  ESP_LOGV(TAG, "Send: %s", s);
  std::string res;
  while((res = this->read_serial_(true))[0] == '>') { }
  return res;
}

// GPIO
//
void WemosMegaGPIOPin::setup() {
  this->pin_mode(flags_);
}
std::string WemosMegaGPIOPin::dump_summary() const {
  char buffer[32];
  snprintf(buffer, sizeof(buffer), "%u via Wemos Mega", this->pin_);
  return buffer;
}
void WemosMegaGPIOPin::pin_mode(gpio::Flags flags) {
  this->parent_->pin_mode(this->pin_, flags);
}
bool WemosMegaGPIOPin::digital_read() {
  return this->parent_->digital_read(this->pin_) != this->inverted_;
}
void WemosMegaGPIOPin::digital_write(bool value) {
  this->parent_->digital_write(this->pin_, value != this->inverted_);
}

// PWM
//
static const char *const TAG_PWM = "wemosmega_pwm";

void WemosMegaPWM::setup() {
  ESP_LOGCONFIG(TAG_PWM, "Setting up WemosMega PWM Output...");
  this->parent_->pin_mode(pin_, gpio::FLAG_OUTPUT);
  this->write_state(0);
}
void WemosMegaPWM::dump_config() {
  ESP_LOGCONFIG(TAG_PWM, "WemosMega PWM on pin %u", this->pin_);
  LOG_FLOAT_OUTPUT(this);
}
void WemosMegaPWM::write_state(float state) {
  this->parent_->pwm_set(this->pin_, 255 * state);
}

// ADC
//
static const char *const TAG_ADC = "wemosmega.adc";

void WemosMegaADCSensor::setup() {
  ESP_LOGCONFIG(TAG_ADC, "Setting up ADC via Wemos Mega '%s'...", this->get_name().c_str());
}
void WemosMegaADCSensor::dump_config() {
  ESP_LOGCONFIG(TAG_ADC, "Wemos Mega ADC Sensor on pin %u", this->pin_);
  ESP_LOGCONFIG(TAG_ADC, "  Samples: %i", this->sample_count_);
  LOG_UPDATE_INTERVAL(this);
}
void WemosMegaADCSensor::update() {
  float value_v = this->sample();
  ESP_LOGV(TAG_ADC, "'%s': Got voltage=%.4fV", this->get_name().c_str(), value_v);
  this->publish_state(value_v);
}

void WemosMegaADCSensor::set_sample_count(uint8_t sample_count) {
  if (sample_count != 0)
    this->sample_count_ = sample_count;
}
float WemosMegaADCSensor::sample() {
  uint32_t raw = 0;
  for(uint8_t sample = 0; sample < this->sample_count_; sample++)
    raw += this->parent_->analog_read(this->pin_);
  raw = (raw + (this->sample_count_ >> 1)) / this->sample_count_;
  if(this->output_raw_)
    return raw;
  return raw / 1024.0f;
}

static const char *const TAG_MAX31865 = "wemosmega.max31865";

void WemosMegaMAX31865::setup() {
  ESP_LOGCONFIG(TAG_MAX31865, "Setting up Wemos Mega MAX31865 '%s'...", this->name_.c_str());
}
void WemosMegaMAX31865::dump_config() {
  ESP_LOGCONFIG(TAG_MAX31865, "Wemos Mega MAX31865 with CS pin %u", this->pin_);
  LOG_UPDATE_INTERVAL(this);
  LOG_SENSOR("  ", "Thermocouple", this);
}
void WemosMegaMAX31865::update() {
  // Conversion time typ: 170ms, max: 220ms
  auto f = std::bind(&WemosMegaMAX31865::read_data_, this);
  this->set_timeout("value", 220, f);
}
void WemosMegaMAX31865::read_data_() {
  this->publish_state(this->parent_->MAX31865_read(this->pin_));
}

}  // namespace wemosmega
}  // namespace esphome
