#include <stdbool.h>
#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/time.h"

#include "pico_8seg.h"
#include "wiznet_ethernet.h"
#include "wiznet_sntp.h"

int main(void) {
    stdio_init_all();

    sleep_ms(2000);

    printf("pico_2w_clock starting\n");

    pico_8seg_init();
    wiznet_ethernet_init();
    wiznet_sntp_init();

    bool dots_on = false;
    absolute_time_t next_display_update = make_timeout_time_ms(1000);

    while (true) {
        wiznet_ethernet_poll();

        if (absolute_time_diff_us(get_absolute_time(), next_display_update) <= 0) {
            bool ethernet_up = wiznet_ethernet_has_ip();
            wiznet_sntp_time_t network_time;
            bool sntp_time_valid = false;

            if (ethernet_up) {
                sntp_time_valid = wiznet_sntp_get_time(&network_time);
            }

            if (sntp_time_valid) {
                dots_on = (network_time.seconds % 2) == 0;

                pico_8seg_set_hhmm(network_time.hours, network_time.minutes, dots_on);

                printf("display time: %02u:%02u:%02u dots=%s ethernet=%s sntp=%s\n",
                    network_time.hours,
                    network_time.minutes,
                    network_time.seconds,
                    dots_on ? "on" : "off",
                    ethernet_up ? "up" : "waiting-for-dhcp",
                    "time-valid");
            } else {
                dots_on = !dots_on;

                pico_8seg_set_hhmm(12, 34, dots_on);

                printf("display test: 12:34 dots=%s ethernet=%s sntp=%s\n",
                    dots_on ? "on" : "off",
                    ethernet_up ? "up" : "waiting-for-dhcp",
                    "waiting-for-time");
            }

            next_display_update = make_timeout_time_ms(1000);
        }

        sleep_ms(10);
    }
}