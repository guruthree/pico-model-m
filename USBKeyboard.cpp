/*
 * USBKeyboard.cpp - USBKeyboard class, which provides an interface
 *                   to the Adafruit usb_hid object for sending keypresses and
 *                   keeping track of *lock LED status
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

#include <algorithm>

#include "usb.h"
//#include "Adafruit_USBD_CDC-stub.h"
#include "Adafruit_TinyUSB_Arduino/src/Adafruit_TinyUSB.h"
#include "USBKeyboard.h"

#define MAX_KEYS 6

// we're going to present to the computer as both a keyboard and mouse (the latter so we can scroll)
uint8_t const desc_hid_report[] =
{
    TUD_HID_REPORT_DESC_KEYBOARD( HID_REPORT_ID(RID_KEYBOARD) ),
    TUD_HID_REPORT_DESC_MOUSE( HID_REPORT_ID(RID_MOUSE) )
};

extern Adafruit_USBD_Device TinyUSBDevice;
Adafruit_USBD_HID usb_hid;

USBKeyboard::USBKeyboard() {
}

// function header to avoid a whole include that causes conflicts
void TinyUSB_Port_InitDevice(uint8_t rhport);

void USBKeyboard::begin() {
    // the following are equivalent to TinyUSBDevice.begin(), but don't require the link against CDC
    TinyUSBDevice.clearConfiguration();
    TinyUSB_Port_InitDevice(0);
    usb_hid.setPollInterval(2);
    usb_hid.setReportDescriptor(desc_hid_report, sizeof(desc_hid_report));
    usb_hid.setStringDescriptor("A Battleship that lives again");
    usb_hid.setReportCallback(NULL, hid_report_callback); // for status LEDs
//    usb_hid.setBootProtocol(true); // we implement 6KRO, (but we don't play nice and allow multiple modifiers) so we probably shouldn't claim we're using boot protocol
    usb_hid.begin();
    while ( !TinyUSBDevice.mounted() ) {
        sleep_us(1000);
    }
}

void USBKeyboard::pressScancode(uint8_t k) {
    if (k == HID_KEY_NONE) {
        return;
    }
    if (k >= HID_KEY_CONTROL_LEFT && k <= HID_KEY_GUI_RIGHT) { // modifier keys
        k -= HID_KEY_CONTROL_LEFT;
        modifiers |= (1 << k); // store modifers in the format they'll be sent
    }
    else { // regular keys
        // assume there is no overflowing
        overflowing = false;
        // put the pressed key at the start of keys
        keys.insert(keys.begin(), k);
        // this makes keys 7 long, if there is no overflowing then value at the end should be 0
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
    if (k >= HID_KEY_CONTROL_LEFT && k <= HID_KEY_GUI_RIGHT) { // modifier keys
        k -= HID_KEY_CONTROL_LEFT;
        modifiers &= ~(1 << k);
    }
    else { // regular keys
        // remove the key wherever it is from keys, which will shrink keys
        keys.erase(std::remove(keys.begin(), keys.end(), k), keys.end());

        if (keys.size() > MAX_KEYS) {
            // still overflowing :(
            overflowing = true;
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

// send a message to the computer about what keys are currently pressed
void USBKeyboard::sendReport() {
    if ( TinyUSBDevice.suspended() )  {
        TinyUSBDevice.remoteWakeup();
    }
    while( !usb_hid.ready() ) {
        sleep_us(100);
    }
    if (!overflowing) {
        usb_hid.keyboardReport(RID_KEYBOARD, modifiers, keys.data());
    }
    else {
        // if too many keys are pressed, we need to send the overflow code
        // https://wiki.osdev.org/USB_Human_Interface_Devices
        usb_hid.keyboardReport(RID_KEYBOARD, modifiers, (uint8_t*)overflow);
    }
    sleep_us(500);
}

// see tinyusb hid.h
uint8_t const conv_table[128][2] =  { HID_ASCII_TO_KEYCODE };

// write out text through the keyboard
void USBKeyboard::type(std::string line) {
    uint8_t k;

    for (int i = 0; i < line.length(); i++) {
        if (line[i] > 127) {
            // not valid ASCII
            continue;
        }

        // convert to scan code
        k = conv_table[line[i]][1];

        pressScancode(k);
        if (conv_table[line[i]][0]) // shift
            pressScancode(HID_KEY_SHIFT_LEFT);
        sendReport();

        releaseScancode(k);
        if (conv_table[line[i]][0]) // shift
            releaseScancode(HID_KEY_SHIFT_LEFT);
        sendReport();
    }
}

// output report callback for LED indicator such as cap lock (from hid_keyboard.ino)
// this gets called by tinyusb when the keyboard is told one of the lock states has changed
void USBKeyboard::uk_hid_report_callback(uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize)
{
    (void) report_id;
    (void) bufsize;
    // LED indicator is output report with only 1 byte length
    if ( report_type != HID_REPORT_TYPE_OUTPUT ) return;

    // The LED bit map is as follows: (also defined by KEYBOARD_LED_* )
    // Kana (4) | Compose (3) | ScrollLock (2) | CapsLock (1) | Numlock (0)
    uint8_t ledIndicator = buffer[0];

    // turn on LED if caplock is set
    numLock = ledIndicator & KEYBOARD_LED_NUMLOCK;
    capsLock = ledIndicator & KEYBOARD_LED_CAPSLOCK;
    scrollLock = ledIndicator & KEYBOARD_LED_SCROLLLOCK;
}

USBKeyboard Keyboard;

// callback wrapper so that it has the class
void hid_report_callback(uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {
    Keyboard.uk_hid_report_callback(report_id, report_type, buffer, bufsize);
}
