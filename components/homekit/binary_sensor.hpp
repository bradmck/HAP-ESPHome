#pragma once
#include <esphome/core/defines.h>
#ifdef USE_BINARY_SENSOR
#include <esphome/core/application.h>
#include <hap.h>
#include <hap_apple_servs.h>
#include <hap_apple_chars.h>
#include "hap_entity.h"

namespace esphome
{
  namespace homekit
  {
    class BinarySensorEntity : public HAPEntity
    {
    private:
      static constexpr const char* TAG = "BinarySensorEntity";
      binary_sensor::BinarySensor* binarySensorPtr;
      static void on_binary_sensor_update(binary_sensor::BinarySensor* obj, bool v) {
        ESP_LOGD(TAG, "%s state: %d", obj->get_name().c_str(), v);
        hap_acc_t* acc = hap_acc_get_by_aid(hap_get_unique_aid(std::to_string(obj->get_object_id_hash()).c_str()));
        if (acc) {
          hap_serv_t* hs = hap_acc_get_first_serv(acc);
          if (hs) {
            hap_char_t* detected_char = hap_serv_get_first_char(hs);
            hap_val_t state;
            state.b = v;
            hap_char_update_val(detected_char, &state);
          }
        }
      }
      static int binary_sensor_read(hap_char_t* hc, hap_status_t* status_code, void* serv_priv, void* read_priv) {
        if (serv_priv) {
          binary_sensor::BinarySensor* binarySensorPtr = (binary_sensor::BinarySensor*)serv_priv;
          ESP_LOGD(TAG, "Read called for Accessory %s (%s)", std::to_string(binarySensorPtr->get_object_id_hash()).c_str(), binarySensorPtr->get_name().c_str());
          hap_val_t binarySensorValue;
          binarySensorValue.b = binarySensorPtr->state;
          hap_char_update_val(hc, &binarySensorValue);
          return HAP_SUCCESS;
        }
        return HAP_FAIL;
      }
      static int acc_identify(hap_acc_t* ha) {
        ESP_LOGI(TAG, "Accessory identified");
        return HAP_SUCCESS;
      }
    public:
      BinarySensorEntity(binary_sensor::BinarySensor* binarySensorPtr) : HAPEntity({{MODEL, "HAP-BINARY-SENSOR"}}), binarySensorPtr(binarySensorPtr) {}
      void setup() {
        hap_serv_t* service = nullptr;

        std::string device_class = binarySensorPtr->get_device_class();
        if (std::equal(device_class.begin(), device_class.end(), strdup("motion"))) {
          service = hap_serv_motion_sensor_create(binarySensorPtr->state);
        }
        else if (std::equal(device_class.begin(), device_class.end(), strdup("occupancy"))) {
          service = hap_serv_occupancy_sensor_create(binarySensorPtr->state);
        }
        else if (std::equal(device_class.begin(), device_class.end(), strdup("door"))) {
          service = hap_serv_contact_sensor_create(binarySensorPtr->state);
        }
        else if (std::equal(device_class.begin(), device_class.end(), strdup("window"))) {
          service = hap_serv_contact_sensor_create(binarySensorPtr->state);
        }
        else if (std::equal(device_class.begin(), device_class.end(), strdup("garage_door"))) {
          service = hap_serv_garage_door_opener_create(binarySensorPtr->state, binarySensorPtr->state, false);
        }
        else if (std::equal(device_class.begin(), device_class.end(), strdup("smoke"))) {
          service = hap_serv_smoke_sensor_create(binarySensorPtr->state);
        }
        else if (std::equal(device_class.begin(), device_class.end(), strdup("fire"))) {
          service = hap_serv_smoke_sensor_create(binarySensorPtr->state);
        }
        else if (std::equal(device_class.begin(), device_class.end(), strdup("water"))) {
          service = hap_serv_leak_sensor_create(binarySensorPtr->state);
        }
        else {
          // Default to motion sensor
          service = hap_serv_motion_sensor_create(binarySensorPtr->state);
        }
        if (service) {
          hap_acc_cfg_t acc_cfg = {
              .model = strdup(accessory_info[MODEL]),
              .manufacturer = strdup(accessory_info[MANUFACTURER]),
              .fw_rev = strdup(accessory_info[FW_REV]),
              .hw_rev = NULL,
              .pv = strdup("1.1.0"),
              .cid = HAP_CID_SENSOR,
              .identify_routine = acc_identify,
          };
          hap_acc_t* accessory = nullptr;
          std::string accessory_name = binarySensorPtr->get_name();
          if (accessory_info[NAME] == NULL) {
            acc_cfg.name = strdup(accessory_name.c_str());
          }
          else {
            acc_cfg.name = strdup(accessory_info[NAME]);
          }
          if (accessory_info[SN] == NULL) {
            acc_cfg.serial_num = strdup(std::to_string(binarySensorPtr->get_object_id_hash()).c_str());
          }
          else {
            acc_cfg.serial_num = strdup(accessory_info[SN]);
          }
          accessory = hap_acc_create(&acc_cfg);
          ESP_LOGD(TAG, "ID HASH: %lu", binarySensorPtr->get_object_id_hash());
          hap_serv_set_priv(service, binarySensorPtr);

          /* Set the read callback for the service */
          hap_serv_set_read_cb(service, binary_sensor_read);

          /* Add the Binary Sensor Service to the Accessory Object */
          hap_acc_add_serv(accessory, service);

          /* Add the Accessory to the HomeKit Database */
          hap_add_bridged_accessory(accessory, hap_get_unique_aid(std::to_string(binarySensorPtr->get_object_id_hash()).c_str()));
          if (!binarySensorPtr->is_internal())
            binarySensorPtr->add_on_state_callback([this](bool v) { BinarySensorEntity::on_binary_sensor_update(binarySensorPtr, v); });
          ESP_LOGI(TAG, "Binary Sensor '%s' linked to HomeKit", accessory_name.c_str());
        }
      }
    };
  }
}
#endif
