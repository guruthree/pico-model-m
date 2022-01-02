## Pico Model M Keyboard

This project contains code to scan the matrix of an IBM 122-key Model M keyboard ("Battleship"), identify key presses, and then act as a standard USB HID keyboard.

This code was written with a 1394324 terminal keyboard in mind, but should be easily modified for any keyboard matrix by editing KeyboardLayout.{cpp,h}.
I wrote this code as I found the QMK/TMK/etc projects a bit confusing and I wasn't quite convinced it would offer the flexibility I wanted.
Particularly, I found most projects operating in a 'converter' mode relied on atmega32u4 chips.
I've not had great luck with these when trying to make a converter previously, you're still limited by the original keyboard controller, and they use the full AVR programming environment (including programming directly with avrdude), which I'm not fond of.
I found indications QMK could operate on a keyboard matrix, but that left an additional challenge.

The big immediate challenge of working directly with an 122-key membrane is that it has 8 columns and 20 rows, and thus needs a microcontroller with 28 GPIO pins.
This rules the 32u4 out.
The RP2040 however, has 36 GPIO pins.
The [Pimoroni PGA2040](https://shop.pimoroni.com/products/pga2040) exposes 30 of these GPIO pins, and at quite an affordable price (£5.50 on sale!).
The lack of a USB header on the PGA2040 is also a plus, as that makes it easy to modify the original keyboard cable to act as a USB cable.

### Compiling

Download the sources
```
git clone https://github.com/guruthree/pico-model-m.git
```

Double check the keyboard mapping, matrix pins, and special function definitions in KeyboardLayout.cpp.
If the mapping of pins to matrix is changed, there are some instances where pinstate[X][Y] is hard coded to certain keys that will need to be changed, e.g., in MatrixScanner.cpp.
Check the RGB LED pin in RGBHandler.h and the colour order in the put_pixel call in RGBHandler.cpp.
Also check the former for colour definitions and the latter for which colours num/caps/scroll lock use.
Check CMakeLists.txt for the correct PICO_BOARD definition.
Check pico-model-m.cpp if you want to change the scroll speed and MatrixScanner.cpp for the debounce time or ghosting protection.
Check KeyboardLayout.h and pico-model-m.h for number of macros and the latter as well for the terminal key combo.

After setting up the [pico-sdk](https://github.com/raspberrypi/pico-sdk),
```
cd pico-model-m
mkdir build
cd build
cmake ..
make
```

### Hardware setup

Prepare your PGA2040, it's going to need headers
I used double right-angle Dupont headers.
Attach a PGA2040 to your keyboard membrane, noting what PGA2040 pin connects to what matrix pin - you'll need to update KeyboardLayout.cpp.
I soldered right angle Dupont headers onto the original PCB, either onto pads or onto the original pull down/up resistors, clipping the other end.
This is fully and easily reversible.
Connect all of the headers for the columns and rows to the PGA2040 with Dupont extensions. 
Make sure the rows (the membrane with fewer pins) are connected to the PGA via current limiting resistors (I used 10 k Ω).
For an indicator LED, one pin can be connected to a WS2812 RGB LED - specify what pin in RGBHandler.h.
Connect a USB cable to the PGA2040, I used the keyboards original cable with Dupont extensions internally and a custom-made passive RJ45 to USB A Male externally.
This is also fully reversible.


### Usage

It's a keyboard?
Standard 101 key operation is the same.
It should be plug and play, although it won't work properly with my fiddly KVM due to some sort of signalling issue, it works fine connected through the KVM's USB hub.
Of the extra functions, F13-F24, F13 is programmed to be escape.
F14-F23 are "Magic" keys.
F14-F16 are modifier keys that combined with F17-F23 type in 3x7=21 useful things.
I have these set to be useful for programming like typing in else, continue, etc.
Of the keys to the left, the left column top to bottom is escape, pause/break, scroll lock, print screen, Windows/super.
The right column top to bottom is the again (macro) key, copy, cut, paste, context/application menu.
Magic 1-Magic 3 (F14-F16) on their own select macro 1, 2, or 3.
Magic [1,3] + Again records a macro.
Pressing again plays back the selected macro.
Macros are only key presses and releases in sequence, they do not playback at the same speed as recorded.
Macros do not record/activate magic keys or other macros.
Magic 2 (F15) + number row 0 will put the PGA2040 programming mode, i.e., it will appear as a USB drive to copy a new .uf2 firmware to.
Magic 3 (F16) + number row 0 will trigger a USB disconnect and reconnect.
The central arrow cluster key plus an arrow in a direction will send mouse scrolls in that direction continuously while pressed.
The number pad contains an extra key where the double height + would be, the upper key is the standard +, the lower (extra) key types in a ^.
