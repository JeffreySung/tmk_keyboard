/*
Copyright 2011 Jun Wako <wakojun@gmail.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "keyboard.h"
#include "host.h"
#include "layer.h"
#include "matrix.h"
#include "led.h"
#include "usb_keycodes.h"
#include "timer.h"
#include "print.h"
#include "debug.h"
#include "command.h"
#ifdef MOUSEKEY_ENABLE
#include "mousekey.h"
#endif
#ifdef EXTRAKEY_ENABLE
#include <util/delay.h>
#endif


static uint8_t last_leds = 0;


void keyboard_init(void)
{
    timer_init();
    matrix_init();
#ifdef PS2_MOUSE_ENABLE
    ps2_mouse_init();
#endif
}

void keyboard_proc(void)
{
    uint8_t fn_bits = 0;
	uint8_t	tmpCode;
#ifdef EXTRAKEY_ENABLE
    uint16_t consumer_code = 0;
    uint16_t system_code = 0;
#endif

    matrix_scan();

    if (matrix_is_modified()) {
        if (debug_matrix) matrix_print();
#ifdef DEBUG_LED
        // LED flash for debug
        DEBUG_LED_CONFIG;
        DEBUG_LED_ON;
#endif
    }

    if (matrix_has_ghost()) {
        // should send error?
        debug("matrix has ghost!!\n");
        return;
    }

    host_swap_keyboard_report();
    host_clear_keyboard_report();
    for (int row = 0; row < matrix_rows(); row++) {
        for (int col = 0; col < matrix_cols(); col++) {
            if (!matrix_is_on(row, col)) continue;

            uint16_t code = layer_get_keycode(row, col);
			tmpCode = (uint8_t)(code );
			// Normal Keycode
			if ( (code & 0xFF00) == 0x0700 ) {
            	if (code == KB_NO) {
            	    // do nothing
            	} else if (IS_MOD(code)) {
            	    host_add_mod_bit(MOD_BIT(tmpCode));
            	} else if (IS_FN(code)) {
            	    fn_bits |= FN_BIT(tmpCode);
            	} else {
                	host_add_key(tmpCode);
				}
			} 
	#ifdef EXTRAKEY_ENABLE
	        // System Control
			else if ( (code & 0xFF00) == 0x0100 ) {
	            if (code == KB_SYSTEM_POWER) {
	#ifdef HOST_PJRC
	                if (suspend && remote_wakeup) {
	                    usb_remote_wakeup();
	                }
	#endif
	                system_code = SYSTEM_POWER_DOWN;
	            } else if (code == KB_SYSTEM_SLEEP) {
	                system_code = SYSTEM_SLEEP;
	            } else if (code == KB_SYSTEM_WAKE) {
	                system_code = SYSTEM_WAKE_UP;
	            }
			} 
            // Consumer Page
			else if ( (code & 0xF000) == 0xC000 ) {
            	consumer_code = (code & 0x0FFF);
			}
#endif
#ifdef MOUSEKEY_ENABLE
            else if (IS_MOUSEKEY(tmpCode)) {
                mousekey_decode(tmpCode);
            }
#endif
            else {
                debug("ignore keycode: "); debug_hex(tmpCode); debug("\n");
            }
        }
    }

    layer_switching(fn_bits);

    if (command_proc()) {
        return;
    }

    // TODO: should send only when changed from last report
    if (matrix_is_modified()) {
        host_send_keyboard_report();
#ifdef EXTRAKEY_ENABLE
        host_consumer_send(consumer_code);
        host_system_send(system_code);
#endif
#ifdef DEBUG_LED
        // LED flash for debug
        DEBUG_LED_CONFIG;
        DEBUG_LED_OFF;
#endif
    }

#ifdef MOUSEKEY_ENABLE
    mousekey_send();
#endif

#ifdef PS2_MOUSE_ENABLE
    // TODO: should comform new API
    if (ps2_mouse_read() == 0)
        ps2_mouse_usb_send();
#endif

    if (last_leds != host_keyboard_leds()) {
        keyboard_set_leds(host_keyboard_leds());
        last_leds = host_keyboard_leds();
    }
}

void keyboard_set_leds(uint8_t leds)
{
    led_set(leds);
}
