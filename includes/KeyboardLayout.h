/*
 * keyboardlayout.h - Defines what keys are related to what pins, and also
 *                    defines special "magic" key-combos and macros.
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

#ifndef keyboardlayout_h
#define keyboardlayout_h

#include <string>
#include <vector>

// note, macro numbering in specialFunctionDefinition starts at 0x01 to avoid starting a string with 0x00
#define NUM_MACROS 3

// columns of the matrix
#define NUM_ACROSS 20
// rows of the matrix
#define NUM_DOWN 8

extern uint8_t across[NUM_ACROSS];
extern uint8_t down[NUM_DOWN];

extern uint8_t keyboardlayout[NUM_DOWN][NUM_ACROSS];

enum specialType {
    SPECIAL_TYPE,
    SPECIAL_PRESS,
    SPECIAL_MACRO,
    SPECIAL_MACRO_RECORD,
    SPECIAL_MACRO_SELECT,
    SPECIAL_RUN,
    SPECIAL_SCROLL,
    SPECIAL_BOOTLOADER,
    SPECIAL_REATTACH,
};

// struct to store special key functions
struct specialFunctionDefinition {
    uint8_t across;
    uint8_t down;
    uint8_t across2;
    uint8_t down2;
    bool twokey = false;
    specialType type;
    std::string topress = ""; // keys to press, or other optional arguments (like macro number)
    specialFunctionDefinition(uint8_t a, uint8_t d, specialType t) : across(a), down(d), type(t) {}
    specialFunctionDefinition(uint8_t a, uint8_t d, specialType t, std::string t2) : across(a), down(d), type(t), topress(t2) {}
    specialFunctionDefinition(uint8_t a, uint8_t d, uint8_t a2, uint8_t d2, specialType t) : across(a), down(d), across2(a2), down2(d2), type(t), twokey(true) {}
    specialFunctionDefinition(uint8_t a, uint8_t d, uint8_t a2, uint8_t d2, specialType t, std::string t2) : across(a), down(d), across2(a2), down2(d2), type(t), topress(t2), twokey(true) {}
};

extern std::vector<specialFunctionDefinition> specials;

#endif
