project(lv_drivers
  HOMEPAGE_URL https://github.com/lvgl/lv_drivers/
  )

file(GLOB_RECURSE SOURCES ./*.c)
add_library(lv_drivers STATIC ${SOURCES})

include_directories(${CMAKE_SOURCE_DIR})

if("${LIB_INSTALL_DIR}" STREQUAL "")
  set(LIB_INSTALL_DIR "lib")
endif()

if("${INC_INSTALL_DIR}" STREQUAL "")
  set(INC_INSTALL_DIR "include/lvgl/lv_drivers")
endif()

install(DIRECTORY "${CMAKE_SOURCE_DIR}/"
  DESTINATION "${CMAKE_INSTALL_PREFIX}/${INC_INSTALL_DIR}/"
  FILES_MATCHING
  PATTERN "*.h"
  PATTERN ".git*" EXCLUDE
  PATTERN "CMakeFiles" EXCLUDE
  PATTERN "docs" EXCLUDE
  PATTERN "lib" EXCLUDE
  )

file(GLOB LV_DRIVERS_PUBLIC_HEADERS
  "${CMAKE_SOURCE_DIR}/lv_drv_conf.h"
  )

set_target_properties(lv_drivers PROPERTIES
  OUTPUT_NAME lv_drivers
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
  PUBLIC_HEADER "${LV_DRIVERS_PUBLIC_HEADERS}"
)

install(TARGETS lv_drivers
  ARCHIVE DESTINATION "${LIB_INSTALL_DIR}"
  PUBLIC_HEADER DESTINATION "${INC_INSTALL_DIR}"
)
