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

#ifndef MatrixScanner_h
#define MatrixScanner_h

#include "pico/multicore.h"

#include "KeyboardLayout.h"

class MatrixScanner {
    private:
        bool pinstate[NUM_DOWN][NUM_ACROSS];
        bool lastpinstate[NUM_DOWN][NUM_ACROSS]; // so we can detect a change
        uint64_t lastpinchangetime[NUM_DOWN][NUM_ACROSS]; // for debounce

        // used for checking ghosting
        std::vector<uint8_t> k1, k2;

        mutex_t mx1;

        void setpininput(uint8_t pin);

    public:
        MatrixScanner();
        void begin();
        void scan();
        void preventGhosting();
        void getPinState(bool outpinstate[NUM_DOWN][NUM_ACROSS], bool outlastpinstate[NUM_DOWN][NUM_ACROSS]);
        
        mutex_t* getMutex() { return &mx1; };
};

extern MatrixScanner KeyMatrix;

void core1_entry();

#endif
