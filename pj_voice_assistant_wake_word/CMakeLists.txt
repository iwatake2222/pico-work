set(BinName "pj_voice_assistant_wake_word")

if(WIN32)
    set(BUILD_ON_PC ON)
    # use this directory as top when building on PC
    cmake_minimum_required(VERSION 3.12)
    project(${BinName})
else()
    set(BUILD_ON_PC OFF)
endif()

add_executable(${BinName} "")

target_include_directories(${BinName}
  PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/
)

file(GLOB_RECURSE SRC ${CMAKE_CURRENT_LIST_DIR}/*.c ${CMAKE_CURRENT_LIST_DIR}/*.cpp ${CMAKE_CURRENT_LIST_DIR}/*.cc ${CMAKE_CURRENT_LIST_DIR}/*.cxx ${CMAKE_CURRENT_LIST_DIR}/*.h ${CMAKE_CURRENT_LIST_DIR}/*.hpp)

if(BUILD_ON_PC)
    list(FILTER SRC EXCLUDE REGEX  ".*adc_buffer")
endif()

target_sources(${BinName}
    PRIVATE
    ${SRC}
)

if(BUILD_ON_PC)
    set(DIR_TFLMICRO "${CMAKE_CURRENT_LIST_DIR}/../generic-tflmicro/src")
    add_subdirectory(${DIR_TFLMICRO} ./generic-tflmicro)
    target_link_libraries(${BinName} "generic-tflmicro")
    add_definitions(-DBUILD_ON_PC)
else()
    pico_enable_stdio_usb(${BinName} 1)
    pico_enable_stdio_uart(${BinName} 1)

    target_link_libraries(${BinName} 
        pico_stdlib
        pico-tflmicro
        hardware_spi
        hardware_adc
        hardware_dma
        hardware_irq
        pico_multicore
    )

    pico_add_extra_outputs(${BinName})
endif()
