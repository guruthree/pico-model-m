/*
 * pico-model-m.cpp - use an RP2040 chip directly connected to an IBM Model M
 *                    membrane to provide a USB HID Keyboard
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

// pico specific includes
#include "pico/time.h"
#include "pico/binary_info.h"
#include "pico/bootrom.h"
#include "hardware/gpio.h"

#include "KeyboardLayout.h"
#include "USBKeyboard.h"
#include "MatrixScanner.h"
#include "RGBHandler.h"


// Debounce delay (ms)
#define DEBOUNCE_DELAY 5

// set the pin to input so that it doesn't "drive the bus"
void setpininput(uint8_t pin) {
    gpio_set_dir(pin, GPIO_IN);
    gpio_pull_down(pin);
}

// status of what's active on the matrix
bool pinstate[NUM_DOWN][NUM_ACROSS];
bool lastpinstate[NUM_DOWN][NUM_ACROSS]; // so we can detect a change
uint64_t lastpinchangetime[NUM_DOWN][NUM_ACROSS]; // for debounce

// used for checking ghosting
std::vector<uint8_t> k1, k2;

// for scrolling
extern Adafruit_USBD_HID usb_hid;
// Scroll delay (ms), time between scroll events when key is held down
#define SCROLL_DELAY 180
bool doscroll = false; // enable scrolling mode with the arrow keys
uint64_t lastscroll = 0; // for repeat scrolling
// the last time a key was pressed to reset out of scroll mode incase it gets stuck
uint64_t lastpress = 0;
// time out time in seconds to deactivate scrolling mode in case we get stuck in it
#define SCROLL_TIMEOUT 30

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

int main() {
    bi_decl(bi_program_description("An IBM Model M Keyboard"));
    bi_decl(bi_program_feature("USB HID Device"));

    sleep_ms(10); // a pause to even out power usage

    // initialise RGB
    RGB.begin();
    RGB.setBlue();

    // Initialise USB (it'll wait here until plugged in)
    Keyboard.begin();

    // Initise GPIO pins
    // Send pins
    for (uint8_t i = 0; i < NUM_ACROSS; i++) {
        gpio_init(across[i]);
        setpininput(across[i]);
    }
    sleep_ms(2);
    // Read pins
    for (uint8_t i = 0; i < NUM_DOWN; i++) {
        gpio_init(down[i]);
        setpininput(down[i]);
    }
    sleep_ms(2);

    // Initialise variables for detecting key press
    for (uint8_t i = 0; i < NUM_ACROSS; i++) {
        for (uint8_t j = 0; j < NUM_DOWN; j++) {
            pinstate[j][i] = 0;
            lastpinstate[j][i] = 0;
            lastpinchangetime[j][i] = 0;
        }
    }
    sleep_ms(2);


    // Main loop
    uint64_t now;
    while (1) {

        // Loop through each send pin and then check each read pin
        for (uint8_t i = 0; i < NUM_ACROSS; i++) {

            gpio_set_dir(across[i], GPIO_OUT);
            gpio_put(across[i], 1);
            sleep_us(12); // delay for changes to GPIO to settle

            for (uint8_t j = 0; j < NUM_DOWN; j++) {
                now = to_us_since_boot(get_absolute_time());
                bool temp = gpio_get(down[j]);

                if (temp != pinstate[j][i] && (now - lastpinchangetime[j][i]) > DEBOUNCE_DELAY*1000) { // 5 ms deboune time
                    pinstate[j][i] = temp;
                    lastpinchangetime[j][i] = now;
                }
            }

            setpininput(across[i]); // so that the send pin floats and won't cause a bus conflict
            sleep_us(12); // delay for changes to GPIO to settle
        }

        // do ghost detection, if there's a ghosted key detected that's newly pressed, ignore it
        // if there is ghosting, but the ghosted key is an impossible key (HID_KEY_NONE) allow it

        // keys with no mapping should never be pressed
        for (uint8_t i = 0; i < NUM_ACROSS; i++) {
            for (uint8_t j = 0; j < NUM_DOWN; j++) {
                if (pinstate[j][i] && keyboardlayout[j][i] == HID_KEY_NONE) {
                    pinstate[j][i] = 0;
                }
            }
        }

        int numghosted;
        // check through each activated key to see if it's ghosting
        for (uint8_t i = 0; i < NUM_ACROSS; i++) {
            for (uint8_t j = 0; j < NUM_DOWN; j++) {
                if (pinstate[j][i]) {
                    k1.clear();
                    k2.clear();
                    // find other keys activated in the same row and column, to indicate where potential ghosting would be
                    for (uint8_t i2 = 0; i2 < NUM_ACROSS; i2++) {
                        if (pinstate[j][i2]) {
                            k1.push_back(i2);
                        }
                    }
                    for (uint8_t j2 = 0; j2 < NUM_DOWN; j2++) {
                        if (pinstate[j2][i]) {
                            k2.push_back(j2);
                        }
                    }
                    // check all the combinations of row by column, if all 4 are lit up, it's probably ghosting
                    numghosted = 0;
                    for (uint8_t i2 = 0; i2 < k1.size(); i2++) {
                        for (uint8_t j2 = 0; j2 < k2.size(); j2++) {
                            if (pinstate[k2[j2]][k1[i2]]) {
                                numghosted++;
                            }
                        }
                    }
                    if (numghosted > 3) { // probably ghosting
                        for (uint8_t i2 = 0; i2 < k1.size(); i2++) {
                            for (uint8_t j2 = 0; j2 < k2.size(); j2++) {
                                // if we're probably ghosting, don't recognize anything that's new & probably causing it
                                if (pinstate[k2[j2]][k1[i2]] != lastpinstate[k2[j2]][k1[i2]]) {
                                    pinstate[k2[j2]][k1[i2]] = 0;
                                }
                            }
                        }
                    }
                }
            }
        }

        // check state against last state
        // if it's changed, print an update
        // copy state to last state
        for (uint8_t i = 0; i < NUM_ACROSS; i++) {
            for (uint8_t j = 0; j < NUM_DOWN; j++) {
                if (pinstate[j][i] != lastpinstate[j][i]) { // the pin has changed, do something
                    lastpress = to_us_since_boot(get_absolute_time());
                    uint8_t scancode = keyboardlayout[j][i];
                    if (scancode == 0xFF) { // a special case key
//                        Keyboard.type("s!\n");
                        handleSpecial(j, i, pinstate[j][i]);
                    }
                    else if (!doscroll) { // only handle regular keys if we're not scrolling
                        if (pinstate[j][i])
                            Keyboard.pressScancode(scancode);
                        else
                            Keyboard.releaseScancode(scancode);
                    }
                    else {
                        // a scroll key was probably triggered
                        // reset scroll time delay so that scrolling will immediately trigger
                        lastscroll = lastpress - SCROLL_DELAY*1000;
                    }
                    lastpinstate[j][i] = pinstate[j][i];
                    if (macrorecording && !doscroll && scancode != 0xFF && scancode != HID_KEY_NONE) { // shouldn't ever hit none, but just to be safe...
                        macro_scancode[activemacro].push_back(scancode);
                        macro_pressed[activemacro].push_back(pinstate[j][i]);
                    }
                }
            }
        }


        if (doscroll) { // intercept for scrolling
            uint64_t now = to_us_since_boot(get_absolute_time());
            if (now - lastscroll > SCROLL_DELAY*1000) {
                if (pinstate[0][16] == true) {
                    usb_hid.mouseReport(2,0,0,0,1,0); // scroll up
                }
                else if (pinstate[0][15] == true) {
                    usb_hid.mouseReport(2,0,0,0,-1,0); // scroll down
                }
                if (pinstate[1][19] == true) {
                    usb_hid.mouseReport(2,0,0,0,0,1); // scroll right
                }
                else if (pinstate[6][0] == true) {
                    usb_hid.mouseReport(2,0,0,0,0,-1); // scroll left
                }
                lastscroll = now;
            }
            else if (now - lastpress > SCROLL_TIMEOUT*1e6) {
                // after 15 seconds exit out of scroll mode anyway
                doscroll = false;
            }
        }

        Keyboard.sendReport();
    }
}
