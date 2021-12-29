/*
 * MatrixScanner.cpp - scan a row coolumn matrix of GPIO pins and record its
 *                     state
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

#include <cstring>

#include "usb.h"

#include "pico/multicore.h"
#include "pico/sync.h"
#include "pico/time.h"
#include "hardware/gpio.h"

#include "MatrixScanner.h"
#include "KeyboardLayout.h"

// debounce delay (ms)
#define DEBOUNCE_DELAY 5

MatrixScanner::MatrixScanner() {
}

void MatrixScanner::begin() {
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


    multicore_launch_core1(core1_entry);
}

void MatrixScanner::scan() {
    uint64_t now;

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
}

void MatrixScanner::preventGhosting() {
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

    // due to unfortanate matrixing, ctrl alt shift combos can result in ghosting and therefore be ignored.
    // since in almost all cases pressing one mod key means the other one wouldn't do anything,
    // ignore the second one that was pressed
    if (pinstate[7][0] && pinstate[0][3]) { // l_alt, r_alt
        if (lastpinstate[7][0]) {
            pinstate[0][3] = false;
        }
        else if (lastpinstate[0][3]) {
            pinstate[7][0] = false;
        }
    }
    if (pinstate[7][3] && pinstate[6][3]) { // l_shift, r_shift
        if (lastpinstate[7][3]) {
            pinstate[6][3] = false;
        }
        else if (lastpinstate[6][3]) {
            pinstate[7][3] = false;
        }
    }
    if (pinstate[0][0] && pinstate[7][2]) { // l_ctrl, r_ctrl
        if (lastpinstate[0][0]) {
            pinstate[7][2] = false;
        }
        else if (lastpinstate[7][2]) {
            pinstate[0][0] = false;
        }
    }

    // caps lock and num lock are never part of a combo, ignore as part of a key combo for anti-ghosting?

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
}

void MatrixScanner::getPinState(bool outpinstate[NUM_DOWN][NUM_ACROSS], bool outlastpinstate[NUM_DOWN][NUM_ACROSS]) {
    mutex_enter_blocking(&mx1); // lock

    memcpy(outpinstate, pinstate, NUM_DOWN*NUM_ACROSS*sizeof(bool));
    memcpy(outlastpinstate, lastpinstate, NUM_DOWN*NUM_ACROSS*sizeof(bool));

    // the pin state has been fetched meaning changes have officially been registered
    // thus, the current pinstate is now the former pinstate
    memcpy(lastpinstate, pinstate, NUM_DOWN*NUM_ACROSS*sizeof(bool));

    mutex_exit(&mx1); // unlock
}

// set the pin to input so that it doesn't "drive the bus"
void MatrixScanner::setpininput(uint8_t pin) {
    gpio_set_dir(pin, GPIO_IN);
    gpio_pull_down(pin);
}

MatrixScanner KeyMatrix;

void core1_entry() {
    mutex_init(KeyMatrix.getMutex());

    while (1) {
        mutex_enter_blocking(KeyMatrix.getMutex()); // lock
        KeyMatrix.scan();
        KeyMatrix.preventGhosting();
        mutex_exit(KeyMatrix.getMutex()); // unlock
        sleep_us(100);
    }
}
