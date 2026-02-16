#include <esphome/core/defines.h>
#ifdef USE_LIGHT
#pragma once
#include <esphome/core/application.h>
#include <hap.h>
#include <hap_apple_servs.h>
#include <hap_apple_chars.h>
#include "hap_entity.h"

namespace esphome
{
  namespace homekit
  {
    class LightEntity : public HAPEntity
    {
    private:
      static constexpr const char* TAG = "LightEntity";
      light::LightState* lightPtr;

      // Listener class to handle state changes in modern ESPHome versions
      class StateListener : public light::LightTargetStateReachedListener {
       public:
        explicit StateListener(light::LightState* parent) : parent_(parent) {}
        // FIX: Method name updated to match the ESPHome base class
        void on_light_target_state_reached() override {
          LightEntity::on_light_update(parent_);
        }
       private:
        light::LightState* parent_;
      };

      StateListener* listener_{nullptr};

      static int light_write(hap_write_data_t write_data[], int count, void* serv_priv, void* write_priv) {
        light::LightState* lightPtr = (light::LightState*)serv_priv;
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
          else if (!strcmp(hap_char_get_type_uuid(write->hc), HAP_CHAR_UUID_BRIGHTNESS)) {
            lightPtr->make_call().set_save(true).set_brightness((float)(write->val.i) / 100.0f).perform();
            hap_char_update_val(write->hc, &(write->val));
            *(write->status) = HAP_STATUS_SUCCESS;
          }
          else if (!strcmp(hap_char_get_type_uuid(write->hc), HAP_CHAR_UUID_HUE)) {
            int hue = 0; float sat = 0; float val = 0;
            rgb_to_hsv(lightPtr->remote_values.get_red(), lightPtr->remote_values.get_green(), lightPtr->remote_values.get_blue(), hue, sat, val);
            float tR, tG, tB;
            hsv_to_rgb(write->val.f, sat, val, tR, tG, tB);
            lightPtr->make_call().set_rgb(tR, tG, tB).set_save(true).perform();
            hap_char_update_val(write->hc, &(write->val));
            *(write->status) = HAP_STATUS_SUCCESS;
          }
          else if (!strcmp(hap_char_get_type_uuid(write->hc), HAP_CHAR_UUID_SATURATION)) {
            int hue = 0; float sat = 0; float val = 0;
            rgb_to_hsv(lightPtr->remote_values.get_red(), lightPtr->remote_values.get_green(), lightPtr->remote_values.get_blue(), hue, sat, val);
            float tR, tG, tB;
            hsv_to_rgb(hue, write->val.f / 100.0f, val, tR, tG, tB);
            lightPtr->make_call().set_rgb(tR, tG, tB).set_save(true).perform();
            hap_char_update_val(write->hc, &(write->val));
            *(write->status) = HAP_STATUS_SUCCESS;
          }
          else if (!strcmp(hap_char_get_type_uuid(write->hc), HAP_CHAR_UUID_COLOR_TEMPERATURE)) {
            lightPtr->make_call().set_color_temperature(write->val.u).set_save(true).perform();
            hap_char_update_val(write->hc, &(write->val));
            *(write->status) = HAP_STATUS_SUCCESS;
          }
          else {
            *(write->status) = HAP_STATUS_RES_ABSENT;
          }
        }
        return ret;
      }

      static void on_light_update(light::LightState* obj) {
        bool rgb = obj->current_values.get_color_mode() & light::ColorCapability::RGB;
        bool level = obj->get_traits().supports_color_capability(light::ColorCapability::BRIGHTNESS);
        bool temperature = obj->current_values.get_color_mode() & (light::ColorCapability::COLOR_TEMPERATURE | light::ColorCapability::COLD_WARM_WHITE);

        hap_acc_t* acc = hap_acc_get_by_aid(hap_get_unique_aid(std::to_string(obj->get_object_id_hash()).c_str()));
        if (acc) {
          hap_serv_t* hs = hap_acc_get_serv_by_uuid(acc, HAP_SERV_UUID_LIGHTBULB);
          hap_char_t* on_char = hap_serv_get_char_by_uuid(hs, HAP_CHAR_UUID_ON);
          hap_val_t state;
          state.b = obj->current_values.get_state() > 0.01f;
          hap_char_update_val(on_char, &state);

          if (level) {
            hap_char_t* level_char = hap_serv_get_char_by_uuid(hs, HAP_CHAR_UUID_BRIGHTNESS);
            hap_val_t lv;
            lv.i = (int)(obj->current_values.get_brightness() * 100);
            hap_char_update_val(level_char, &lv);
          }
          if (rgb) {
            hap_char_t* h_char = hap_serv_get_char_by_uuid(hs, HAP_CHAR_UUID_HUE);
            hap_char_t* s_char = hap_serv_get_char_by_uuid(hs, HAP_CHAR_UUID_SATURATION);
            int cHue = 0; float cSat = 0; float cVal = 0;
            rgb_to_hsv(obj->current_values.get_red(), obj->current_values.get_green(), obj->current_values.get_blue(), cHue, cSat, cVal);
            hap_val_t h, s;
            h.f = cHue; s.f = cSat * 100.0f;
            hap_char_update_val(h_char, &h);
            hap_char_update_val(s_char, &s);
          }
          if (temperature) {
            hap_char_t* t_char = hap_serv_get_char_by_uuid(hs, HAP_CHAR_UUID_COLOR_TEMPERATURE);
            hap_val_t t;
            t.u = (uint32_t)obj->current_values.get_color_temperature();
            hap_char_update_val(t_char, &t);
          }
        }
      }

      static int acc_identify(hap_acc_t* ha) {
        ESP_LOGI(TAG, "Accessory identified");
        return HAP_SUCCESS;
      }

    public:
      LightEntity(light::LightState* lightPtr) : HAPEntity({{MODEL, "HAP-LIGHT"}}), lightPtr(lightPtr) {}
      
      void setup() override {
        // Register the state listener
        this->listener_ = new StateListener(this->lightPtr);
        this->lightPtr->add_target_state_reached_listener(this->listener_);

        hap_acc_cfg_t acc_cfg = {
            .model = strdup(accessory_info[MODEL]),
            .manufacturer = strdup(accessory_info[MANUFACTURER]),
            .fw_rev = strdup(accessory_info[FW_REV]),
            .hw_rev = NULL,
            .pv = strdup("1.1.0"),
            // FIX: Changed HAP_CID_LIGHTBULB to HAP_CID_LIGHTING
            .cid = HAP_CID_LIGHTING,
            .identify_routine = acc_identify,
        };
        // HAP initialization logic follows...
      }
    };
  }
}
#endif
