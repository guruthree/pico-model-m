/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 guruthree
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
#include "hardware/gpio.h"
#include "pico/binary_info.h"
#include "pico/bootrom.h"

#include "ws2812.h"

#include "includes/usb.h"
#include "includes/Adafruit_USBD_CDC-stub.h"
#include "Adafruit_TinyUSB_Arduino/src/Adafruit_TinyUSB.h"
#include "includes/TinyKeyboardScancode.h"

// Adafruit TinyUSB instance
extern Adafruit_USBD_Device USBDevice;
extern Adafruit_USBD_HID usb_hid;

// Debounce delay (ms)
#define DEBOUNCE_DELAY 5

float currentColor[3] = {0, 0, 0};
float targetColor[3] = {0, 0, 0};

#define RED updateTargetColor(33, 0, 0)
#define GREEN updateTargetColor(0, 33, 0)
#define BLUE updateTargetColor(0, 0, 33)
#define ORANGE updateTargetColor(33, 10, 0)
#define PURPLE updateTargetColor(30, 0, 5)

void updateTargetColor(float r, float g, float b) {
    targetColor[0] = r;
    targetColor[1] = g;
    targetColor[2] = b;
}

// This function is called by a timer to change the on-board LED to flash
// differently depending on USB state
static bool loopTask(repeating_timer_t *rt){
    for (uint8_t c; c < 3; c++) {
        currentColor[c] += (targetColor[c] - currentColor[c]) / 5;
//        if (currentColor[c] < 5) { // this ensures black
//            currentColor[c] = 0;
//        }
    }
    put_pixel(urgb_u32(currentColor[1], currentColor[0], currentColor[2])); // g r b
    return true;
}

// set the pin to input so that it doesn't "drive the bus"
void setpininput(uint8_t pin) {
    gpio_set_dir(pin, GPIO_IN);
    gpio_pull_down(pin);
}

#define NUM_ACROSS 20
uint8_t across[NUM_ACROSS] = {20, 19, 18, 17, 13, 14, 15, 16, 12, 10, 11, 9, 8, 6, 4, 2, 1, 3, 5, 7};
#define NUM_DOWN 8
uint8_t down[NUM_DOWN] = {28, 26, 27, 21, 23, 25, 22, 24};

// the keyboard layout in a 2D array
#include "includes/keyboardlayout.h"

// status of what's active on the matrix
bool pinstate[NUM_DOWN][NUM_ACROSS];
bool lastpinstate[NUM_DOWN][NUM_ACROSS]; // so we can detect a change
uint64_t lastpinchangetime[NUM_DOWN][NUM_ACROSS]; // for debounce

// Scroll delay (ms), time between scroll events when key is held down
#define SCROLL_DELAY 180
bool doscroll = false; // enable scrolling mode with the arrow keys
uint64_t lastscroll = 0; // for repeat scrolling

// the last time a key was pressed to reset out of scroll mode incase it gets stuck
uint64_t lastpress = 0;
// time out time in seconds to deactivate scrolling mode in case we get stuck in it
#define SCROLL_TIMEOUT 30

// used for checking ghosting
std::vector<uint8_t> k1, k2;

bool macrorecording = false;
// record the macro here
std::vector< std::vector<uint8_t> > macro_scancode(3, std::vector<uint8_t>(0));
std::vector< std::vector<uint8_t> > macro_pressed(3, std::vector<uint8_t>(0));;
uint8_t activemacro = 0;

bool NUM_LOCK = false;
bool CAPS_LOCK = false;
bool SCROLL_LOCK = false;

void handleSpecial(uint8_t down, uint8_t across, bool pressed) { // pressed or released
    bool doprocess = false;
    uint8_t c;
    for (c = 0; c < specials.size(); c++) {
        if (!specials[c].twokey) {
            if (specials[c].down == down && specials[c].across == across) {
                doprocess = true;
                break;
            }
        }
        else { // need to check for two keys being pressed
//            if (pinstate[specials[c].down][specials[c].across] && pinstate[specials[c].down2][specials[c].across2]) {
            if (pinstate[specials[c].down][specials[c].across] && specials[c].down2 == down && specials[c].across2 == across) { // activate on the second key
                doprocess = true;
                break;
            }
        }
    }
    if (!doprocess) {
        return;
    }
//    Keyboard2.println("do process");

    switch (specials[c].type) {
        case SPECIAL_TYPE:
            if (pressed) {
                Keyboard2.print(specials[c].topress);
            }
            break;
        case SPECIAL_PRESS:
            for (uint8_t d = 0; d < specials[c].topress.length(); d++) {
                if (pressed) {
                    Keyboard2.pressScancode(specials[c].topress[d]);
                }
                else {
                    Keyboard2.releaseScancode(specials[c].topress[d]);
                }
            }
            break;
        case SPECIAL_RUN:
            if (!pressed) {
                Keyboard2.pressScancode(HID_KEY_ALT_RIGHT);
                Keyboard2.pressScancode(HID_KEY_F3);
                Keyboard2.releaseScancode(HID_KEY_F3);
                Keyboard2.releaseScancode(HID_KEY_ALT_RIGHT);
                sleep_ms(150); // need to wait for the terminal to open
                Keyboard2.println(specials[c].topress);
            }
            break;
        case SPECIAL_SCROLL:
            doscroll = pressed;
//            Keyboard2.print("sm:");
//            Keyboard2.println(doscroll);
            break;
        case SPECIAL_MACRO_RECORD:
//            Keyboard2.println("record?");
            if (!pressed) { // released
                if (macrorecording == false) {
                    activemacro = specials[c].topress[0]-1; // stored variable starts at 0x01, need to subtract 1 for array index
                    macro_scancode[activemacro].clear();
                    macro_pressed[activemacro].clear();
                    macrorecording = true;
//                    Keyboard2.println("recording");
                }
                else {
                    macrorecording = false;
//                    Keyboard2.println("stopped recording1");
                }
            }
            break;
        case SPECIAL_MACRO_SELECT:
            if (!pressed) { // released
                if (activemacro != specials[c].topress[0]-1) {
                    macrorecording = false;
                }
                activemacro = specials[c].topress[0]-1;
//                Keyboard2.print("selected macro ");
//                Keyboard2.println(activemacro);
            }
            break;
        case SPECIAL_MACRO: // play back macro
//            Keyboard2.println("stopped recording2");
            if (!pressed) { // released
                if (macrorecording) {
                    macrorecording = false;
                }
                else {
//                    Keyboard2.print("playing back ");
//                    Keyboard2.println(macro_scancode[activemacro].size());
                    for (uint8_t d = 0; d < macro_scancode[activemacro].size(); d++) {
//                        char tempaa[50];
//                        sprintf(tempaa, "%x: %i", macro_scancode[activemacro][d], macro_pressed[activemacro][d]); 
//                        Keyboard2.println(tempaa);
                        if (macro_pressed[activemacro][d]) {
                            Keyboard2.pressScancode(macro_scancode[activemacro][d]);
                        }
                        else {
                            Keyboard2.releaseScancode(macro_scancode[activemacro][d]);
                        }
//                        sleep_ms(1);
                    }
                }
            }
            break;
/*        case SPECIAL_TYPE_LOCKS:
            if (NUM_LOCK)
                Keyboard2.println("Num-Lock LED On");
            if (CAPS_LOCK)
                Keyboard2.println("Caps-Lock LED On");
            if (SCROLL_LOCK)
                Keyboard2.println("Scroll-Lock LED On");
            break;*/
        case SPECIAL_BOOTLOADER:
            reset_usb_boot(0, 0);
            break;
        default:
            break;
    }
    return;
}

// Output report callback for LED indicator such as Caplocks (from hid_keyboard.ino)
void hid_report_callback(uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize)
{
    (void) report_id;
    (void) bufsize;
    // LED indicator is output report with only 1 byte length
    if ( report_type != HID_REPORT_TYPE_OUTPUT ) return;

    // The LED bit map is as follows: (also defined by KEYBOARD_LED_* )
    // Kana (4) | Compose (3) | ScrollLock (2) | CapsLock (1) | Numlock (0)
    uint8_t ledIndicator = buffer[0];

    // turn on LED if caplock is set
    NUM_LOCK = ledIndicator & KEYBOARD_LED_NUMLOCK;
    CAPS_LOCK = ledIndicator & KEYBOARD_LED_CAPSLOCK;
    SCROLL_LOCK = ledIndicator & KEYBOARD_LED_SCROLLLOCK;

    if (NUM_LOCK && !CAPS_LOCK) {
        GREEN;
    }
    else if (!NUM_LOCK && CAPS_LOCK) {
        RED;
    }
    else if (NUM_LOCK && CAPS_LOCK) {
        PURPLE;
    }
    else {
        ORANGE;
//        BLUE;
    }
}


int main() {
    bi_decl(bi_program_description("An IBM Model M Keyboard"));
    bi_decl(bi_program_feature("USB HID Device"));

    sleep_ms(10); // some sleeps to even out power usage

    ws2812_init();
    put_pixel(0);

    // Timer for regularly processing USB events
    struct repeating_timer timer;
    add_repeating_timer_ms(15, loopTask, NULL, &timer);

//    usb_hid.setBootProtocol(true); // shouldn't be needed?
//    sleep_ms(2);
    usb_hid.setStringDescriptor("A Battleship that lives again");
    USBDevice.begin(); // Initialise Adafruit TinyUSB
    sleep_ms(2);
    usb_hid.setReportCallback(NULL, hid_report_callback); // for status LEDs
    sleep_ms(2);

    // Initialise a keyboard (code will wait here to be plugged in)
    Keyboard2.begin();
    sleep_ms(2);
    BLUE;

    // Initise GPIO pins
    // Send pins
    for (uint8_t i = 0; i < NUM_ACROSS; i++) {
        gpio_init(across[i]);
        setpininput(across[i]);
    }
    sleep_ms(2);
    // Read pins
    for (uint8_t i = 0; i < NUM_DOWN; i++) {
        gpio_init(down[i]);
        setpininput(down[i]);
    }
    sleep_ms(2);

    // Initialise variables for detecting key press
    for (uint8_t i = 0; i < NUM_ACROSS; i++) {
        for (uint8_t j = 0; j < NUM_DOWN; j++) {
            pinstate[j][i] = 0;
            lastpinstate[j][i] = 0;
            lastpinchangetime[j][i] = 0;
        }
    }
    sleep_ms(2);


    // Main loop
    uint64_t now;
    while (1) {

        // Loop through each send pin and then check each read pin
        for (uint8_t i = 0; i < NUM_ACROSS; i++) {

            gpio_set_dir(across[i], GPIO_OUT);
            gpio_put(across[i], 1);
            sleep_us(12); // delay for changes to GPIO to settle

            for (uint8_t j = 0; j < NUM_DOWN; j++) {
                now = to_us_since_boot(get_absolute_time());
                bool temp = gpio_get(down[j]);

                if (temp != pinstate[j][i] && (now - lastpinchangetime[j][i]) > DEBOUNCE_DELAY*1000) { // 5 ms deboune time
                    pinstate[j][i] = temp;
                    lastpinchangetime[j][i] = now;
                }
            }

            setpininput(across[i]); // so that the send pin floats and won't cause a bus conflict
            sleep_us(12); // delay for changes to GPIO to settle
        }

        // do ghost detection, if there's a ghosted key detected that's newly pressed, ignore it
        // if there is ghosting, but the ghosted key is an impossible key (HID_KEY_NONE) allow it

        // keys with no mapping should never be pressed
        for (uint8_t i = 0; i < NUM_ACROSS; i++) {
            for (uint8_t j = 0; j < NUM_DOWN; j++) {
                if (pinstate[j][i] && keyboardlayout[j][i] == HID_KEY_NONE) {
                    pinstate[j][i] = 0;
                }
            }
        }

        int numghosted;
        // check through each activated key to see if it's ghosting
        for (uint8_t i = 0; i < NUM_ACROSS; i++) {
            for (uint8_t j = 0; j < NUM_DOWN; j++) {
                if (pinstate[j][i]) {
                    k1.clear();
                    k2.clear();
                    // find other keys activated in the same row and column, to indicate where potential ghosting would be
                    for (uint8_t i2 = 0; i2 < NUM_ACROSS; i2++) {
                        if (pinstate[j][i2]) {
                            k1.push_back(i2);
                        }
                    }
                    for (uint8_t j2 = 0; j2 < NUM_DOWN; j2++) {
                        if (pinstate[j2][i]) {
                            k2.push_back(j2);
                        }
                    }
                    // check all the combinations of row by column, if all 4 are lit up, it's probably ghosting
                    numghosted = 0;
                    for (uint8_t i2 = 0; i2 < k1.size(); i2++) {
                        for (uint8_t j2 = 0; j2 < k2.size(); j2++) {
                            if (pinstate[k2[j2]][k1[i2]]) {
                                numghosted++;
                            }
                        }
                    }
                    if (numghosted > 3) { // probably ghosting
                        for (uint8_t i2 = 0; i2 < k1.size(); i2++) {
                            for (uint8_t j2 = 0; j2 < k2.size(); j2++) {
                                // if we're probably ghosting, don't recognize anything that's new & probably causing it
                                if (pinstate[k2[j2]][k1[i2]] != lastpinstate[k2[j2]][k1[i2]]) {
                                    pinstate[k2[j2]][k1[i2]] = 0;
                                }
                            }
                        }
                    }
                }
            }
        }

        // check state against last state
        // if it's changed, print an update
        // copy state to last state
        for (uint8_t i = 0; i < NUM_ACROSS; i++) {
            for (uint8_t j = 0; j < NUM_DOWN; j++) {
                if (pinstate[j][i] != lastpinstate[j][i]) { // the pin has changed, do something
                    lastpress = to_us_since_boot(get_absolute_time());
                    uint8_t scancode = keyboardlayout[j][i];
                    if (scancode == 0xFF) { // a special case key
//                        Keyboard2.println("s!");
                        handleSpecial(j, i, pinstate[j][i]);
                    }
                    else if (!doscroll) { // only handle regular keys if we're not scrolling
                        if (pinstate[j][i])
                            Keyboard2.pressScancode(scancode);
                        else
                            Keyboard2.releaseScancode(scancode);
                    }
                    else {
                        // a scroll key was probably triggered
                        // reset scroll time delay so that scrolling will immediately trigger
                        lastscroll = lastpress - SCROLL_DELAY*1000;
                    }
                    lastpinstate[j][i] = pinstate[j][i];
                    if (macrorecording && !doscroll && scancode != 0xFF && scancode != HID_KEY_NONE) { // shouldn't ever hit none, but just to be safe...
                        macro_scancode[activemacro].push_back(scancode);
                        macro_pressed[activemacro].push_back(pinstate[j][i]);
                    }
                }
            }
        }


        if (doscroll) { // intercept for scrolling
            uint64_t now = to_us_since_boot(get_absolute_time());
            if (now - lastscroll > SCROLL_DELAY*1000) {
                if (pinstate[0][16] == true) {
                    usb_hid.mouseReport(2,0,0,0,1,0); // scroll up
                }
                else if (pinstate[0][15] == true) {
                    usb_hid.mouseReport(2,0,0,0,-1,0); // scroll down
                }
                if (pinstate[1][19] == true) {
                    usb_hid.mouseReport(2,0,0,0,0,1); // scroll right
                }
                else if (pinstate[6][0] == true) {
                    usb_hid.mouseReport(2,0,0,0,0,-1); // scroll left
                }
                lastscroll = now;
            }
            else if (now - lastpress > SCROLL_TIMEOUT*1e6) {
                // after 15 seconds exit out of scroll mode anyway
                doscroll = false;
            }
        }
    }
}
