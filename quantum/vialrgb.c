/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Per-key host control, ported from Vial's quantum/vialrgb.c. */

#include "vialrgb.h"

#include <inttypes.h>
#include <string.h>
#include "quantum.h"
#include "rgb_matrix.h"

#ifndef RGB_MATRIX_MAXIMUM_BRIGHTNESS
#    define RGB_MATRIX_MAXIMUM_BRIGHTNESS UINT8_MAX
#endif

typedef struct {
    uint16_t vialrgb_id;
    uint16_t qmk_id;
} vialrgb_supported_mode_t;

#include "vialrgb_effects.inc"

#define SUPPORTED_MODES_LENGTH (sizeof(supported_modes) / sizeof(*supported_modes))

#ifdef RGB_MATRIX_EFFECT_VIALRGB_DIRECT
HSV g_direct_mode_colors[RGB_MATRIX_LED_COUNT];
#endif

static void get_supported(uint8_t *args, uint8_t length) {
    /* report supported effects (VialRGB ids) greater than the one the host already has */
    uint16_t gt;
    memcpy(&gt, args, sizeof(gt));
    memset(args, 0xFF, length);
    for (size_t i = 0; i < SUPPORTED_MODES_LENGTH; ++i) {
        uint16_t id = supported_modes[i].vialrgb_id;
        if (id > gt && length >= sizeof(id)) {
            memcpy(args, &id, sizeof(id));
            length -= sizeof(id);
            args += sizeof(id);
        }
    }
}

static uint16_t qmk_id_to_vialrgb_id(uint16_t id) {
    for (size_t i = 0; i < SUPPORTED_MODES_LENGTH; ++i) {
        if (supported_modes[i].qmk_id == id) {
            return supported_modes[i].vialrgb_id;
        }
    }
    return 0;
}

static uint16_t vialrgb_id_to_qmk_id(uint16_t id) {
    for (size_t i = 0; i < SUPPORTED_MODES_LENGTH; ++i) {
        if (supported_modes[i].vialrgb_id == id) {
            return supported_modes[i].qmk_id;
        }
    }
    return 0;
}

static uint16_t get_mode(void) {
    if (!rgb_matrix_is_enabled()) {
        return VIALRGB_EFFECT_OFF;
    }
    return qmk_id_to_vialrgb_id(rgb_matrix_get_mode());
}

static void set_mode(uint16_t mode) {
    if (mode == VIALRGB_EFFECT_OFF) {
        rgb_matrix_disable_noeeprom();
    } else {
        rgb_matrix_enable_noeeprom();
        rgb_matrix_mode_noeeprom(vialrgb_id_to_qmk_id(mode));
    }
}

#ifdef RGB_MATRIX_EFFECT_VIALRGB_DIRECT
static void get_matrix_pos_for_led(uint16_t led, uint8_t *output) {
    /* 0xFF means the LED is not part of the key matrix */
    output[0] = output[1] = 0xFF;
    for (size_t row = 0; row < MATRIX_ROWS; ++row) {
        for (size_t col = 0; col < MATRIX_COLS; ++col) {
            if (g_led_config.matrix_co[row][col] == led) {
                output[0] = row;
                output[1] = col;
                return;
            }
        }
    }
}

static void fast_set_leds(uint8_t *args, size_t length) {
    /* u16 first LED index, u8 count, then HSV per LED, 9 LEDs per 32-byte packet */
    if (length < 3) return;

    uint16_t first_index = args[0] | (args[1] << 8);
    uint8_t  num_leds    = args[2];
    length -= 3;
    args += 3;

    if (num_leds * 3 > length) return;

    for (size_t i = 0; i < num_leds; ++i) {
        if (i + first_index >= RGB_MATRIX_LED_COUNT) break;
        uint8_t val                                     = args[i * 3 + 2];
        g_direct_mode_colors[i + first_index].h         = args[i * 3 + 0];
        g_direct_mode_colors[i + first_index].s         = args[i * 3 + 1];
        g_direct_mode_colors[i + first_index].v         = (val > RGB_MATRIX_MAXIMUM_BRIGHTNESS) ? RGB_MATRIX_MAXIMUM_BRIGHTNESS : val;
    }
}
#endif

void vialrgb_get_value(uint8_t *data, uint8_t length) {
    if (length != VIAL_RAW_EPSIZE) return;

    uint8_t  cmd  = data[1];
    uint8_t *args = &data[2];

    switch (cmd) {
        case vialrgb_get_info:
            args[0] = VIALRGB_PROTOCOL_VERSION & 0xFF;
            args[1] = VIALRGB_PROTOCOL_VERSION >> 8;
            args[2] = RGB_MATRIX_MAXIMUM_BRIGHTNESS;
            break;
        case vialrgb_get_mode: {
            uint16_t mode = get_mode();
            args[0]       = mode & 0xFF;
            args[1]       = mode >> 8;
            args[2]       = rgb_matrix_get_speed();
            args[3]       = rgb_matrix_get_hue();
            args[4]       = rgb_matrix_get_sat();
            args[5]       = rgb_matrix_get_val();
            break;
        }
        case vialrgb_get_supported:
            get_supported(args, length - 2);
            break;
#ifdef RGB_MATRIX_EFFECT_VIALRGB_DIRECT
        case vialrgb_get_number_leds:
            args[0] = RGB_MATRIX_LED_COUNT & 0xFF;
            args[1] = RGB_MATRIX_LED_COUNT >> 8;
            break;
        case vialrgb_get_led_info: {
            uint16_t led = (args[0] & 0xFF) | ((uint16_t)args[1] << 8);
            if (led >= RGB_MATRIX_LED_COUNT) return;
            args[0] = g_led_config.point[led].x;
            args[1] = g_led_config.point[led].y;
            args[2] = g_led_config.flags[led];
            get_matrix_pos_for_led(led, &args[3]);
            break;
        }
#endif
        default:
            break;
    }
}

void vialrgb_set_value(uint8_t *data, uint8_t length) {
    if (length != VIAL_RAW_EPSIZE) return;

    uint8_t  cmd  = data[1];
    uint8_t *args = &data[2];

    switch (cmd) {
        case vialrgb_set_mode: {
            uint16_t mode = args[0] | (args[1] << 8);
            set_mode(mode);
            rgb_matrix_set_speed_noeeprom(args[2]);
            rgb_matrix_sethsv_noeeprom(args[3], args[4], args[5]);
            break;
        }
#ifdef RGB_MATRIX_EFFECT_VIALRGB_DIRECT
        case vialrgb_direct_fastset:
            fast_set_leds(args, length);
            break;
#endif
        default:
            break;
    }
}

void vialrgb_save(uint8_t *data, uint8_t length) {
    (void)data;
    (void)length;
    eeconfig_update_rgb_matrix();
}
