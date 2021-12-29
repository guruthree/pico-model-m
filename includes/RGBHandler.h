/*
 * RGB.h - handle all things to do with RGB/leds on the keyboard (such as
 *         the num and caps lock lights)
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

#ifndef RGBHandler_h
#define RGBHandler_h

#include "hardware/pio.h"

#define NUM_PIXELS 1

#define LED_PIO pio0
#define LED_SM 0

class RGBHandler {
    private:
        #if NUM_PIXELS != 1
            #error "Currently only coded up for 1 pixel"
        #endif
        float currentColor[3] = {0, 0, 0};
        float targetColor[3] = {0, 0, 0};

        void put_pixel(uint32_t pixel_grb) { pio_sm_put_blocking(LED_PIO, LED_SM, pixel_grb << 8u); };
        uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) { 
            return ((uint32_t) (r) << 8) | ((uint32_t) (g) << 16) | (uint32_t) (b);
        };

        struct repeating_timer RGBtimer;

        bool loopTask(repeating_timer_t *rt);
        friend bool RGBloopTask(repeating_timer_t *rt);


    public:
        RGBHandler();
        void begin();
        void updateTargetColor(float r, float g, float b);

        void setRed() { updateTargetColor(33, 0, 0); };
        void setGreen() { updateTargetColor(0, 33, 0); };
        void setBlue() { updateTargetColor(0, 0, 33); }
        void setOrange() { updateTargetColor(30, 10, 0); };
        void setPurple() { updateTargetColor(30, 0, 5); };
        void setYellow() { updateTargetColor(17, 20, 0); }
};

extern RGBHandler RGB;

bool RGBloopTask(repeating_timer_t *rt);

#endif
