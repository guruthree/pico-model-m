cmake_minimum_required(VERSION 3.13)

# these need to be early to be honored on the first cmake
set(PICO_BOARD "pimoroni_pga2040" CACHE STRING "" FORCE)
set(PICO_STDIO_UART 0 CACHE BOOL "" FORCE)

include(pico_sdk_import.cmake)

project(pico-model-m_project C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

add_executable(pico-model-m
    pico-model-m.cpp
    KeyboardLayout.cpp
    USBKeyboard.cpp
    MatrixScanner.cpp
    RGBHandler.cpp
    Adafruit_TinyUSB_Arduino/src/arduino/hid/Adafruit_USBD_HID.cpp
    Adafruit_TinyUSB_Arduino/src/arduino/Adafruit_USBD_Device.cpp
    Adafruit_TinyUSB_Arduino/src/arduino/ports/rp2040/Adafruit_TinyUSB_rp2040.cpp
)

pico_generate_pio_header(pico-model-m ${CMAKE_CURRENT_LIST_DIR}/includes/ws2812.pio)

target_include_directories(pico-model-m PRIVATE ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(pico-model-m
    pico_stdlib
    pico_unique_id
    tinyusb_device
    tinyusb_board
    hardware_pio
    pico_multicore
)
pico_add_extra_outputs(pico-model-m)

add_definitions(-DUSE_TINYUSB)
add_definitions(-DUSB_VID=0x2e8a)
add_definitions(-DUSB_PID=0xb475)
add_definitions(-DUSB_MANUFACTURER="@theguruofthree")
add_definitions(-DUSB_PRODUCT="IBM Battleship Keyboard")
add_definitions(-DARDUINO_ARCH_RP2040)
add_definitions(-DCFG_TUSB_CONFIG_FILE="includes/tusb_config.h")

# for bi_decl
execute_process(COMMAND git log --pretty=format:"%h" -n 1
                OUTPUT_VARIABLE GIT_REV
                ERROR_QUIET)
add_definitions(-DVERSION=${GIT_REV})
string(TIMESTAMP BUILD_TIME "%Y-%m-%d %H:%M:%S")
add_definitions(-DBUILD_TIME="${BUILD_TIME}")


include_directories(includes
    Adafruit_TinyUSB_Arduino/src
)

# remove Adafruit tinyusb library code so that only the pico-sdk tinyusb code is used
# to restore, cd Adafruit_TinyUSB_Arduino/ && git checkout HEAD .
file(REMOVE Adafruit_TinyUSB_Arduino/src/tusb.c)
file(REMOVE Adafruit_TinyUSB_Arduino/src/tusb.h)
file(REMOVE Adafruit_TinyUSB_Arduino/src/tusb_option.h)
file(REMOVE_RECURSE Adafruit_TinyUSB_Arduino/src/osal/)
file(REMOVE_RECURSE Adafruit_TinyUSB_Arduino/src/portable/)
file(REMOVE_RECURSE Adafruit_TinyUSB_Arduino/src/device/)
file(REMOVE_RECURSE Adafruit_TinyUSB_Arduino/src/common/)
file(REMOVE_RECURSE Adafruit_TinyUSB_Arduino/src/class/)
