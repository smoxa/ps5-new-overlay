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

# Source files
set(SOURCES
    src/main.cpp
    src/monitor.cpp
    src/overlay_ui.cpp
    src/notify.cpp
    src/config.cpp
)

set(INCLUDE_DIRS
    include
)

if(IS_PS5_BUILD)
    add_executable(ps5_overlay ${SOURCES})

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
    )

    target_compile_options(ps5_overlay PRIVATE
        -O2
        -Wall
        -Wextra
    )

    target_link_libraries(ps5_overlay PRIVATE
        kernel_sys
        SceNotification
        SceLibcInternal
        SceNet
    )

    # Post-build: copy output to dist/
    add_custom_command(TARGET ps5_overlay POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/dist"
        COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:ps5_overlay>" "${CMAKE_SOURCE_DIR}/dist/ps5_overlay.elf"
        COMMENT "Copied ps5_overlay.elf to dist/"
    )
else()
    # Host development / testing executable
    add_executable(ps5_overlay ${SOURCES})

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
