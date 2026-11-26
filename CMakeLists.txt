cmake_minimum_required(VERSION 3.20)
project(liquid_glass_ui LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED IMPORTED_TARGET gtk4)
pkg_check_modules(LAYER_SHELL REQUIRED IMPORTED_TARGET gtk4-layer-shell-0)
pkg_check_modules(EPOXY REQUIRED IMPORTED_TARGET epoxy)
pkg_check_modules(PANGOCAIRO REQUIRED IMPORTED_TARGET pangocairo)
pkg_check_modules(EGL REQUIRED IMPORTED_TARGET egl)

add_executable(liquid-glass-ui
    src/main.cpp
    src/shader_utils.cpp
    src/desktop_capture.cpp
    src/glass_renderer.cpp
    src/ui_widgets.cpp
    src/preview_dump.cpp
)

# Link gtk4-layer-shell before wayland-client (via gtk4/layer-shell pkg-config order)
target_link_libraries(liquid-glass-ui PRIVATE
    PkgConfig::LAYER_SHELL
    PkgConfig::GTK4
    PkgConfig::EPOXY
    PkgConfig::PANGOCAIRO
    PkgConfig::EGL
)

target_compile_options(liquid-glass-ui PRIVATE
    -O2
    -Wall
    -Wextra
    -Wno-unused-parameter
)

target_compile_definitions(liquid-glass-ui PRIVATE
    LIQUID_GLASS_SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/shaders"
)
