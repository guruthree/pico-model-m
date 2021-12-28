/*
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

#include <algorithm>

#include "usb.h"
#include "Adafruit_USBD_CDC-stub.h"
#include "Adafruit_TinyUSB_Arduino/src/Adafruit_TinyUSB.h"
#include "USBKeyboard.h"

#define RID_KEYBOARD 1
#define RID_MOUSE 2

#define MAX_KEYS 6

uint8_t const desc_hid_report[] =
{
    TUD_HID_REPORT_DESC_KEYBOARD( HID_REPORT_ID(RID_KEYBOARD) ),
    TUD_HID_REPORT_DESC_MOUSE( HID_REPORT_ID(RID_MOUSE) )
};

Adafruit_USBD_HID usb_hid;

USBKeyboard::USBKeyboard() {
}

void USBKeyboard::begin() {
    TinyUSBDevice.begin();
    usb_hid.setStringDescriptor("A Battleship that lives again");
    usb_hid.setReportDescriptor(desc_hid_report, sizeof(desc_hid_report));
//    usb_hid.setReportCallback(NULL, hid_report_callback); // for status LEDs
//    usb_hid.setBootProtocol(true); // shouldn't be needed?
    usb_hid.begin();
    while (!TinyUSBDevice.mounted()) {
        sleep_ms(1);
    }
}

void USBKeyboard::pressScancode(uint8_t k) {
    if (k == HID_KEY_NONE) {
        return;
    }
    if (k >= HID_KEY_CONTROL_LEFT && k <= HID_KEY_GUI_RIGHT) {
        k -= HID_KEY_CONTROL_LEFT;
        modifiers |= (1 << k);
    }
    else {
        // assume all is well
        overflowing = false;
        // put the pressed key at the start of keys
        keys.insert(keys.begin(), k);
        // this makes keys 7 long, if all is well the value in that position is 0
        if (keys[MAX_KEYS] == HID_KEY_NONE) {
            // in which case remove that value so that keys is the correct length
            while (keys.size() > MAX_KEYS) keys.pop_back();
        }
        else {
            // if the key at the end isn't 0, we're overflowing and should report that
            // we also shouldn't change keys anymore. releasing a key will remove it and
            // and keys should shrink
            overflowing = true;
        }
    }
}

void USBKeyboard::releaseScancode(uint8_t k) {
    if (k == HID_KEY_NONE) {
        return;
    }
    if (k >= HID_KEY_CONTROL_LEFT && k <= HID_KEY_GUI_RIGHT) {
        k -= HID_KEY_CONTROL_LEFT;
        modifiers &= ~(1 << k);
    }
    else {
        // remove the key wherever it is from keys, which will shrink keys
        keys.erase(std::remove(keys.begin(), keys.end(), k), keys.end());

        if (keys.size() > MAX_KEYS) {
            // still overflowing :(
            overflowing = false;
        }
        else {
            // add back onto the end of keys to ensure it's the proper size
            while (keys.size() < MAX_KEYS) keys.push_back(HID_KEY_NONE);

            // yay we've released enough keys to not be overflowing,
            // we can continue with normal operation
            overflowing = false;
        }
    }
}

void USBKeyboard::sendReport() {
    if ( TinyUSBDevice.suspended() )  {
        TinyUSBDevice.remoteWakeup();
    }
    while( !usb_hid.ready() ) {
        sleep_us(10);
    }
    if (!overflowing) {
        usb_hid.keyboardReport(RID_KEYBOARD, modifiers, keys.data());
    }
    else {
        // if too many keys are pressed, we need to send the overflow code
        // https://wiki.osdev.org/USB_Human_Interface_Devices
        usb_hid.keyboardReport(RID_KEYBOARD, modifiers, (uint8_t*)overflow);
    }
}

uint8_t const conv_table[128][2] =  { HID_ASCII_TO_KEYCODE };

void USBKeyboard::type(std::string line) {
    uint8_t k;

    for (int i = 0; i < line.length(); i++) {
        if (line[i] > 127) {
            // not valid ASCII
            continue;
        }

        // convert to scan code
        k = conv_table[line[i]][0];

        pressScancode(k);
        if (conv_table[line[i]][1]) // shift
            pressScancode(HID_KEY_SHIFT_LEFT);
        sendReport();
        sleep_ms(2);

        releaseScancode(k);
        if (conv_table[line[i]][1]) // shift
            releaseScancode(HID_KEY_SHIFT_LEFT);
        sendReport();
        sleep_ms(2);
    }
}

USBKeyboard Keyboard;
