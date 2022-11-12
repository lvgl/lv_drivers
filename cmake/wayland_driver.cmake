project(lv_wayland)

option(USE_WL_SHELL "use wl shell (discard, no recommend)" OFF)
option(USE_XDG_SHELL "use xdg shell" OFF)
option(USE_WAYLAND_TIMER_HANDLER "" OFF)

set(WAYLAND_SCANNER_EXECUTABLE)

macro(wayland_generate protocol_xml_file output_dir)

    if(NOT EXISTS ${protocol_xml_file})
        message(FATAL_ERROR "-- protocol XML file not found: " ${protocol_xml_file})
    endif()

    get_filename_component(output_file_base ${protocol_xml_file} NAME_WE)
    set(xdg_shell_file "${output_dir}/wayland-${output_file_base}-client-protocol")
    message("-- generating ${xdg_shell_file}.h")
    execute_process(COMMAND "${WAYLAND_SCANNER_EXECUTABLE}"  client-header  "${protocol_xml_file}" "${xdg_shell_file}.h")
    message("-- generating ${xdg_shell_file}.c")
    execute_process(COMMAND "${WAYLAND_SCANNER_EXECUTABLE}"  private-code "${protocol_xml_file}"  "${xdg_shell_file}.c")
endmacro()

if (NOT USE_WL_SHELL AND USE_XDG_SHELL)
    message(FATAL_ERROR "Please select at least one of USE_WL_SHELL and USE_XDG_SHELL (that is, can both select)")
endif()

if (USE_WL_SHELL)
    add_definitions(-DLV_WAYLAND_WL_SHELL=1)
endif()

if (USE_XDG_SHELL)
    add_definitions(-DLV_WAYLAND_XDG_SHELL=1)
endif()

if (USE_WAYLAND_TIMER_HANDLER)
    add_definitions(-DLV_WAYLAND_TIMER_HANDLER=1)
endif()

add_definitions(-DUSE_WAYLAND=1)

# wayland driver dependences
find_package(PkgConfig)
pkg_check_modules(PKG_WAYLAND wayland-client wayland-cursor wayland-protocols xkbcommon)
message("-- Find wayland dependences : ${PKG_WAYLAND_LIBRARIES}")

if (USE_XDG_SHELL)
    message("-- select xdg shell")
    find_program(WAYLAND_SCANNER_EXECUTABLE NAMES wayland-scanner)
    pkg_check_modules(WAYLAND_PROTOCOLS REQUIRED wayland-protocols>=1.25)
    if (NOT EXISTS WAYLAND_PROTOCOLS_BASE) # may be you need to specify wayland protocol path for cross compile
        pkg_get_variable(WAYLAND_PROTOCOLS_BASE wayland-protocols pkgdatadir)
    endif()
    message("-- find wayland scanner : ${WAYLAND_SCANNER_EXECUTABLE}")
    message("-- find wayland protocol path : ${WAYLAND_PROTOCOLS_BASE}")

    # use wayland scanner with xdg-shell.xml to generate xgd protocol source and header files 
    set(WAYLAND_PROTOCOLS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/wayland/protocols")
    file(MAKE_DIRECTORY ${WAYLAND_PROTOCOLS_DIR})
    wayland_generate("${WAYLAND_PROTOCOLS_BASE}/stable/xdg-shell/xdg-shell.xml" ${WAYLAND_PROTOCOLS_DIR})
    include_directories(${WAYLAND_PROTOCOLS_DIR})
endif()