cmake_minimum_required (VERSION 2.6)
project (NESCHAN CXX)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_EXPORT_COMPILE_COMMANDS "1")

# Locate SDL include/lib
if(APPLE)
   #SET(GUI_TYPE MACOSX_BUNDLE)
   find_path(SDL2_INCLUDE_DIRS SDL.h)
   find_library(SDL2_LIBRARIES SDL2)
   mark_as_advanced(SDL2_LIBRARIES)
   SET(EXTRA_LIBS ${SDL2_LIBRARIES})
elseif(UNIX)
    find_package(PkgConfig)
    pkg_check_modules(SDL2 REQUIRED sdl2)
endif()

 # Include directories
include_directories("$(PROJECT_SOURCE_DIR)/lib/inc" ${SDL2_INCLUDE_DIRS})
add_definitions(-D_REENTRANT)

add_subdirectory(lib)
add_subdirectory(test)
add_executable(NESCHAN_APP src/neschan.cpp)
set_target_properties(NESCHAN_APP PROPERTIES OUTPUT_NAME "neschan")
target_link_libraries(NESCHAN_APP NESCHANLIB ${SDL2_LIBRARIES})
