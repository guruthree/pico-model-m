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

// Add functions to TinyKeyboard for directly using USB scancodes
// the work around might be to create a new modified header file and not include the original header file?

//#define private protected
#define TinyKeyboard_(void) TinyKeyboard_(void); friend class TinyKeyboardScancode
#include "TinyUSB_Mouse_and_Keyboard/TinyUSB_Mouse_and_Keyboard.h"

class TinyKeyboardScancode : public TinyKeyboard_ {
    public:
        // send a scan code
        size_t pressScancode(uint8_t k) {
            if (k >= HID_KEY_CONTROL_LEFT && k <= HID_KEY_GUI_RIGHT) {
                k -= HID_KEY_CONTROL_LEFT;
                _keyReport.modifiers |= (1 << k);
            }
            else {
                uint8_t i;
                // Add k to the key report only if it's not already present
                // and if there is an empty slot.
                if (_keyReport.keys[0] != k && _keyReport.keys[1] != k && 
                        _keyReport.keys[2] != k && _keyReport.keys[3] != k &&
                        _keyReport.keys[4] != k && _keyReport.keys[5] != k) {

                    for (i = 0; i < 6; i++) {
                        if (_keyReport.keys[i] == 0x00) {
                            _keyReport.keys[i] = k;
                            break;
                        }
                    }
                    if (i == 6) {
                    setWriteError();
                    return 0;
                    } 
                }
            }
            sendReport(&_keyReport);
            return 1;
        }

        // release a scan code
        size_t releaseScancode(uint8_t k) {
            if (k >= HID_KEY_CONTROL_LEFT && k <= HID_KEY_GUI_RIGHT) {
                k -= HID_KEY_CONTROL_LEFT;
                _keyReport.modifiers &= ~(1 << k);
            }
            else {
                uint8_t i;    
                // Test the key report to see if k is present.  Clear it if it exists.
                // Check all positions in case the key is present more than once (which it shouldn't be)
                for (i = 0; i < 6; i++) {
                    if (0 != k && _keyReport.keys[i] == k) {
                        _keyReport.keys[i] = 0x00;
                    }
                }
            }
            sendReport(&_keyReport);
            return 1;
        }
};


TinyKeyboardScancode Keyboard2;
