#ifndef WIZNET_SNTP_H
#define WIZNET_SNTP_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
} wiznet_sntp_time_t;

void wiznet_sntp_init(void);
bool wiznet_sntp_get_time(wiznet_sntp_time_t *time_out);

#endif