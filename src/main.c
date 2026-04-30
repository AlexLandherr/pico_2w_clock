#include <stdbool.h>
#include <stdio.h>

#include "pico/stdlib.h"
#include "pico_8seg.h"

int main(void) {
    stdio_init_all();

    sleep_ms(1500);

    printf("pico_2w_clock starting\n");

    pico_8seg_init();

    bool dots_on = false;

    while (true) {
        dots_on = !dots_on;

        pico_8seg_set_hhmm(12, 34, dots_on);

        printf("display test: 12:34 dots=%s\n", dots_on ? "on" : "off");

        sleep_ms(1000);
    }
}
