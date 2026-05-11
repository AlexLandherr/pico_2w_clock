#include "wiznet_sntp.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/time.h"

#include "sntp.h"

#define SOCKET_SNTP 1
#define SNTP_TIMEZONE_CODE_UTC 21
#define SNTP_RETRY_INTERVAL_MS 1000
#define SNTP_REFRESH_INTERVAL_MS (60 * 60 * 1000)

static uint8_t sntp_server_ip[4] = {162, 159, 200, 1};
static uint8_t sntp_buf[MAX_SNTP_BUF_SIZE];

static bool initialized = false;
static bool has_time = false;

static datetime last_sntp_datetime;
static absolute_time_t last_sync_time;
static absolute_time_t next_sntp_attempt_time;

static bool time_has_reached(absolute_time_t deadline) {
    return absolute_time_diff_us(get_absolute_time(), deadline) <= 0;
}

static void convert_cached_time_to_hhmmss(wiznet_sntp_time_t *time_out) {
    int64_t elapsed_us = absolute_time_diff_us(last_sync_time, get_absolute_time());

    if (elapsed_us < 0) {
        elapsed_us = 0;
    }

    uint32_t elapsed_seconds = (uint32_t)(elapsed_us / 1000000);

    uint32_t seconds_today =
        ((uint32_t)last_sntp_datetime.hh * 3600) +
        ((uint32_t)last_sntp_datetime.mm * 60) +
        (uint32_t)last_sntp_datetime.ss;

    seconds_today = (seconds_today + elapsed_seconds) % 86400;

    time_out->hours = (uint8_t)(seconds_today / 3600);
    time_out->minutes = (uint8_t)((seconds_today / 60) % 60);
    time_out->seconds = (uint8_t)(seconds_today % 60);
}

void wiznet_sntp_init(void) {
    /*
     * For this first smoke test we use UTC and convert/display local time later.
     * Cloudflare documents 162.159.200.1 as one of its public NTP service addresses.
     */
    SNTP_init(SOCKET_SNTP, sntp_server_ip, SNTP_TIMEZONE_CODE_UTC, sntp_buf);

    initialized = true;
    has_time = false;
    next_sntp_attempt_time = get_absolute_time();

    printf("sntp: initialized, server=%u.%u.%u.%u timezone=UTC code=%u\n",
       sntp_server_ip[0],
       sntp_server_ip[1],
       sntp_server_ip[2],
       sntp_server_ip[3],
       SNTP_TIMEZONE_CODE_UTC);
}

bool wiznet_sntp_get_time(wiznet_sntp_time_t *time_out) {
    if (!initialized || time_out == NULL) {
        return false;
    }

    if (time_has_reached(next_sntp_attempt_time)) {
        datetime new_time;
        int8_t result = SNTP_run(&new_time);

        if (result == 1) {
            last_sntp_datetime = new_time;
            last_sync_time = get_absolute_time();
            has_time = true;

            next_sntp_attempt_time = delayed_by_ms(last_sync_time, SNTP_REFRESH_INTERVAL_MS);

            printf("sntp: sync ok, %04u-%02u-%02u %02u:%02u:%02u UTC\n",
                   new_time.yy,
                   new_time.mo,
                   new_time.dd,
                   new_time.hh,
                   new_time.mm,
                   new_time.ss);
        } else {
            next_sntp_attempt_time = delayed_by_ms(get_absolute_time(), SNTP_RETRY_INTERVAL_MS);

            if (!has_time) {
                printf("sntp: waiting for time, result=%d\n", result);
            }
        }
    }

    if (!has_time) {
        return false;
    }

    convert_cached_time_to_hhmmss(time_out);
    return true;
}