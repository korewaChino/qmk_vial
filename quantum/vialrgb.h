/* SPDX-License-Identifier: GPL-2.0-or-later */

#pragma once

#include <inttypes.h>

/* VialRGB speaks over the VIA raw HID command space with sub-ids from 0x40 */
#define VIALRGB_PROTOCOL_VERSION 1
#define VIAL_RAW_EPSIZE 32

enum {
    vialrgb_set_mode        = 0x41,
    vialrgb_direct_fastset  = 0x42,
};

enum {
    vialrgb_get_info        = 0x40,
    vialrgb_get_mode        = 0x41,
    vialrgb_get_supported   = 0x42,
    vialrgb_get_number_leds = 0x43,
    vialrgb_get_led_info    = 0x44,
};

void vialrgb_get_value(uint8_t *data, uint8_t length);
void vialrgb_set_value(uint8_t *data, uint8_t length);
void vialrgb_save(uint8_t *data, uint8_t length);

#if defined(VIALRGB_ENABLE) && !defined(RGB_MATRIX_ENABLE)
#    error VIALRGB_ENABLE=yes requires RGB_MATRIX_ENABLE=yes
#endif
