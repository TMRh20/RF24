#include "gpio.h"

GPIO::GPIO()
{
}

void GPIO::open(int port, int DDR) {
    gpio_init(port);
    gpio_set_dir(port, DDR);
}

void GPIO::close(int port) {
    gpio_init(port);
}

int GPIO::read(int port) {
    return gpio_get(port);
}

void GPIO::write(int port, int value) {
    gpio_put(port, value);
}

GPIO::~GPIO()
{
}
