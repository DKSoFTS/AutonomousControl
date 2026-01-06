#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/sensor/sensor.h"

namespace smart_table {

// Example: height calculation like autonomous_controller
static const float base_height = 29.5f;
static const float step_size = 0.4f;
static const uint8_t height_min = 0x4B;  // minimum raw value
static const uint8_t height_max = 0x7B;  // maximum raw value
using t_button = uint8_t;
t_button const button_down = 0x01;
t_button const button_up = 0x02;
t_button const button_1 = 0x04;
t_button const button_2 = 0x08;
t_button const button_3 = 0x10;
t_button const button_4 = 0x20;
t_button const button_M = 0x40;


class SmartTable : public esphome::Component {
 public:
  SmartTable(esphome::uart::UARTComponent *desk,
             esphome::uart::UARTComponent *controller,
             esphome::sensor::Sensor *height_sensor);

  void loop() override;

  // Move table up
  // void go_up(uint32_t duration_ms = 10);
  // // Move table down
  // void go_down(uint32_t duration_ms = 10);

  protected:
    esphome::uart::UARTComponent *desk_;
    esphome::uart::UARTComponent *controller_;
    esphome::sensor::Sensor *height_sensor_;
  };

  // Helper function used by YAML lambda
  void press_button(uint8_t mask, uint32_t duration_ms);
  void go_up(float distance_in);
  void go_down(float distance_in);
  void go_to_height(float height);
  void update_height_control();
  void send_empty_byte();
  void byte_up(uint32_t duration_dist);
  void byte_down(uint32_t duration_dist);
  void send_byte(uint8_t mask);

}  // namespace smart_table