cmake_minimum_required(VERSION 3.20)

project(TraySymbols VERSION 0.2.0 LANGUAGES CXX RC)

add_executable(TraySymbols WIN32
    src/main.cpp
    src/TraySymbols.rc
)

target_compile_features(TraySymbols PRIVATE cxx_std_17)
target_compile_definitions(TraySymbols PRIVATE
    UNICODE
    _UNICODE
    _WIN32_WINNT=0x0600
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)
target_link_libraries(TraySymbols PRIVATE
    comctl32
    gdi32
    ole32
    shell32
    user32
)

if(MSVC)
    target_compile_options(TraySymbols PRIVATE /W4 /permissive- /utf-8)
    target_link_options(TraySymbols PRIVATE /MANIFEST:NO)
endif()
