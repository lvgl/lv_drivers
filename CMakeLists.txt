cmake_minimum_required(VERSION 3.13)

project(lv_drivers HOMEPAGE_URL https://github.com/lvgl/lv_drivers/)

# Option to set the project as bundled.
# If bundled, the installation steps are skipped because lv_drivers are linked
# directly with any dependant targets.
option( LV_DRIVERS_BUNDLED "Set lv_drivers as bundled (disables install)" OFF )

# Option to build as shared library (as opposed to static), default: OFF
option(BUILD_SHARED_LIBS "Build shared as library (as opposed to static)" OFF)

file(GLOB_RECURSE SOURCES ./*.c)

if (BUILD_SHARED_LIBS)
  add_library(lv_drivers SHARED ${SOURCES})
else()
  add_library(lv_drivers STATIC ${SOURCES})
endif()

add_library(lvgl_drivers ALIAS lv_drivers)
add_library(lvgl::drivers ALIAS lv_drivers)

target_include_directories(lv_drivers SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

find_package(PkgConfig)
pkg_check_modules(PKG_WAYLAND wayland-client wayland-cursor wayland-protocols xkbcommon)
target_link_libraries(lv_drivers PUBLIC lvgl ${PKG_WAYLAND_LIBRARIES})

option( LV_DRIVERS_USE_SDL "Use SDL2 libraries" OFF )

if( LV_DRIVERS_USE_SDL )
  find_package( SDL2 REQUIRED )
  target_link_libraries( lv_drivers PRIVATE SDL2::SDL2 )
endif( LV_DRIVERS_USE_SDL )

if("${LIB_INSTALL_DIR}" STREQUAL "")
  set(LIB_INSTALL_DIR "lib")
endif()

if("${INC_INSTALL_DIR}" STREQUAL "")
  set(INC_INSTALL_DIR "include/lvgl/lv_drivers")
endif()

if (NOT LV_DRIVERS_BUNDLED)
install(
  DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/"
  DESTINATION "${CMAKE_INSTALL_PREFIX}/${INC_INSTALL_DIR}/"
  FILES_MATCHING
  PATTERN "*.h"
  PATTERN ".git*" EXCLUDE
  PATTERN "CMakeFiles" EXCLUDE
  PATTERN "docs" EXCLUDE
  PATTERN "lib" EXCLUDE)
endif()

file(GLOB LV_DRIVERS_PUBLIC_HEADERS "${CMAKE_SOURCE_DIR}/lv_drv_conf.h")

set_target_properties(
  lv_drivers
  PROPERTIES OUTPUT_NAME lv_drivers
             ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
             LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
             RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
             PUBLIC_HEADER "${LV_DRIVERS_PUBLIC_HEADERS}")

if (NOT LV_DRIVERS_BUNDLED)
install(
  TARGETS lv_drivers
  ARCHIVE DESTINATION "${LIB_INSTALL_DIR}"
  LIBRARY DESTINATION "${LIB_INSTALL_DIR}"
  RUNTIME DESTINATION "${LIB_INSTALL_DIR}"
  PUBLIC_HEADER DESTINATION "${INC_INSTALL_DIR}")
endif()