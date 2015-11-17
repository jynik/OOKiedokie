cmake_minimum_required(VERSION 2.8.12)
project(ookiedokie C)

################################################################################
# Project configuration
################################################################################

# All build output lands in this directory:
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake/Modules)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
    message(STATUS "Build type not specified: defaulting to a release build.")
endif()

set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "Build type")

################################################################################
# Version information 
################################################################################

set(VERSION_INFO_MAJOR  0)
set(VERSION_INFO_MINOR  1)
set(VERSION_INFO_PATCH  0)

if(NOT DEFINED VERSION_INFO_EXTRA)
    set(VERSION_INFO_EXTRA "git")
endif()
include(Version)

set(VERSION "${VERSION_INFO}")

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/src/version.h
    @ONLY
)

################################################################################
# Configuration options
################################################################################

include(FindPkgConfig)
include(FindLibjansson)
include(FindLibbladeRF)

option(ENABLE_BLADERF
       "Enable support for the Nuand bladeRF (https://www.nuand.com)"
       ${LIBBLADERF_FOUND})

option(ENABLE_BLADERF_SC16Q11_FILE 
       "Enable support for files containing raw data in the bladeRF's SC16 Q11 format."   
       ON)

option(BUILD_FIR_TEST
       "Build FIR filter test program"
       OFF)

if(NOT DEFINED OOKIEDOKIE_BIN_DIR)
    set(OOKIEDOKIE_BIN_DIR bin)
endif()
set(OOKIEDOKIE_BIN_DIR ${OOKIEDOKIE_BIN_DIR} CACHE string "Directory under install prefix to install ookiedokie binary")


if(NOT DEFINED OOKIEDOKIE_DATA_DIR)
    set(OOKIEDOKIE_DATA_DIR share/OOKiedokie)
endif()
set(OOKIEDOKIE_DATA_DIR ${OOKIEDOKIE_DATA_DIR} CACHE string "Directory under install prefix to install auxillary files, such as device specification files")

message(STATUS "OOKIEDOKIE_BIN_DIR set to ${CMAKE_INSTALL_PREFIX}/${OOKIEDOKIE_BIN_DIR}")
message(STATUS "OOKIEDOKIE_DATA_DIR set to ${CMAKE_INSTALL_PREFIX}/${OOKIEDOKIE_DATA_DIR}\n")

################################################################################
# ookiedokie Build
################################################################################

set(OOKIEDOKIE_INCLUDE_DIRS
        src/
        ${CMAKE_CURRENT_BINARY_DIR}/src
        ${LIBJANSSON_INCLUDE_DIR}
)

set(OOKIEDOKIE_SOURCE
        src/main.c
        src/conversions.c
        src/device.c
        src/find.c
        src/fir.c
        src/formatter.c
        src/keyval_list.c
        src/log.c
        src/ookiedokie.c
        src/ookiedokie_cfg.c
        src/state_machine.c
        src/sdr/sdr.c
)

set(OOKIEDOKIE_LIBS
        ${LIBJANSSON_LIBRARIES}
        m
)

add_definitions("-DOOKIEDOKIE_DATA_DIR=\"${CMAKE_INSTALL_PREFIX}/${OOKIEDOKIE_DATA_DIR}/\"")

if(ENABLE_BLADERF_SC16Q11_FILE)
    add_definitions("-DENABLE_BLADERF_SC16Q11_FILE=1")
    set(OOKIEDOKIE_SOURCE ${OOKIEDOKIE_SOURCE} src/sdr/bladeRF_file.c)
endif()

if(ENABLE_BLADERF)
    add_definitions("-DENABLE_BLADERF=1")
    set(OOKIEDOKIE_SOURCE ${OOKIEDOKIE_SOURCE} src/sdr/bladeRF.c)
    set(OOKIEDOKIE_INCLUDE_DIRS ${OOKIEDOKIE_INCLUDE_DIRS} ${LIBBLADERF_INCLUDE_DIR})
    set(OOKIEDOKIE_LIBS ${OOKIEDOKIE_LIBS} ${LIBBLADERF_LIBRARIES})
endif()

include_directories(${OOKIEDOKIE_INCLUDE_DIRS})

# Shorten filenames in log output
set(SRC_TO_SHORTEN ${OOKIEDOKIE_SOURCE})
include(ShortFileMacro)

add_executable(ookiedokie ${OOKIEDOKIE_SOURCE})
target_link_libraries(ookiedokie ${OOKIEDOKIE_LIBS})

################################################################################
# fir_test Build 
################################################################################
if(BUILD_FIR_TEST)
    set(FIR_TEST_SOURCE
        src/conversions.c
        src/find.c
        src/fir.c
        src/log.c

        src/test/fir_test.c
    )
    
    set(FIR_TEST_LIBS
        ${LIBJANSSON_LIBRARIES}
        m
    )

    set(SRC_TO_SHORTEN ${FIR_TEST_SOURCE})
    include(ShortFileMacro)

    add_executable(fir_test ${FIR_TEST_SOURCE})
    target_link_libraries(fir_test ${FIR_TEST_LIBS})

    set_target_properties(fir_test PROPERTIES 
        COMPILE_FLAGS "-DENABLE_FIR_VERBOSE_LOGGING=1"
    )

endif()

################################################################################
# Installation
################################################################################

install(TARGETS ookiedokie DESTINATION ${OOKIEDOKIE_BIN_DIR})

install(
    DIRECTORY
        ${CMAKE_CURRENT_SOURCE_DIR}/filters
        ${CMAKE_CURRENT_SOURCE_DIR}/devices
    DESTINATION
        ${CMAKE_INSTALL_PREFIX}/${OOKIEDOKIE_DATA_DIR}
)

################################################################################
# Uninstallation
################################################################################

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND
    ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
)

