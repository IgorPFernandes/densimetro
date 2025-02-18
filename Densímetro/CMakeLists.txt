# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.0)
set(toolchainVersion 13_3_Rel1)
set(picotoolVersion 2.1.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()

set(PICO_BOARD pico_w CACHE STRING "Board type")

include(pico_sdk_import.cmake)

project(Densimetro C CXX ASM)

pico_sdk_init()

add_executable(Densimetro 
    densimetro.c 
    src/Oled/ssd1306.c 
    src/ultrasonic/ultrasonic.c 
    src/Servo/Servo.c
)

pico_set_program_name(Densimetro "Densimetro")
pico_set_program_version(Densimetro "0.1")

pico_enable_stdio_uart(Densimetro 0)
pico_enable_stdio_usb(Densimetro 1)

target_link_libraries(Densimetro
    pico_stdlib
    pico_cyw43_arch_lwip_threadsafe_background
    hardware_i2c
    hardware_pwm
    hardware_adc
    hardware_pio
)

target_include_directories(Densimetro PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/src
    ${CMAKE_CURRENT_LIST_DIR}/src/Oled
    ${CMAKE_CURRENT_LIST_DIR}/src/ultrasonic
    ${CMAKE_CURRENT_LIST_DIR}/src/Servo
)

pico_add_extra_outputs(Densimetro)
