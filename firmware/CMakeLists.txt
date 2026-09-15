# The firmware build. Driven by scripts/build_firmware.sh, which pins the SDK
# and the compiler (scripts/toolchain_versions.env); invoking cmake by hand
# works too provided PICO_SDK_PATH and PICO_TOOLCHAIN_PATH are set the same way.
cmake_minimum_required(VERSION 3.13...3.27)

# The Pico 2 W. Set as a cache default so the Waveshare wiki's note about
# pico versus pico2 builds does not apply: this project has one board.
set(PICO_BOARD pico2_w CACHE STRING "Board type")

include(pico_sdk_import.cmake)

project(stopbath_pico C CXX ASM)

pico_sdk_init()

set(REPOSITORY_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")

# The Flipper repository's warning set, mirrored in the host Makefile. This
# project's own sources take the full set; vendored code the base set, as the
# Makefile does for the host build.
set(BASE_WARNINGS -Wall -Wextra -Werror -Wstrict-prototypes -Wredundant-decls -Wdouble-promotion -Wundef)
set(PROJECT_WARNINGS ${BASE_WARNINGS} -Wshadow -Wconversion)
set(VENDORED_WARNINGS ${BASE_WARNINGS})

set(PROJECT_SOURCES
    first_light.c
    DEV_Config.c
    ${REPOSITORY_ROOT}/remote_input/remote_input_model.c
    ${REPOSITORY_ROOT}/remote_display/remote_bitmap.c
    ${REPOSITORY_ROOT}/remote_display/remote_font.c)

# The first light image. Two were built, one per vendored driver, until the
# author observed on 2026-09-15 that the V2 driver lights the panel and the
# V1 does not (evaluation log 4.3); the V1 driver and its image were removed.
function(add_first_light_image target_name panel_binding vendored_driver)
    add_executable(${target_name} ${PROJECT_SOURCES} ${panel_binding} ${vendored_driver})
    target_include_directories(${target_name} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${REPOSITORY_ROOT}/lib/waveshare)
    target_link_libraries(${target_name} PRIVATE pico_stdlib hardware_spi hardware_gpio hardware_timer)
    set_source_files_properties(${PROJECT_SOURCES} ${panel_binding} PROPERTIES COMPILE_OPTIONS "${PROJECT_WARNINGS}")
    set_source_files_properties(${vendored_driver} PROPERTIES COMPILE_OPTIONS "${VENDORED_WARNINGS}")
    # No stdio of any kind: the SDK's USB stdio would translate line endings
    # on the channel the protocol will use (evaluation log 4.1), and there is
    # nothing to print.
    pico_enable_stdio_usb(${target_name} 0)
    pico_enable_stdio_uart(${target_name} 0)
    pico_add_extra_outputs(${target_name})
endfunction()

add_first_light_image(stopbath_pico_first_light panel.c ${REPOSITORY_ROOT}/lib/waveshare/EPD_4in2_V2.c)
