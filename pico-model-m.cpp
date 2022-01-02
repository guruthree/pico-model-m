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

// status of what's active on the matrix
bool pinstate[NUM_DOWN][NUM_ACROSS];
bool lastpinstate[NUM_DOWN][NUM_ACROSS]; // so we can detect a change

// these variables are needed for mouse scrolling
extern Adafruit_USBD_HID usb_hid; // for sending mouseReports
// press the center arrow cluster key to set
bool doscroll = false; // to true
// then press down an arrow key, mouse scrolls will be sent at 
// scroll delay (ms) intervals
#define SCROLL_DELAY 150
// to make sure we don't get stuck scrolling, we have a time out 
// in seconds to deactivate scrolling mode
uint64_t lastscroll = 0; // the last time we sent a mouse scroll, for the delay
#define SCROLL_TIMEOUT 30
// variable to store when the last time a key was pressed for the timeout
uint64_t lastpress = 0;

#include "pico-model-m.h"

int main() {
    bi_decl(bi_program_description("Firmware to scan an IBM Model M keyboard matrix and register as a USB Keyboard"));
    bi_decl(bi_program_version_string(VERSION));
    bi_decl(bi_program_build_date_string(BUILD_TIME));
    bi_decl(bi_program_url("https://github.com/guruthree/pico-model-m"));
    bi_decl(bi_1pin_with_name(WS2812_PIN, "ws2812 RGB LED"));
    bi_decl(bi_pin_mask_with_name(0xfffff << 1, "Matrix columns"));
    bi_decl(bi_pin_mask_with_name(0xff << 21, "Matrix rows"));
    bi_decl(bi_program_feature("USB HID, GPIO, RGB"))
    TinyUSBDevice.detach(); // don't do anything USB until we're ready

    // initialise RGB
    RGB.begin();
    RGB.setBlue();

    // initialise USB (it'll wait here until plugged in)
    Keyboard.begin();
//    sleep_ms(3000); // let USB settle (needed for my KVM?)

    // initialise variables for detecting key press
    for (uint8_t i = 0; i < NUM_ACROSS; i++) {
        for (uint8_t j = 0; j < NUM_DOWN; j++) {
            pinstate[j][i] = 0;
            lastpinstate[j][i] = 0;
        }
    }

    // initialise the keyboard matrix
    // this will launch the matrix scan onto the second core
    KeyMatrix.begin();

    // main loop
    while (1) {

        // if we secured the mutex lock on pinstate we can process changes
        if (KeyMatrix.getPinState(pinstate, lastpinstate)) {
            // check state against last state
            // if it's changed, print an update
            // copy state to last state
            for (uint8_t i = 0; i < NUM_ACROSS; i++) {
                for (uint8_t j = 0; j < NUM_DOWN; j++) {
                    if (pinstate[j][i] != lastpinstate[j][i]) { // the pin has changed, do something
                        lastpress = to_us_since_boot(get_absolute_time());
                        uint8_t scancode = keyboardlayout[j][i];
                        if (scancode == 0xFF) { // a special case key
                            handleSpecial(j, i, pinstate[j][i]);
                        }
                        else if (!doscroll) { // only handle regular keys if we're not scrolling
                            if (pinstate[j][i]) {
                                Keyboard.pressScancode(scancode);
                            }
                            else {
                                Keyboard.releaseScancode(scancode);
                            }
                        }
                        else {
                            // a scroll key was probably triggered
                            // reset scroll time delay so that scrolling will immediately trigger
                            lastscroll = lastpress - SCROLL_DELAY*1000;
                        }
                        if (macrorecording && !doscroll && scancode != 0xFF && scancode != HID_KEY_NONE) { // shouldn't ever hit none, but just to be safe...
                            macro_scancode[activemacro].push_back(scancode);
                            macro_pressed[activemacro].push_back(pinstate[j][i]);
                        }
                    }
                }
            }
            Keyboard.sendReport();
        }


        if (doscroll) { // intercept for scrolling
            uint64_t now = to_us_since_boot(get_absolute_time());
            if (now - lastscroll > SCROLL_DELAY*1000) { // only scroll every so often
                while( !usb_hid.ready() ) {
                    sleep_us(100);
                }
                if (pinstate[0][16] == true) {
                    usb_hid.mouseReport(RID_MOUSE,0,0,0,1,0); // scroll up
                }
                else if (pinstate[0][15] == true) {
                    usb_hid.mouseReport(RID_MOUSE,0,0,0,-1,0); // scroll down
                }
                if (pinstate[1][19] == true) {
                    usb_hid.mouseReport(RID_MOUSE,0,0,0,0,1); // scroll right
                }
                else if (pinstate[6][0] == true) {
                    usb_hid.mouseReport(RID_MOUSE,0,0,0,0,-1); // scroll left
                }
                lastscroll = now;
            }
            else if (now - lastpress > SCROLL_TIMEOUT*1e6) {
                // after X inactive seconds exit out of scroll mode
                doscroll = false;
            }
        }

        sleep_us(500); // I think the above code takes a couple milliseconds, so around a 200 hz refresh?
    }
}
