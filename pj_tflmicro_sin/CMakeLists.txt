set(BinName "pj_tflmicro_sin")
add_executable(${BinName}
	Main.cpp
	Model.h
	Model.cpp
)

pico_enable_stdio_usb(${BinName} 1)
pico_enable_stdio_uart(${BinName} 1)

target_link_libraries(${BinName} 
	pico_stdlib
	pico-tflmicro
)

pico_add_extra_outputs(${BinName})

