# CMakeList.txt : CMake project for canboy, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.10)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

# vcpkg path
set(CMAKE_TOOLCHAIN_FILE
    "C:/Program Files/Microsoft Visual Studio/18/Community/VC/vcpkg/scripts/buildsystems/vcpkg.cmake"
    CACHE STRING "")

project ("canboy")

# Add source to this project's executable.
add_executable ( canboy
    src/Emu.cpp
	src/main.cpp
    src/Bus.cpp
    src/lr35902.cpp
    src/instructions.cpp
    src/registers.cpp
    platform/Windows/menu.h
    platform/Windows/menu.cpp
 "include/Emu.h" "src/Emu.cpp" "include/cartridge.h" "src/cartridge.cpp" "include/io.h" "src/io.cpp" "include/timer.h" "src/timer.cpp" "include/ppu.h" "src/ppu.cpp" "include/renderer.h" "src/renderer.cpp" "include/joypad.h" "src/joypad.cpp" "external/nfd.h" "external/nfd_common.h" "external/nfd_common.c" "external/nfd_win.cpp" "include/apu.h" "src/apu.cpp" )

# target_link_options(canboy PRIVATE "/ENTRY:mainCRTStartup")

find_package(SDL3 CONFIG REQUIRED)
find_package(SDL3_ttf CONFIG REQUIRED)
target_link_libraries(canboy PRIVATE SDL3::SDL3 SDL3_ttf::SDL3_ttf)

target_include_directories(canboy PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/external
    ${CMAKE_SOURCE_DIR}/platform/Windows
)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET canboy PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
