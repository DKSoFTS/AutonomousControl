#include "smart_table.h"
#include "esphome/core/log.h"

namespace smart_table
{

  static const char *TAG = "smart_table";
  float last_height = -1.0;
  // Global variable to track the desired height
  static float target_height = -1.0;
  static bool moving_to_target = false;

  // Store pointer to desk UART so press_button can use it
  static esphome::uart::UARTComponent *desk_uart_ptr = nullptr;

  SmartTable::SmartTable(esphome::uart::UARTComponent *desk,
                         esphome::uart::UARTComponent *controller,
                         esphome::sensor::Sensor *height_sensor)
  {
    desk_ = desk;
    controller_ = controller;
    height_sensor_ = height_sensor;

    // Save globally for press_button()
    desk_uart_ptr = desk_;
  }

  void SmartTable::loop()
  {
    static uint8_t rx[6];
    static uint8_t idx = 0;

    // ==================================================
    // Controller → Desk (stream only)
    // ==================================================
    while (controller_->available())
    {
      uint8_t b;
      controller_->read_byte(&b);
      if (!moving_to_target)
      {
        desk_->write_byte(b);
      }
      // Optional: log raw data
      // ESP_LOGD(TAG, "REMOTE → DESK : 0x%02X", b);
    }

    // ==================================================
    // Desk → Controller (stream + height decode)
    // ==================================================
    while (desk_->available())
    {
      uint8_t b;
      desk_->read_byte(&b);
      controller_->write_byte(b);

      // Feed the 6-byte height frame parser
      rx[idx++] = b;
      if (idx == 6)
      {
        idx = 0;

        // Only decode valid frames: 0x98 0x98 + last 2 bytes equal
        if ((rx[0] == 0x98 && rx[1] == 0x98) && (rx[4] == rx[5]))
        {
          uint8_t raw = rx[4];
          if (raw >= height_min && raw <= height_max)
          {
            // Step calculation like the remote
            uint8_t height_step = raw - height_min + 1;
            float height_in = base_height + step_size * (height_step - 1);

            if (height_sensor_ != nullptr && height_in != last_height)
            {
              // ESP_LOGI(TAG, "Desk Height: %.1f in (raw=0x%02X step=%u)", height_in, raw, height_step);
              height_sensor_->publish_state(height_in);
              last_height = height_in; // save new value
            }
          }
        }
      }
    }

    smart_table::update_height_control();
  }

  // ==================================================
  // Button helper (non-blocking, called from YAML)
  // ==================================================
  void press_button(uint8_t mask, uint32_t duration_ms)
  {
    if (desk_uart_ptr == nullptr) {
      ESP_LOGE(TAG, "Desk UART not initialized");
      return;
    }

    send_empty_byte();
    send_byte(mask);

  }

  void go_up(float distance_in)
  {
    if (last_height < 0)
    {
      ESP_LOGW(TAG, "Current height unknown, cannot go_up");
      return;
    }

    float new_target = last_height + distance_in;

    ESP_LOGI(TAG, "Go UP by %.2f in (%.2f → %.2f)",
             distance_in, last_height, new_target);

    go_to_height(new_target);
  }

  void go_down(float distance_in)
  {
    if (last_height < 0)
    {
      ESP_LOGW(TAG, "Current height unknown, cannot go_down");
      return;
    }

    float new_target = last_height - distance_in;

    ESP_LOGI(TAG, "Go DOWN by %.2f in (%.2f → %.2f)",
             distance_in, last_height, new_target);

    go_to_height(new_target);
  }

  void send_empty_byte()
  {
    if (desk_uart_ptr == nullptr)
      return;

    // uint8_t release[5] = {0xD8, 0xD8, 0x66, 0x00, 0x00};
    // desk_uart_ptr->write_array(release, 5);
    send_byte(0x00);
  }

  // Call this to start moving
  void go_to_height(float height)
  {
    if (last_height < 0)
    {
      ESP_LOGW(TAG, "Current height unknown, cannot go_to_height");
      return;
    }

    ESP_LOGI(TAG, "Starting move from %.1f in to %.1f in", last_height, height);
    send_empty_byte();
    target_height = height;
    moving_to_target = true;
  }

  // Call this from your SmartTable::loop()
  void update_height_control()
  {
    if (!moving_to_target)
      return; // nothing to do

    // ESP_LOGI("TAG","Working %f", last_height);
    const float tolerance = 0.25f;

    if (fabs(last_height - target_height) <= tolerance)
    {
      ESP_LOGI(TAG, "Reached target height %.1f in", last_height);
      // send_empty_byte();
      moving_to_target = false; // stop moving
      return;
    }

    // Move one small step each loop iteration
    if (last_height < target_height)
    {
      byte_up(0); // 50 ms step
    }
    else
    {
      byte_down(0);
    }
  }

  void send_byte(uint8_t mask)
  {
    if (desk_uart_ptr == nullptr)
    {
      ESP_LOGE(TAG, "Desk UART not initialized");
      return;
    }
    uint8_t msg[5] = {0xD8, 0xD8, 0x66, mask, mask};

    //  ESP_LOGD(TAG, "TX -> DESK: %02X %02X %02X %02X %02X", msg[0], msg[1], msg[2], msg[3], msg[4]);

    desk_uart_ptr->write_array(msg, 5);
  }

  void byte_up(uint32_t duration_ms)
  {
    send_byte(button_up);
  }

  void byte_down(uint32_t duration_ms)
  {
    send_byte(button_down);
  }

} // namespace smart_table