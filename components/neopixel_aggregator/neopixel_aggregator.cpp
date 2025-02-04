#include "neopixel_aggregator.h"

namespace esphome {
namespace neopixel_aggregator {

// overly restrictive but sufficient for now
Color hex_to_color(const std::string& hex) {
    int r, g, b;
    if (hex.size() != 7 || hex[0] != '#') {
      return Color(0, 0, 0);  // Default to black
    }

    sscanf(hex.c_str(), "#%02x%02x%02x", &r, &g, &b);
    return Color(r & 0xFF, g & 0xFF, b & 0xFF);
}

// from esphome/components/light/esp_hsv_color.cpp
Color to_rgb(uint8_t hue, uint8_t sat, uint8_t val) {
  // upper 3 hue bits are for branch selection, lower 5 are for values
  const uint8_t offset8 = (hue & 0x1F) << 3;  // 0..248
  // third of the offset, 255/3 = 85 (actually only up to 82; 164)
  const uint8_t third = esp_scale8(offset8, 85);
  const uint8_t two_thirds = esp_scale8(offset8, 170);
  Color rgb(255, 255, 255, 0);
  switch (hue >> 5) {
    case 0b000:
      rgb.r = 255 - third;
      rgb.g = third;
      rgb.b = 0;
      break;
    case 0b001:
      rgb.r = 171;
      rgb.g = 85 + third;
      rgb.b = 0;
      break;
    case 0b010:
      rgb.r = 171 - two_thirds;
      rgb.g = 170 + third;
      rgb.b = 0;
      break;
    case 0b011:
      rgb.r = 0;
      rgb.g = 255 - third;
      rgb.b = third;
      break;
    case 0b100:
      rgb.r = 0;
      rgb.g = 171 - two_thirds;
      rgb.b = 85 + two_thirds;
      break;
    case 0b101:
      rgb.r = third;
      rgb.g = 0;
      rgb.b = 255 - third;
      break;
    case 0b110:
      rgb.r = 85 + third;
      rgb.g = 0;
      rgb.b = 171 - third;
      break;
    case 0b111:
      rgb.r = 170 + third;
      rgb.g = 0;
      rgb.b = 85 - third;
      break;
    default:
      break;
  }
  // low saturation -> add uniform color to orig. hue
  // high saturation -> use hue directly
  // scales with square of saturation
  // (r,g,b) = (r,g,b) * sat + (1 - sat)^2
  rgb *= sat;
  const uint8_t desat = 255 - sat;
  rgb += esp_scale8(desat, desat);
  // (r,g,b) = (r,g,b) * val
  rgb *= val;
  return rgb;
}


void NeopixelAggregatorComponent::setup() {
  this->register_entity_listeners();
  ESP_LOGD("neopixel_aggregator", "Setup completed");
}

void NeopixelAggregatorComponent::register_entity(light::LightState *ent) {
  neopixel_entity_ = static_cast<light::AddressableLight *>(ent->get_output());
}

void NeopixelAggregatorComponent::set_brightness(float brightness) {
  brightness_ = brightness;
  this->redraw();
}

void NeopixelAggregatorComponent::set_off_color(const std::string &off_color) {
  off_color_ = hex_to_color(off_color);
  this->redraw();
}

void NeopixelAggregatorComponent::set_force(bool force) {
  force_all_on_ = force;  
}

Color NeopixelAggregatorComponent::map_color(const Color& color) const {
  Color ncolor = Color(color.r, color.g, color.b);
  return ncolor.fade_to_black((1.0-this->brightness_) * 255);
}

void NeopixelAggregatorComponent::register_heartbeat(int led) {
  if (led >= 0) {
    heartbeat_ = led;
    this->set_interval("heartbeat", 50, [this] { this->heartbeat_interval(); });
  }
}

void NeopixelAggregatorComponent::heartbeat_interval() {
  uint8_t hue = ((micros() >> this->heartbeat_speed_)) & 0xFF;
  Color color = to_rgb(hue, 40, 255);

  if (!network::is_connected()) {
    color = Color(255,0,0);
  } else if (!api_is_connected()) {
    color = Color(0,0,255);
  }

  neopixel_entity_->get(heartbeat_).set(this->map_color(color));
  neopixel_entity_->schedule_show();
}


void NeopixelAggregatorComponent::register_entity_listeners() {
  // This appears to happen before log is available

  for (const auto &entry : led_mappings_) {
    int led_number = entry.first;
    for (const auto &mapping : entry.second) {
      const std::string entity_id = std::get<0>(mapping);
      const Color color = std::get<1>(mapping);
      esphome::api::global_api_server->subscribe_home_assistant_state(
        entity_id, nullopt, [this, led_number, entity_id, color](std::string state) {
          // undefined and off are false, any others?
          const bool state_bool = state == "on" || state == "ON";
          this->on_homeassistant_state(entity_id, led_number, color, state_bool);
        }
      );
    }
  }
}

void NeopixelAggregatorComponent::redraw() {
  for (const auto &entry : led_mappings_) {
    int led_number = entry.first;
    bool _first = true;
    auto ents = entry.second;
    for (int i=0;i<ents.size() && i<=0;i++) {
      auto mapping = ents[i];
      const std::string entity_id = std::get<0>(mapping);
      const Color color = std::get<1>(mapping);
      const bool state_bool = std::get<2>(mapping);
      this->on_homeassistant_state(entity_id, led_number, color, state_bool);
    }
  }
}

void NeopixelAggregatorComponent::on_homeassistant_state(const std::string &entity_id, const int led_number, const Color color, bool state_bool) {
  const auto new_mapping = std::make_tuple(entity_id, color, state_bool);
  auto &led_list = led_mappings_[led_number];
  std::replace_if(led_list.begin(), led_list.end(), [entity_id](std::tuple<std::string, Color, bool> existing) { return std::get<0>(existing) == entity_id; }, new_mapping);


  if (!neopixel_entity_) {
    return;
  }

  Color use_color = off_color_;

  if (!did_led_setup_) {
    Color black = Color(0,0,0);
    for (int i=0;i<neopixel_entity_->size();i++) {
      // ESP_LOGD("found-neo","zero out LED %d",i);
      neopixel_entity_->get(led_number).set(black);
    }
    did_led_setup_ = true;
  }

  for (const auto& tup : led_list) {
    if (force_all_on_ || std::get<2>(tup)) {  // Check if the bool is true
      use_color = std::get<1>(tup);  // Update _color to the matching color
    }
  }

  neopixel_entity_->get(led_number).set(this->map_color(use_color));
  neopixel_entity_->schedule_show();
}

void NeopixelAggregatorComponent::loop() {
  // Poll for updates from Home Assistant API
}

void NeopixelAggregatorComponent::add_led_mapping(int led_number, const std::string &entity_id, const std::string &color_str) {
  auto &led_list = led_mappings_[led_number];
  led_list.emplace_back(entity_id, hex_to_color(color_str), false);
}

void NeopixelAggregatorComponent::dump_config() {
  ESP_LOGD("neopixel_aggregator", "LED mappings:");
  for (const auto &entry : led_mappings_) {
    int led_number = entry.first;
    for (const auto &mapping : entry.second) {
      // (yay)
      ESP_LOGD("neopixel_aggregator", "LED %d -> Entity: %s Color: #%02X%02X%02X Current state: %s", led_number, std::get<0>(mapping).c_str(), std::get<1>(mapping).r,std::get<1>(mapping).g,std::get<1>(mapping).b, std::get<2>(mapping)?"on":"off");
    }
  }
}


}  // namespace neopixel_aggregator
}  // namespace esphome

