/*
 * pico-model-m.h - use an RP2040 chip directly connected to an IBM Model M
 *                  membrane to provide a USB HID Keyboard
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 guruthree
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

// everything for the special functions

bool macrorecording = false;
// record the macro here
std::vector< std::vector<uint8_t> > macro_scancode(NUM_MACROS, std::vector<uint8_t>(0));
std::vector< std::vector<uint8_t> > macro_pressed(NUM_MACROS, std::vector<uint8_t>(0));;
uint8_t activemacro = 0;


void handleSpecial(uint8_t down, uint8_t across, bool pressed) { // pressed or released
    bool doprocess = false;
    uint8_t c;
    for (c = 0; c < specials.size(); c++) {
        if (!specials[c].twokey) {
            if (specials[c].down == down && specials[c].across == across) {
                doprocess = true;
                break;
            }
        }
        else { // need to check for two keys being pressed
//            if (pinstate[specials[c].down][specials[c].across] && pinstate[specials[c].down2][specials[c].across2]) {
            if (pinstate[specials[c].down][specials[c].across] && specials[c].down2 == down && specials[c].across2 == across) { // activate on the second key
                doprocess = true;
                break;
            }
        }
    }
    if (!doprocess) {
        return;
    }

    switch (specials[c].type) {
        case SPECIAL_TYPE:
            if (pressed) {
                Keyboard.type(specials[c].topress);
            }
            break;
        case SPECIAL_PRESS:
            for (uint8_t d = 0; d < specials[c].topress.length(); d++) {
                if (pressed) {
                    Keyboard.pressScancode(specials[c].topress[d]);
                }
                else {
                    Keyboard.releaseScancode(specials[c].topress[d]);
                }
            }
            break;
        case SPECIAL_RUN:
            if (!pressed) {
                Keyboard.pressScancode(HID_KEY_ALT_RIGHT);
                Keyboard.pressScancode(HID_KEY_F3);
                Keyboard.sendReport();
                Keyboard.releaseScancode(HID_KEY_F3);
                Keyboard.releaseScancode(HID_KEY_ALT_RIGHT);
                Keyboard.sendReport();
                sleep_ms(150); // need to wait for the terminal to open
                Keyboard.type(specials[c].topress);
                Keyboard.type("\n");
            }
            break;
        case SPECIAL_SCROLL:
            doscroll = pressed;
            break;
        case SPECIAL_MACRO_RECORD:
            if (!pressed) { // released
                if (macrorecording == false) {
                    activemacro = specials[c].topress[0]-1; // stored variable starts at 0x01, need to subtract 1 for array index
                    macro_scancode[activemacro].clear();
                    macro_pressed[activemacro].clear();
                    macrorecording = true;
                }
                else {
                    macrorecording = false;
                }
            }
            break;
        case SPECIAL_MACRO_SELECT:
            if (!pressed) { // released
                if (activemacro != specials[c].topress[0]-1) {
                    macrorecording = false;
                }
                activemacro = specials[c].topress[0]-1;
            }
            break;
        case SPECIAL_MACRO: // play back macro
            if (!pressed) { // released
                if (macrorecording) {
                    macrorecording = false;
                }
                else {
                    for (uint8_t d = 0; d < macro_scancode[activemacro].size(); d++) {
                        if (macro_pressed[activemacro][d]) {
                            Keyboard.pressScancode(macro_scancode[activemacro][d]);
                        }
                        else {
                            Keyboard.releaseScancode(macro_scancode[activemacro][d]);
                        }
                        Keyboard.sendReport();
                        sleep_ms(2);
                    }
                }
            }
            break;
/*        case SPECIAL_TYPE_LOCKS:
            if (NUM_LOCK)
                Keyboard.type("Num-Lock LED On");
            if (CAPS_LOCK)
                Keyboard.type("Caps-Lock LED On");
            if (SCROLL_LOCK)
                Keyboard.type("Scroll-Lock LED On");
            break;*/
        case SPECIAL_BOOTLOADER:
            reset_usb_boot(0, 0);
            break;
        default:
            break;
    }
    return;
}

