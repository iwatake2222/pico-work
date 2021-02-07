set(BinName "pj_base")
add_executable(${BinName}
	Main.cpp
)

pico_enable_stdio_usb(${BinName} 1)
pico_enable_stdio_uart(${BinName} 0)

target_link_libraries(${BinName} pico_stdlib hardware_i2c)
pico_add_extra_outputs(${BinName})

