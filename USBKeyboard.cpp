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
    if (k >= HID_KEY_CONTROL_LEFT && k <= HID_KEY_GUI_RIGHT) {
        k -= HID_KEY_CONTROL_LEFT;
        modifiers |= (1 << k);
    }
    else {
        overflowing = false;
        uint8_t i;
        // Add k to the key report only if it's not already present
        // and if there is an empty slot.
        if (keys[0] != k && keys[1] != k && 
               keys[2] != k && keys[3] != k &&
                keys[4] != k && keys[5] != k) {

            for (i = 0; i < MAX_KEYS; i++) {
                if (keys[i] == 0x00) {
                    keys[i] = k;
                    break;
                }
            }
            if (i == MAX_KEYS) {
                // too many keys presseed, set overflowing
                overflowing = true;
            } 
        }
    }
}

void USBKeyboard::releaseScancode(uint8_t k) {
    if (k >= HID_KEY_CONTROL_LEFT && k <= HID_KEY_GUI_RIGHT) {
        k -= HID_KEY_CONTROL_LEFT;
        modifiers &= ~(1 << k);
    }
    else {
        uint8_t i;    
        // Test the key report to see if k is present.  Clear it if it exists.
        // Check all positions in case the key is present more than once (which it shouldn't be)
        for (i = 0; i < MAX_KEYS; i++) {
            if (0 != k && keys[i] == k) {
                keys[i] = 0x00;
            }
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

void USBKeyboard::type(std::string line) {
    for (int i = 0; i < line.length(); i++) {
        // convert to scan code
        uint8_t k = 0;
        pressScancode(k);
        sendReport();
        sleep_ms(2);
        releaseScancode(k);
        sendReport();
        sleep_ms(2);
    }
}

USBKeyboard Keyboard;
