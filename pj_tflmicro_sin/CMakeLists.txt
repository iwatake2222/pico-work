set(BinName "pj_tflmicro_sin")

set(BUILD_ON_PC OFF)
# set(BUILD_ON_PC ON)

if(BUILD_ON_PC)
	# use this directory as top when building on PC
	cmake_minimum_required(VERSION 3.12)
	project(${BinName})
endif()

add_executable(${BinName}
	Main.cpp
	model.h
	model.cpp
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
	)

	pico_add_extra_outputs(${BinName})
endif()