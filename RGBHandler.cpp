/*
 * RGB.cpp - handle all things to do with RGB/leds on the keyboard (such as
 *           the num and caps lock lights)
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

#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "ws2812.pio.h"

#include "USBKeyboard.h"
#include "RGBHandler.h"

#define WS2812_PIN 0
#define IS_RGBW false

RGBHandler::RGBHandler() {
}

void RGBHandler::begin() {
    uint offset = pio_add_program(LED_PIO, &ws2812_program);

    ws2812_program_init(LED_PIO, LED_SM, offset, WS2812_PIN, 800000, IS_RGBW);

    // Timer for udating the RGB nicely
    add_repeating_timer_ms(15, RGBloopTask, NULL, &RGBtimer);

    put_pixel(0);
}

void RGBHandler::updateTargetColor(float r, float g, float b) {
    targetColor[0] = r;
    targetColor[1] = g;
    targetColor[2] = b;
}

// This function is called by a timer to change the on-board LED to flash
// differently depending on USB state and change it nicely
bool RGBHandler::loopTask(repeating_timer_t *rt) {
    if (Keyboard.getNumLock() && !Keyboard.getCapsLock()) {
        setGreen();
    }
    else if (!Keyboard.getNumLock() && Keyboard.getCapsLock()) {
        setRed();
    }
    else if (Keyboard.getNumLock() && Keyboard.getCapsLock()) {
        setPurple();
    }
    else {
        setOrange();
    }

    for (uint8_t c; c < 3; c++) {
        currentColor[c] += (targetColor[c] - currentColor[c]) / 5;
    }
    put_pixel(urgb_u32(currentColor[1], currentColor[0], currentColor[2])); // g r b*/

    return true;
}

RGBHandler RGB;

bool RGBloopTask(repeating_timer_t *rt) {
    return RGB.loopTask(rt);
}