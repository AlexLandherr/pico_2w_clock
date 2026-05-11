#include "wiznet_sntp.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/time.h"

#include "sntp.h"

#define SOCKET_SNTP 1

#ifndef WIZNET_SNTP_TIMEZONE_CODE
#define WIZNET_SNTP_TIMEZONE_CODE 21
#endif

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

static int16_t wiznet_timezone_code_to_offset_minutes(uint8_t timezone_code) {
    switch (timezone_code) {
        case 0:
            return -12 * 60;
        case 1:
            return -11 * 60;
        case 2:
            return -10 * 60;
        case 3:
            return -(9 * 60 + 30);
        case 4:
            return -9 * 60;
        case 5:
        case 6:
            return -8 * 60;
        case 7:
        case 8:
            return -7 * 60;
        case 9:
        case 10:
            return -6 * 60;
        case 11:
        case 12:
        case 13:
            return -5 * 60;
        case 14:
            return -(4 * 60 + 30);
        case 15:
        case 16:
            return -4 * 60;
        case 17:
            return -(3 * 60 + 30);
        case 18:
            return -3 * 60;
        case 19:
            return -2 * 60;
        case 20:
            return -1 * 60;
        case 21:
        case 22:
            return 0;
        case 23:
        case 24:
        case 25:
            return 1 * 60;
        case 26:
        case 27:
            return 2 * 60;
        case 28:
        case 29:
            return 3 * 60;
        case 30:
            return 3 * 60 + 30;
        case 31:
            return 4 * 60;
        case 32:
            return 4 * 60 + 30;
        case 33:
            return 5 * 60;
        case 34:
            return 5 * 60 + 30;
        case 35:
            return 5 * 60 + 45;
        case 36:
            return 6 * 60;
        case 37:
            return 6 * 60 + 30;
        case 38:
            return 7 * 60;
        case 39:
            return 8 * 60;
        case 40:
            return 9 * 60;
        case 41:
            return 9 * 60 + 30;
        case 42:
            return 10 * 60;
        case 43:
            return 10 * 60 + 30;
        case 44:
            return 11 * 60;
        case 45:
            return 11 * 60 + 30;
        case 46:
            return 12 * 60;
        case 47:
            return 12 * 60 + 45;
        case 48:
            return 13 * 60;
        case 49:
            return 14 * 60;
        default:
            return 0;
    }
}

static void format_timezone_offset(char *buffer, size_t buffer_size, uint8_t timezone_code) {
    int16_t offset_minutes = wiznet_timezone_code_to_offset_minutes(timezone_code);
    char sign = '+';

    if (offset_minutes < 0) {
        sign = '-';
        offset_minutes = (int16_t)-offset_minutes;
    }

    uint8_t hours = (uint8_t)(offset_minutes / 60);
    uint8_t minutes = (uint8_t)(offset_minutes % 60);

    if (hours == 0 && minutes == 0) {
        snprintf(buffer, buffer_size, "+00:00 (UTC)");
    } else {
        snprintf(buffer, buffer_size, "%c%02u:%02u", sign, hours, minutes);
    }
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
    SNTP_init(SOCKET_SNTP, sntp_server_ip, WIZNET_SNTP_TIMEZONE_CODE, sntp_buf);

    initialized = true;
    has_time = false;
    next_sntp_attempt_time = get_absolute_time();

    printf("sntp: initialized, server=%u.%u.%u.%u timezone-code=%u\n",
           sntp_server_ip[0],
           sntp_server_ip[1],
           sntp_server_ip[2],
           sntp_server_ip[3],
           WIZNET_SNTP_TIMEZONE_CODE);
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

            char timezone_offset_text[16];

            format_timezone_offset(timezone_offset_text,
                                sizeof(timezone_offset_text),
                                WIZNET_SNTP_TIMEZONE_CODE);

            printf("sntp: sync ok, %04u-%02u-%02u %02u:%02u:%02u%s\n",
                new_time.yy,
                new_time.mo,
                new_time.dd,
                new_time.hh,
                new_time.mm,
                new_time.ss,
                timezone_offset_text
            );
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