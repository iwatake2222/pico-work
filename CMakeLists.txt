cmake_minimum_required(VERSION 3.12)

set(PICO_SDK_PATH ${CMAKE_CURRENT_LIST_DIR}/pico-sdk)
include(pico_sdk_import.cmake)

set(ProjectName "pico-work")
project(${ProjectName})
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_subdirectory(pico-tflmicro)

# add_subdirectory(pj_base)
# add_subdirectory(pj_paint)
# add_subdirectory(pj_tflmicro_sin)
# add_subdirectory(pj_tflmicro_mnist)

add_subdirectory(pj_adc)
