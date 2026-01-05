set(DEFAULT_BUILD_TYPE "Release")

if(NOT CMAKE_BUILD_TYPE)
  message(
    STATUS
      "Setting build type to '${DEFAULT_BUILD_TYPE}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${DEFAULT_BUILD_TYPE}")
endif()

# Find the binaries for the toolchain
find_program(AVR_CC avr-gcc REQUIRED)
find_program(AVR_CXX avr-g++ REQUIRED)
find_program(AVR_OBJCOPY avr-objcopy REQUIRED)
find_program(AVR_OBJDUMP avr-objdump REQUIRED)

if(TOOLCHAIN_USE_PYMCUPROG)
  find_program(PROGRAMMER pymcuprog REQUIRED)
else()
  find_program(PROGRAMMER avrdude OPTIONAL)
endif()

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR avr)

set(CMAKE_C_STANDARD_REQUIRED TRUE)
set(CMAKE_C_COMPILER ${AVR_CC})

set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_COMPILER ${AVR_CXX})
set(CMAKE_ASM_COMPILER ${AVR_CC})

# Export compile commands for use with LSPs
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

enable_language(C CXX ASM)

# ------------------------------- Device pack --------------------------------

# Find the device pack if specified (this is needed for newer devices which
# don't have native support in avr-gcc)
if(TOOLCHAIN_NEED_DEVICE_PACK)

  set(DEVICE_PACK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/device_pack)

  if(NOT EXISTS ${DEVICE_PACK_DIR})
    message(
      FATAL_ERROR
        "Device pack not found in root of project directory. The device pack (ATPACK/DFP) should be in a folder named 'device_pack'. Download it for your device at http://packs.download.atmel.com."
    )
  endif()

  # Helper variables for the device related directories in the ATPACK and the
  # toolchain
  set(MCU_DEVICE_DIRECTORY ${DEVICE_PACK_DIR}/gcc/dev/${MCU})
  set(MCU_INCLUDE_DIRECTORY ${DEVICE_PACK_DIR}/include)

  if(NOT EXISTS ${MCU_DEVICE_DIRECTORY}/device-specs/specs-${MCU})
    message(
      FATAL_ERROR
        "Could not find device pack for ${MCU}, tried looking in directory ${MCU_DEVICE_DIRECTORY}/device-specs/specs-${MCU}. The device name might be misspelled or the device pack might be incorrect for this device."
    )
  endif()

  # Find the device library name by going through the device specs for the
  # device
  file(READ ${MCU_DEVICE_DIRECTORY}/device-specs/specs-${MCU}
       SPECS_FILE_CONTENT)
  string(
    REGEX MATCH
          "-D__AVR_DEVICE_NAME__=${MCU} -D__AVR_DEV_LIB_NAME__=([a-zA-Z0-9]*)"
          _ ${SPECS_FILE_CONTENT})
  set(MCU_DEV_LIB_NAME ${CMAKE_MATCH_1})

endif()

# ---------------------------- Definitions & flags -----------------------------

if(NOT DEFINED F_CPU)
  message(WARNING "F_CPU not defined, has to be defined in code")
else()
  set(TOOLCHAIN_COMPILE_DEFINITIONS F_CPU=${F_CPU})
endif()

set(TOOLCHAIN_COMPILE_DEFINITIONS
    ${TOOLCHAIN_COMPILE_DEFINITIONS} $<$<CONFIG:Debug>:DEBUG>
    $<$<CONFIG:Release>:NDEBUG>)

set(TOOLCHAIN_COMPILE_OPTIONS
    -mmcu=${MCU}
    -Werror
    -Wall
    -Wextra
    -Wpedantic
    -Wshadow
    -Wno-array-bounds
    -Wno-vla
    # Optimisations
    -funsigned-char
    -funsigned-bitfields
    -fpack-struct
    -fshort-enums
    -ffunction-sections
    -fdata-sections
    -fno-split-wide-types
    -fno-tree-scev-cprop
    $<$<CONFIG:Debug>:-Og>
    $<$<CONFIG:Debug>:-Wno-unused-function>
    $<$<CONFIG:Release>:-Os>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
    $<$<COMPILE_LANGUAGE:CXX>:-Wno-volatile>
    $<$<COMPILE_LANGUAGE:CXX>:-Wno-register>)

set(TOOLCHAIN_LINK_OPTIONS
    -mmcu=${MCU}
    -Wl,-Map=${TARGET}.map
    -Wl,--print-memory-usage
    -Wl,--gc-section
    -Wl,--sort-section=alignment
    -Wl,--cref
    $<$<CONFIG:Release>:-Os>
    $<$<CONFIG:Debug>:-Og>)

if(TOOLCHAIN_NEED_DEVICE_PACK)
  set(TOOLCHAIN_COMPILE_DEFINITIONS ${TOOLCHAIN_COMPILE_DEFINITIONS}
                                    __AVR_DEV_LIB_NAME__=${MCU_DEV_LIB_NAME})

  set(TOOLCHAIN_COMPILE_OPTIONS
      ${TOOLCHAIN_COMPILE_OPTIONS}
      # Include the AVR header files from the ATPACK
      -I${MCU_INCLUDE_DIRECTORY}
      # Notify the compiler about the device specs
      -B${MCU_DEVICE_DIRECTORY})

  set(TOOLCHAIN_LINK_OPTIONS
      ${TOOLCHAIN_LINK_OPTIONS}
      # Notify the compiler about the device specs
      -B${MCU_DEVICE_DIRECTORY})

endif()

add_compile_options(${TOOLCHAIN_COMPILE_OPTIONS})
add_compile_definitions(${TOOLCHAIN_COMPILE_DEFINITIONS})
add_link_options(${TOOLCHAIN_LINK_OPTIONS})

function(configure_target TARGET)

  if(NOT ARGN)
    message(FATAL_ERROR "No source files given for ${TARGET}.")
  endif(NOT ARGN)

  add_executable(${TARGET} ${SOURCES})
  set_target_properties(${TARGET} PROPERTIES OUTPUT_NAME ${TARGET}.elf)

  # STRIP
  add_custom_target(
    strip ALL
    avr-strip ${TARGET}.elf
    DEPENDS ${TARGET}
    COMMENT "Stripping ${TARGET}.elf")

  # HEX
  add_custom_target(
    hex ALL
    ${AVR_OBJCOPY} -O ihex ${TARGET}.elf ${TARGET}.hex
    DEPENDS strip
    COMMENT "Creating ${TARGET}.hex")

  # EEPROM
  add_custom_target(
    eeprom ALL
    ${AVR_OBJCOPY}
    -j
    .eeprom
    --set-section-flags=.eeprom=alloc,load
    --change-section-lma
    .eeprom=0
    --no-change-warnings
    -O
    ihex
    ${TARGET}.elf
    ${TARGET}-eeprom.hex
    DEPENDS strip
    COMMENT "Creating ${TARGET}-eeprom.hex")

  if(TOOLCHAIN_USE_PYMCUPROG)
    add_custom_target(
      upload
      ${PROGRAMMER} write -f ${TARGET}.hex --erase --verify
      DEPENDS hex
      COMMENT "Uploading ${TARGET}.hex to ${MCU}")

    add_custom_target(
      reset
      ${PROGRAMMER} reset
      COMMENT "Resetting device...")
  else()
    add_custom_target(
      upload
      ${PROGRAMMER}
      -c
      ${AVRDUDE_PROGRAMMER_ID}
      -p
      ${AVRDUDE_MCU}
      -U
      flash:w:${TARGET}.hex:i
      DEPENDS hex
      COMMENT "Uploading ${TARGET}.hex to ${MCU}")

    add_custom_target(
      upload_eeprom
      ${PROGRAMMER}
      -c
      ${AVRDUDE_PROGRAMMER_ID}
      -p
      ${AVRDUDE_MCU}
      -U
      eeprom:w:${TARGET}-eeprom.hex:i
      DEPENDS eeprm
      COMMENT "Uploading ${TARGET}-eeprom.hex to ${MCU}")
  endif()

  # Disassemble
  add_custom_target(
    disassemble
    ${AVR_OBJDUMP} -h -S ${TARGET}.elf > ${TARGET}.lst
    DEPENDS strip
    COMMENT "Disassembling ${TARGET}.elf")

  # Remove .hex, .map, eeprom .hex and .lst on clean
  set_property(
    TARGET ${TARGET}
    APPEND
    PROPERTY ADDITIONAL_CLEAN_FILES ${TARGET}.hex)

  set_property(
    TARGET ${TARGET}
    APPEND
    PROPERTY ADDITIONAL_CLEAN_FILES ${TARGET}.map)

  set_property(
    TARGET ${TARGET}
    APPEND
    PROPERTY ADDITIONAL_CLEAN_FILES ${TARGET}-eeprom.hex)

  set_property(
    TARGET ${TARGET}
    APPEND
    PROPERTY ADDITIONAL_CLEAN_FILES ${TARGET}.lst)

endfunction(configure_target)
