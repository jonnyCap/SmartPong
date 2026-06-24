cmake_minimum_required(VERSION 3.16)

set(SMARTPONG_FIRMWARE_DIR "${CMAKE_CURRENT_LIST_DIR}/firmware")

set(SDKCONFIG "${SMARTPONG_FIRMWARE_DIR}/sdkconfig")
set(SDKCONFIG_DEFAULTS "${SMARTPONG_FIRMWARE_DIR}/sdkconfig-example")
set(EXTRA_COMPONENT_DIRS "${SMARTPONG_FIRMWARE_DIR}/main")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

idf_build_set_property(DEPENDENCIES_LOCK "${SMARTPONG_FIRMWARE_DIR}/dependencies.lock")

# Trim the build to the minimal set of required components.
idf_build_set_property(MINIMAL_BUILD ON)

project(firmware)
