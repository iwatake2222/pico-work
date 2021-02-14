set(BinName "pj_tflmicro_mnist")
add_executable(${BinName}
	Main.cpp
	conv_mnist_quant.h
	conv_mnist_quant.cpp
)

pico_enable_stdio_usb(${BinName} 1)
pico_enable_stdio_uart(${BinName} 1)

target_link_libraries(${BinName} 
	pico_stdlib
	pico-tflmicro
)

pico_add_extra_outputs(${BinName})

