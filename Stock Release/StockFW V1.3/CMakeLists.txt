cmake_minimum_required(VERSION 3.20)

# ---------------------------- Target Configuration ----------------------------

# Specify the target/project name
set(TARGET tram8)

set(MCU atmega8a)
#set(F_CPU 16000000)

set(AVRDUDE_MCU m4809)
set(AVRDUDE_PROGRAMMER_ID pkobn_updi)

# Optionally, use pymcuprog instead of avrdude:
#
# set(TOOLCHAIN_USE_PYMCUPROG true)

# Specify the desired language standard
set(CMAKE_C_STANDARD 23)
set(CMAKE_CXX_STANDARD 14)

# Include the toolchain. If a device pack is needed, keep this value to true.
# The device pack should be in a folder name 'device_pack'
set(TOOLCHAIN_NEED_DEVICE_PACK false)
include(cmake/Toolchain.cmake)

# ----------------------------------- Target -----------------------------------

project(tram8)
file(GLOB SOURCES Tram8/*.c)

configure_target(${TARGET} ${SOURCES})

# Specify own definitions, include directories and libraries here. The
# AVRToolChain.cmake file already specifies a bunch of standard compile options
# and definitions

# target_compile_definitions(${TARGET} ...)
target_include_directories(${TARGET} PRIVATE Tram8)
#target_link_libraries(${TARGET} PRIVATE m)
