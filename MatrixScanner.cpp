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
}

void MatrixScanner::getPinState(bool outpinstate[NUM_DOWN][NUM_ACROSS]) {
    mutex_enter_blocking(&mx1); // lock
    memcpy(outpinstate, pinstate, NUM_DOWN*NUM_ACROSS*sizeof(bool));
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
