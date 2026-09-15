#include "keys.h"

#include "hardware/gpio.h"

#define KEY0_GPIO 15u
#define KEY1_GPIO 17u

static uint gpio_for(RemoteInputKey key) {
    return key == RemoteInputKey0 ? KEY0_GPIO : KEY1_GPIO;
}

static void configure_key(uint gpio) {
    gpio_init(gpio);
    gpio_set_dir(gpio, false);
    gpio_pull_up(gpio);
}

void keys_initialise(void) {
    configure_key(KEY0_GPIO);
    configure_key(KEY1_GPIO);
}

bool keys_is_pressed(RemoteInputKey key) {
    return !gpio_get(gpio_for(key));
}
