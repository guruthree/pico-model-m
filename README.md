## Pico Model M Keyboard

This project contains code to scan the matrix of an IBM 122-key Model M keyboard ("Battleship"), identify key presses, and then act as a standard USB HID keyboard.

This code was written with a 1394324 terminal keyboard in mind, but should be easily modified for any keyboard matrix by editing keyboardlayout.h.
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
Check the RGB LED pin in RGBHandler.h and the colour order in the put_pixel call in RGBHandler.cpp.
Check CMakeLists.txt for the correct PICO_BOARD definition.
Check pico-model-m.cpp if you want to change the scroll speed and MatrixScanner.cpp for the debounce time or ghosting protection.

After setting up the [pico-sdk](https://github.com/raspberrypi/pico-sdk),
```
cd pico-model-m
mkdir build
cd build
cmake ..
make
```
