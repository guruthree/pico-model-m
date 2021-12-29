/*
 * USBKeyboard.h - USBKeyboard class, which provides an interface
 *                 to the Adafruit usb_hid object for sending keypresses and
 *                 keeping track of *lock LED status
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

#ifndef USBKeyboard_h
#define USBKeyboard_h

#include <string>
#include <vector>

#include "Adafruit_TinyUSB_Arduino/src/Adafruit_TinyUSB.h"

#define RID_KEYBOARD 1
#define RID_MOUSE 2

class USBKeyboard {
    private:
        std::vector<uint8_t> keys = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        uint8_t modifiers = 0;
        bool overflowing = false;
        const uint8_t overflow[6] = {0x01, 0x01, 0x01, 0x01, 0x01, 0x01};

        bool numLock = false;
        bool capsLock = false;
        bool scrollLock = false;

        void uk_hid_report_callback(uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize);
        friend void hid_report_callback(uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize);

    public:
        USBKeyboard();
        void begin();
        void pressScancode(uint8_t k);
        void releaseScancode(uint8_t k);
        void sendReport();
        void type(std::string line);

        bool getNumLock() { return numLock; };
        bool getCapsLock() { return capsLock; };
        bool getScrollLock() { return scrollLock; };
};

extern USBKeyboard Keyboard;

void hid_report_callback(uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize);

#endif
