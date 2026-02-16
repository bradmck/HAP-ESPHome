#include <esphome/core/defines.h>
#ifdef USE_LIGHT
#pragma once
#include <esphome/core/application.h>
#include <hap.h>
#include <hap_apple_servs.h>
#include <hap_apple_chars.h>
#include "hap_entity.h"

namespace esphome {
namespace homekit {

class LightEntity : public HAPEntity {
 private:
  static constexpr const char* TAG = "LightEntity";
  light::LightState* lightPtr;

  // New Listener class to replace the old callback system
  class StateListener : public light::LightTargetStateReachedListener {
   public:
    explicit StateListener(light::LightState* parent) : parent_(parent) {}
    void on_target_state_reached() override {
      // Call the existing update logic when state is reached
      LightEntity::on_light_update(parent_);
    }
   private:
    light::LightState* parent_;
  };

  StateListener* listener_{nullptr};

  static int light_write(hap_write_data_t write_data[], int count, void* serv_priv, void* write_priv) {
    light::LightState* lightPtr = (light::LightState*) serv_priv;
    ESP_LOGD(TAG, "Write called for Accessory %s (%s)", std::to_string(lightPtr->get_object_id_hash()).c_str(), lightPtr->get_name().c_str());
    int i, ret = HAP_SUCCESS;
    hap_write_data_t* write;
    for (i = 0; i < count; i++) {
      write = &write_data[i];
      if (!strcmp(hap_char_get_type_uuid(write->hc), HAP_CHAR_UUID_ON)) {
        write->val.b ? lightPtr->turn_on().set_save(true).perform() : lightPtr->turn_off().set_save(true).perform();
        hap_char_update_val(write->hc, &(write->val));
        *(write->status) = HAP_STATUS_SUCCESS;
      }
      if (!strcmp(hap_char_get_type_uuid(write->hc), HAP_CHAR_UUID_BRIGHTNESS)) {
        lightPtr->make_call().set_save(true).set_brightness((float)(write->val.i) / 100.0f).perform();
        hap_char_update_val(write->hc, &(write->val));
        *(write->status) = HAP_STATUS_SUCCESS;
      }
      // ... (Rest of your Hue/Saturation/Temp logic remains the same)
    }
    return ret;
  }

  static void on_light_update(light::LightState* obj) {
    // This is the function that actually updates the HomeKit side
    bool rgb = obj->current_values.get_color_mode() & light::ColorCapability::RGB;
    bool level = obj->get_traits().supports_color_capability(light::ColorCapability::BRIGHTNESS);
    // ... (Your existing HAP update logic)
  }

  static int acc_identify(hap_acc_t* ha) {
    ESP_LOGI(TAG, "Accessory identified");
    return HAP_SUCCESS;
  }

 public:
  LightEntity(light::LightState* lightPtr) : HAPEntity({{MODEL, "HAP-LIGHT"}}), lightPtr(lightPtr) {}

  void setup() {
    // 1. Create the listener instance
    this->listener_ = new StateListener(this->lightPtr);

    // 2. Register it using the new method name
    this->lightPtr->add_target_state_reached_listener(this->listener_);

    // 3. (Your existing HAP setup code continues here...)
    hap_acc_cfg_t acc_cfg = {
        .model = strdup(accessory_info[MODEL]),
        .manufacturer = strdup(accessory_info[MANUFACTURER]),
        .fw_rev = strdup(accessory_info[FW_REV]),
        .cid = HAP_CID_LIGHTBULB,
        .identify_routine = acc_identify,
    };
    // ...
  }
};

}  // namespace homekit
}  // namespace esphome
#endif
