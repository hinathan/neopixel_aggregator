#pragma once

#include "esphome.h"
#include <map>
#include <vector>
#include "esphome/components/light/addressable_light.h"
#include "esphome/core/util.h"
#include "esphome/components/network/util.h"

namespace esphome {
namespace neopixel_aggregator {

class NeopixelAggregatorComponent : public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config();
  void register_entity_listeners();
  void add_led_mapping(int led_number, const std::string &entity_id, const std::string &color_str);
  void on_homeassistant_state(const std::string &entity_id, const int led_number, const Color color, bool state_bool);
  void register_entity(light::LightState *ent);
  void register_heartbeat(int led);
  void heartbeat_interval();
  void set_brightness(float brightness);
  void set_force(bool force);
  void set_off_color(const std::string &off_color);
  void redraw();

 protected:
  Color map_color(const Color& color) const;

 private:
  light::AddressableLight *neopixel_entity_;
  int heartbeat_ = -1;
  int heartbeat_phase_ = 0;
  bool did_led_setup_ = false;
  int heartbeat_speed_ = 14;
  float brightness_ = 1.0;
  bool force_all_on_ = false;
  Color off_color_ = Color(0,0,0);
  std::map<int, std::vector<std::tuple<std::string, Color, bool>>> led_mappings_;
};

}  // namespace neopixel_aggregator
}  // namespace esphome

