cmake_minimum_required(VERSION 3.20)
project(ps5_overlay C CXX)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Check for PS5 Payload SDK
if(DEFINED ENV{PS5_PAYLOAD_SDK} OR DEFINED PS5_PAYLOAD_SDK)
    set(IS_PS5_BUILD TRUE)
    if(NOT DEFINED PS5_PAYLOAD_SDK)
        set(PS5_PAYLOAD_SDK "$ENV{PS5_PAYLOAD_SDK}")
    endif()
    message(STATUS "PS5 Payload SDK detected at: ${PS5_PAYLOAD_SDK}")
else()
    set(IS_PS5_BUILD FALSE)
    message(STATUS "PS5 Payload SDK not found, building host development/test binary")
endif()

set(INCLUDE_DIRS
    include
)

set(COMMON_SOURCES
    src/main.cpp
    src/monitor.cpp
    src/overlay_ui.cpp
    src/notify.cpp
    src/config.cpp
    src/web_server.cpp
    src/injector/pt.c
    src/injector/elfldr.c
    src/injector/shellui_inject.cpp
)

if(IS_PS5_BUILD)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)

    # 1. Target: In-game HUD payload injected into SceShellUI
    set(SHELLUI_SOURCES
        src/shellui/shellui_main.cpp
    )

    add_executable(ps5_overlay_shellui ${SHELLUI_SOURCES})
    set_target_properties(ps5_overlay_shellui PROPERTIES
        OUTPUT_NAME "ps5_overlay_shellui"
        SUFFIX ".elf"
    )

    target_compile_definitions(ps5_overlay_shellui PRIVATE
        __PS5__=1
        PS5=1
        _GNU_SOURCE
    )

    target_include_directories(ps5_overlay_shellui PRIVATE
        ${INCLUDE_DIRS}
        ${PS5_PAYLOAD_SDK}/target/include
    )

    target_compile_options(ps5_overlay_shellui PRIVATE
        -O2
        -Wall
        -Wextra
        -fPIC
        -fPIE
    )

    target_link_libraries(ps5_overlay_shellui PRIVATE
        kernel_sys
        SceLibcInternal
    )

    # 2. Custom command: Embed ps5_overlay_shellui.elf into C source
    set(EMBEDDED_SHELLUI_C "${CMAKE_CURRENT_BINARY_DIR}/embedded_shellui.c")
    add_custom_command(
        OUTPUT "${EMBEDDED_SHELLUI_C}"
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/bin2c.py"
                "$<TARGET_FILE:ps5_overlay_shellui>"
                "${EMBEDDED_SHELLUI_C}"
                "g_overlay_shellui_elf"
        DEPENDS ps5_overlay_shellui "${CMAKE_CURRENT_SOURCE_DIR}/tools/bin2c.py"
        COMMENT "Converting ps5_overlay_shellui.elf to embedded C source"
        VERBATIM
    )

    # 3. Target: Main loader daemon & Web HUD server
    add_executable(ps5_overlay ${COMMON_SOURCES} "${EMBEDDED_SHELLUI_C}")
    set_target_properties(ps5_overlay PROPERTIES
        OUTPUT_NAME "ps5_overlay"
        SUFFIX ".elf"
    )

    target_compile_definitions(ps5_overlay PRIVATE
        __PS5__=1
        PS5=1
        _GNU_SOURCE
    )

    target_include_directories(ps5_overlay PRIVATE
        ${INCLUDE_DIRS}
        ${PS5_PAYLOAD_SDK}/target/include
        ${CMAKE_CURRENT_BINARY_DIR}
    )

    target_compile_options(ps5_overlay PRIVATE
        -O2
        -Wall
        -Wextra
        -fPIC
        -fPIE
    )

    target_link_libraries(ps5_overlay PRIVATE
        kernel_sys
        SceNotification
        SceLibcInternal
        SceNet
    )

    add_dependencies(ps5_overlay ps5_overlay_shellui)

    # Post-build: copy output ELFs to dist/
    add_custom_command(TARGET ps5_overlay POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/dist"
        COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:ps5_overlay>" "${CMAKE_SOURCE_DIR}/dist/ps5_overlay.elf"
        COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:ps5_overlay_shellui>" "${CMAKE_SOURCE_DIR}/dist/ps5_overlay_shellui.elf"
        COMMENT "Copied ps5_overlay.elf and ps5_overlay_shellui.elf to dist/"
    )

else()
    # Host development / testing executable
    add_executable(ps5_overlay ${COMMON_SOURCES} src/embedded_shellui.c)

    target_include_directories(ps5_overlay PRIVATE
        ${INCLUDE_DIRS}
    )

    target_compile_options(ps5_overlay PRIVATE
        -O2
        -Wall
        -Wextra
    )

    if(WIN32)
        target_link_libraries(ps5_overlay PRIVATE ws2_32)
    else()
        target_link_libraries(ps5_overlay PRIVATE pthread)
    endif()
endif()
