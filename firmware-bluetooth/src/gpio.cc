#include <string.h>

#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "globals.h"
#include "gpio.h"
#include "platform.h"
#include "remapper.h"

LOG_MODULE_REGISTER(gpio, LOG_LEVEL_DBG);

static const struct device* gpio0_dev = DEVICE_DT_GET(DT_NODELABEL(gpio0));

static uint32_t gpio_valid_pins_mask;
static uint32_t gpio_in_mask;
static uint32_t gpio_out_mask;
static uint32_t prev_gpio_state;
static uint64_t last_gpio_change[32];
static volatile bool set_gpio_dir_pending;

uint32_t get_gpio_valid_pins_mask() {
#if defined(CONFIG_BOARD_SEEED_XIAO_NRF52840)
    // Header pins D1-D3. D4/D5 are free on BLE Sense (I2C moved to P0.07/P0.27).
    uint32_t mask = BIT(3) | BIT(28) | BIT(29);
#if DT_NODE_EXISTS(DT_NODELABEL(lsm6ds3tr_c))
    mask |= BIT(4) | BIT(5);
#endif
    return mask;
#elif defined(CONFIG_BOARD_ADAFRUIT_FEATHER_NRF52840)
    // P0 pins exposed on the Feather header (P1 pins are outside the GPIO 0-29 scheme).
    return BIT(4) | BIT(5) | BIT(12) | BIT(13) | BIT(14) | BIT(15) | BIT(26) | BIT(27) | BIT(28) | BIT(29) | BIT(30) | BIT(31);
#else
    // Default: P0 pins except the QSPI flash interface.
    return 0x03FF6FFFu & ~(BIT(17) | BIT(19) | BIT(20) | BIT(21) | BIT(22) | BIT(23));
#endif
}

void gpio_pins_init() {
    gpio_valid_pins_mask = get_gpio_valid_pins_mask();
    if (!device_is_ready(gpio0_dev)) {
        LOG_ERR("gpio0 not ready");
        return;
    }
    prev_gpio_state = 0;
    memset(last_gpio_change, 0, sizeof(last_gpio_change));
}

void set_gpio_inout_masks(uint32_t in_mask, uint32_t out_mask) {
    gpio_out_mask = (out_mask & ~in_mask) & gpio_valid_pins_mask;
    // Treat all valid pins except outputs as inputs so the monitor works.
    gpio_in_mask = gpio_valid_pins_mask & ~gpio_out_mask;
    set_gpio_dir_pending = true;
}

static void configure_pin_input(int pin) {
    gpio_pin_configure(gpio0_dev, pin, GPIO_INPUT | GPIO_PULL_UP);
}

static void set_gpio_dir() {
    for (uint8_t i = 0; i <= 31; i++) {
        uint32_t bit = BIT(i);
        if (gpio_valid_pins_mask & bit && gpio_in_mask & bit) {
            configure_pin_input(i);
        }
    }
}

bool read_gpio(uint64_t now) {
    if (!device_is_ready(gpio0_dev)) {
        return false;
    }

    uint32_t gpio_state = 0;
    for (uint8_t i = 0; i <= 31; i++) {
        if (gpio_in_mask & BIT(i)) {
            int val = gpio_pin_get(gpio0_dev, i);
            if (val > 0) {
                gpio_state |= BIT(i);
            }
        }
    }

    uint32_t changed = prev_gpio_state ^ gpio_state;
    if (changed != 0) {
        for (uint8_t i = 0; i <= 31; i++) {
            uint32_t bit = BIT(i);
            if (changed & bit) {
                if (last_gpio_change[i] + gpio_debounce_time <= now) {
                    uint32_t usage = GPIO_USAGE_PAGE | i;
                    int32_t state = !(gpio_state & bit);  // active low
                    set_input_state(usage, state, state);
                    if (monitor_enabled) {
                        monitor_usage(usage, state, 0);
                    }
                    last_gpio_change[i] = now;
                } else {
                    gpio_state ^= bit;
                    changed ^= bit;
                }
            }
        }
        prev_gpio_state = gpio_state;
    }
    return changed != 0;
}

void write_gpio() {
    if (suspended || !device_is_ready(gpio0_dev)) {
        return;
    }

    uint32_t value = gpio_out_state[0] | (gpio_out_state[1] << 8) | (gpio_out_state[2] << 16) | (gpio_out_state[3] << 24);
    for (uint8_t i = 0; i <= 31; i++) {
        uint32_t bit = BIT(i);
        if (!(gpio_out_mask & bit)) {
            continue;
        }
        switch (gpio_output_mode) {
            case 0:
                gpio_pin_configure(gpio0_dev, i, GPIO_OUTPUT);
                gpio_pin_set(gpio0_dev, i, (value & bit) ? 1 : 0);
                break;
            case 1:
                if (value & bit) {
                    configure_pin_input(i);
                } else {
                    gpio_pin_configure(gpio0_dev, i, GPIO_OUTPUT);
                    gpio_pin_set(gpio0_dev, i, 0);
                }
                break;
        }
    }
    memset(gpio_out_state, 0, sizeof(gpio_out_state));
}

void gpio_process_pending_dir() {
    if (set_gpio_dir_pending && !suspended) {
        set_gpio_dir();
        set_gpio_dir_pending = false;
    }
}
