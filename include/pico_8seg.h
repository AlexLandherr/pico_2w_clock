#ifndef PICO_8SEG_H
#define PICO_8SEG_H

#include <stdbool.h>
#include <stdint.h>

void pico_8seg_init(void);
void pico_8seg_set_hhmm(uint8_t hours, uint8_t minutes, bool dots_on);
void pico_8seg_clear(void);

#endif
