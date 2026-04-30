#include "pico_8seg.h"

#include "pico/stdlib.h"
#include "pico/time.h"

#define PIN_RCLK 9
#define PIN_CLK  10
#define PIN_DIN  11

#define SEG_DP 0x80

static repeating_timer_t scan_timer;
static volatile uint8_t display_buffer[4] = {0x00, 0x00, 0x00, 0x00};
static volatile uint8_t scan_index = 0;

/*
 * Waveshare-style active-high segment codes.
 *
 * bit 0 = a
 * bit 1 = b
 * bit 2 = c
 * bit 3 = d
 * bit 4 = e
 * bit 5 = f
 * bit 6 = g
 * bit 7 = dp
 */
static const uint8_t digit_segments[10] = {
    0x3f, /* 0 */
    0x06, /* 1 */
    0x5b, /* 2 */
    0x4f, /* 3 */
    0x66, /* 4 */
    0x6d, /* 5 */
    0x7d, /* 6 */
    0x07, /* 7 */
    0x7f, /* 8 */
    0x6f  /* 9 */
};

/*
 * Waveshare-style active-low digit select bytes.
 * Left-to-right should be: 0xFE, 0xFD, 0xFB, 0xF7
 */
static const uint8_t digit_select[4] = {
    0xfe,
    0xfd,
    0xfb,
    0xf7
};

static void pulse_clock(uint pin) {
    gpio_put(pin, 1);
    sleep_us(1);
    gpio_put(pin, 0);
    sleep_us(1);
}

static void shift_out_u8(uint8_t value) {
    for (int bit = 0; bit < 8; bit++) {
        gpio_put(PIN_DIN, (value & 0x80) != 0);
        pulse_clock(PIN_CLK);
        value <<= 1;
    }
}

static void write_raw(uint8_t digit, uint8_t segments) {
    /*
     * Mimic the Waveshare demo order:
     * first digit-select byte, then segment byte.
     */
    gpio_put(PIN_RCLK, 1);

    shift_out_u8(digit);
    shift_out_u8(segments);

    gpio_put(PIN_RCLK, 0);
    sleep_us(1);
    gpio_put(PIN_RCLK, 1);
}

static bool scan_callback(repeating_timer_t *timer) {
    (void)timer;

    uint8_t index = scan_index;
    write_raw(digit_select[index], display_buffer[index]);

    scan_index = (uint8_t)((index + 1) & 0x03);
    return true;
}

void pico_8seg_init(void) {
    gpio_init(PIN_RCLK);
    gpio_init(PIN_CLK);
    gpio_init(PIN_DIN);

    gpio_set_dir(PIN_RCLK, GPIO_OUT);
    gpio_set_dir(PIN_CLK, GPIO_OUT);
    gpio_set_dir(PIN_DIN, GPIO_OUT);

    gpio_put(PIN_RCLK, 1);
    gpio_put(PIN_CLK, 0);
    gpio_put(PIN_DIN, 0);

    pico_8seg_clear();

    add_repeating_timer_ms(-2, scan_callback, NULL, &scan_timer);
}

void pico_8seg_set_hhmm(uint8_t hours, uint8_t minutes, bool dots_on) {
    hours %= 24;
    minutes %= 60;

    display_buffer[0] = digit_segments[hours / 10];
    display_buffer[1] = digit_segments[hours % 10];
    display_buffer[2] = digit_segments[minutes / 10];
    display_buffer[3] = digit_segments[minutes % 10];

    /*
     * Put the blinking dot after the hour ones digit,
     * so 12.34-ish. We can adjust this later if the physical
     * dot location is not the one you want.
     */
    if (dots_on) {
        display_buffer[1] |= SEG_DP;
    } else {
        display_buffer[1] &= (uint8_t)~SEG_DP;
    }
}

void pico_8seg_clear(void) {
    display_buffer[0] = 0x00;
    display_buffer[1] = 0x00;
    display_buffer[2] = 0x00;
    display_buffer[3] = 0x00;
}
