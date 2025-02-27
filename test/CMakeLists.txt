cmake_minimum_required(VERSION 3.29)
project(display_test)

set(CMAKE_CXX_STANDARD 20)

add_executable(mono_vlsb_test
        test_main_mono_vlsb.cpp
        mono_vlsb_test.cpp
        mono_vlsb_test.h
        ../display/framebuf.cpp
        ../display/framebuf.h
        ../display/mono_vlsb.cpp
        ../display/mono_vlsb.h
        ../display/fonts/font_petme128_8x8.h
        ../display/fonts/FreeMono12pt7b.h
        ../display/fonts/gfxfont.h
)

target_include_directories(mono_vlsb_test  PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ../display
        ../display/fonts
)

add_executable(mono_hlsb_test
        test_main_mono_hlsb.cpp
        mono_hlsb_test.cpp
        mono_hlsb_test.h
        ../display/framebuf.cpp
        ../display/framebuf.h
        ../display/mono_horiz.cpp
        ../display/mono_horiz.h
        ../display/fonts/font_petme128_8x8.h
        ../display/fonts/FreeMono12pt7b.h
        ../display/fonts/gfxfont.h
)

target_include_directories(mono_hlsb_test  PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ../display
        ../display/fonts
)
