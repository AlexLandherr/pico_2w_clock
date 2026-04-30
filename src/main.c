#include <stdbool.h>
#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/time.h"

#include "pico_8seg.h"
#include "wiznet_ethernet.h"

int main(void) {
    stdio_init_all();

    sleep_ms(2000);

    printf("pico_2w_clock starting\n");

    pico_8seg_init();
    wiznet_ethernet_init();

    bool dots_on = false;
    absolute_time_t next_display_update = make_timeout_time_ms(1000);

    while (true) {
        wiznet_ethernet_poll();

        if (absolute_time_diff_us(get_absolute_time(), next_display_update) <= 0) {
            dots_on = !dots_on;

            pico_8seg_set_hhmm(12, 34, dots_on);

            printf("display test: 12:34 dots=%s ethernet=%s\n",
                   dots_on ? "on" : "off",
                   wiznet_ethernet_has_ip() ? "up" : "waiting-for-dhcp");

            next_display_update = make_timeout_time_ms(1000);
        }

        sleep_ms(10);
    }
}