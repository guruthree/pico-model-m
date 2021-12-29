/*
 * keyboardlayout.cpp - Defines what keys are related to what pins, and also
 *                      defines special "magic" key-combos and macros.
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

#include "usb.h"

#include "KeyboardLayout.h"

// GPIO pins for the membrane
uint8_t across[NUM_ACROSS] = {20, 19, 18, 17, 13, 14, 15, 16, 12, 10, 11, 9, 8, 6, 4, 2, 1, 3, 5, 7};
uint8_t down[NUM_DOWN] = {28, 26, 27, 21, 23, 25, 22, 24};

// https://deskthority.net/wiki/Scancode

// 0xFF -> special function to be handed differently
uint8_t keyboardlayout[NUM_DOWN][NUM_ACROSS] = {
//                          0 (pin 1)             1 (pin 2)              2 (pin 3)            3 (pin 4)          4 (pin 5)     5 (pin 6)     6 (pin 7)  7 (pin 8)  8 (pin 9)      9 (pin 10)   10 (pin 11)  11 (pin 12)             12 (pin 13)      13 (pin 14)           14 (pin 15)         15 (pin 16)        16 (pin 17)            17 (pin 18)              18 (pin 19)           19 (pin 20)
/*0 (pin 1)*/{   HID_KEY_CONTROL_LEFT,  HID_KEY_APPLICATION,      HID_KEY_GUI_LEFT,   HID_KEY_ALT_RIGHT,      HID_KEY_NONE, HID_KEY_NONE, HID_KEY_NONE, HID_KEY_G, HID_KEY_H, HID_KEY_ESCAPE,   HID_KEY_F1,  HID_KEY_F2,          HID_KEY_NONE,     HID_KEY_NONE,   HID_KEY_APOSTROPHE, HID_KEY_ARROW_DOWN,  HID_KEY_ARROW_UP,          HID_KEY_NONE,            HID_KEY_NONE,                 0xFF},
/*1 (pin 2)*/{                   0xFF, HID_KEY_PRINT_SCREEN,                  0xFF,        HID_KEY_NONE,         HID_KEY_A,    HID_KEY_S,    HID_KEY_D, HID_KEY_F, HID_KEY_J,           0xFF,         0xFF,  HID_KEY_F3,             HID_KEY_K,        HID_KEY_L,    HID_KEY_SEMICOLON,   HID_KEY_KEYPAD_4,    HID_KEY_DELETE,      HID_KEY_KEYPAD_5,        HID_KEY_KEYPAD_6,  HID_KEY_ARROW_RIGHT},
/*2 (pin 3)*/{HID_KEY_KEYPAD_SUBTRACT,        HID_KEY_PAUSE,          HID_KEY_NONE,        HID_KEY_NONE,         HID_KEY_1,    HID_KEY_2,    HID_KEY_3, HID_KEY_4, HID_KEY_7,           0xFF,   HID_KEY_F4,  HID_KEY_F5,             HID_KEY_8,        HID_KEY_9,            HID_KEY_0,   HID_KEY_NUM_LOCK,   HID_KEY_PAGE_UP, HID_KEY_KEYPAD_DIVIDE, HID_KEY_KEYPAD_MULTIPLY,         HID_KEY_NONE},
/*3 (pin 4)*/{           HID_KEY_NONE,       HID_KEY_ESCAPE,                  0xFF,        HID_KEY_NONE,     HID_KEY_GRAVE, HID_KEY_NONE, HID_KEY_NONE, HID_KEY_5, HID_KEY_6,           0xFF,         0xFF,  HID_KEY_F6,         HID_KEY_EQUAL,     HID_KEY_NONE,        HID_KEY_MINUS,  HID_KEY_BACKSPACE,    HID_KEY_INSERT,          HID_KEY_HOME,            HID_KEY_NONE,         HID_KEY_NONE},
/*4 (pin 5)*/{     HID_KEY_KEYPAD_ADD,                 0xFF,           HID_KEY_TAB,        HID_KEY_NONE,         HID_KEY_Q,    HID_KEY_W,    HID_KEY_E, HID_KEY_R, HID_KEY_U,           0xFF,   HID_KEY_F7,  HID_KEY_F8,             HID_KEY_I,        HID_KEY_O,            HID_KEY_P,   HID_KEY_KEYPAD_7, HID_KEY_PAGE_DOWN,      HID_KEY_KEYPAD_8,        HID_KEY_KEYPAD_9,         HID_KEY_NONE},
/*5 (pin 6)*/{           HID_KEY_NONE,  HID_KEY_SCROLL_LOCK,                  0xFF,        HID_KEY_NONE,      HID_KEY_NONE, HID_KEY_NONE, HID_KEY_NONE, HID_KEY_T, HID_KEY_Y,           0xFF,         0xFF,  HID_KEY_F9, HID_KEY_BRACKET_RIGHT,     HID_KEY_NONE, HID_KEY_BRACKET_LEFT,       HID_KEY_NONE,       HID_KEY_END,          HID_KEY_NONE,            HID_KEY_NONE,         HID_KEY_NONE},
/*6 (pin 7)*/{     HID_KEY_ARROW_LEFT,     HID_KEY_KEYPAD_1,     HID_KEY_CAPS_LOCK, HID_KEY_SHIFT_RIGHT,         HID_KEY_Z,    HID_KEY_X,    HID_KEY_C, HID_KEY_V, HID_KEY_M,           0xFF,  HID_KEY_F10, HID_KEY_F11,         HID_KEY_COMMA,   HID_KEY_PERIOD,     HID_KEY_EUROPE_1,      HID_KEY_ENTER,      HID_KEY_NONE,      HID_KEY_KEYPAD_2,        HID_KEY_KEYPAD_3,         HID_KEY_NONE},
/*7 (pin 8)*/{       HID_KEY_ALT_LEFT,        HID_KEY_SPACE, HID_KEY_CONTROL_RIGHT,  HID_KEY_SHIFT_LEFT, HID_KEY_BACKSLASH, HID_KEY_NONE, HID_KEY_NONE, HID_KEY_B, HID_KEY_N,           0xFF,         0xFF, HID_KEY_F12,          HID_KEY_NONE,     HID_KEY_NONE,        HID_KEY_SLASH,       HID_KEY_NONE,      HID_KEY_NONE,      HID_KEY_KEYPAD_0,  HID_KEY_KEYPAD_DECIMAL, HID_KEY_KEYPAD_ENTER}
};


// magic1: 9, 1 - magic2: 10, 1 - magic3: 9, 2
// magic4: 9, 3 - magic5: 10, 3 - magic6: 9, 4 
// magic7: 9, 5 - magic8: 10, 5 - magic9: 9, 6 - magic10: 9, 7

std::vector<specialFunctionDefinition> specials = {
    specialFunctionDefinition(0, 1, SPECIAL_TYPE, "^"), // keypad carrot
    specialFunctionDefinition(1, 4, SPECIAL_PRESS, {HID_KEY_CONTROL_LEFT, HID_KEY_X, 0x00}), // cut
    specialFunctionDefinition(2, 5, SPECIAL_PRESS, {HID_KEY_CONTROL_LEFT, HID_KEY_C, 0x00}), // copy
    specialFunctionDefinition(2, 1, SPECIAL_PRESS, {HID_KEY_CONTROL_LEFT, HID_KEY_V, 0x00}), // paste
    specialFunctionDefinition(10, 7, SPECIAL_RUN, "octave"), // octave
    specialFunctionDefinition(19, 0, SPECIAL_SCROLL), // scroll mode

    specialFunctionDefinition(9, 1, 9, 3, SPECIAL_TYPE, "else"), // magic1
    specialFunctionDefinition(9, 1, 10, 3, SPECIAL_TYPE, "if"), // magic1
    specialFunctionDefinition(9, 1, 9, 4, SPECIAL_TYPE, "for"), // magic1
    specialFunctionDefinition(9, 1, 9, 5, SPECIAL_TYPE, "end"), // magic1
    specialFunctionDefinition(9, 1, 10, 5, SPECIAL_TYPE, "break"), // magic1
    specialFunctionDefinition(9, 1, 9, 6, SPECIAL_TYPE, "continue"), // magic1
    specialFunctionDefinition(9, 1, 9, 7, SPECIAL_TYPE, "while"), // magic1

    specialFunctionDefinition(10, 1, 9, 3, SPECIAL_TYPE, "()"), // magic2
    specialFunctionDefinition(10, 1, 10, 3, SPECIAL_TYPE, "[]"), // magic2
    specialFunctionDefinition(10, 1, 9, 4, SPECIAL_TYPE, "{}"), // magic2
    specialFunctionDefinition(10, 1, 9, 5, SPECIAL_TYPE, "true"), // magic2
    specialFunctionDefinition(10, 1, 10, 5, SPECIAL_TYPE, "false"), // magic2
    specialFunctionDefinition(10, 1, 9, 6, SPECIAL_TYPE, "nan"), // magic2
    specialFunctionDefinition(10, 1, 9, 7, SPECIAL_TYPE, "function"), // magic2

    specialFunctionDefinition(9, 2, 9, 3, SPECIAL_TYPE, "sqrt"), // magic3
    specialFunctionDefinition(9, 2, 10, 3, SPECIAL_TYPE, "round"), // magic3
    specialFunctionDefinition(9, 2, 9, 4, SPECIAL_TYPE, "pow"), // magic3
    specialFunctionDefinition(9, 2, 9, 5, SPECIAL_TYPE, "sin"), // magic3
    specialFunctionDefinition(9, 2, 10, 5, SPECIAL_TYPE, "cos"), // magic3
    specialFunctionDefinition(9, 2, 9, 6, SPECIAL_TYPE, "tan"), // magic3
//    specialFunctionDefinition(9, 2, 9, 7, SPECIAL_TYPE, "function"), // magic3

    specialFunctionDefinition(9, 1, 2, 3, SPECIAL_MACRO_RECORD, {0x01, 0x00}), // ctrl again - record macro 0x01
    specialFunctionDefinition(10, 1, 14, 2, SPECIAL_BOOTLOADER, {15, 1, 17, 7, 0x00}), // 20 (numpad) 40
    specialFunctionDefinition(10, 1, 2, 3, SPECIAL_MACRO_RECORD, {0x02, 0x00}), // ctrl again - record macro 0x02
    specialFunctionDefinition(9, 2, 2, 3, SPECIAL_MACRO_RECORD, {0x03, 0x00}), // ctrl again - record macro 0x03
    specialFunctionDefinition(2, 3, SPECIAL_MACRO, {0x01, 0x00}), // again - stop record, playback last selected macro
    specialFunctionDefinition(9, 1, SPECIAL_MACRO_SELECT, {0x01, 0x00}), // ctrl again - record 0x01
    specialFunctionDefinition(10, 1, SPECIAL_MACRO_SELECT, {0x02, 0x00}), // ctrl again - record 0x02
    specialFunctionDefinition(9, 2, SPECIAL_MACRO_SELECT, {0x03, 0x00}), // ctrl again - record 0x03
};
