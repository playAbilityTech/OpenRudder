#ifndef _BLE_BUTTON_LED_H_
#define _BLE_BUTTON_LED_H_

#include <stdint.h>

enum class BleLedMode : uint8_t {
    OFF = 0,
    ON = 1,
    BLINK = 2,
};

void ble_button_led_init(void);
void ble_button_led_poll(void);
void ble_status_led_set_mode(BleLedMode mode);
void ble_status_led_set_blink_count(int count);
void ble_status_led_activity_flash(void);

#endif
