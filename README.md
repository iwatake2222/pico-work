# About
Playground for Raspberry Pi Pico

## Build
```
git clone https://github.com/iwatake2222/pico-work.git
cd pico-work
# git submodule update --init
git submodule update --init --recursive
mkdir build && cd build

# For Windows
cmake .. -G "NMake Makefiles"
cd pj_base
nmake
```

*Note* : It's important to clone TinyUSB submodule. Otherwise, you cannot use USB including UART on USB CDC. (Build succeeds even without TinyUSB although you will get warning on cmake `TinyUSB submodule has not been initialized; USB support will be unavailable` )

## Acknowledgements
- pico-sdk
	- Copyright 2020 (c) 2020 Raspberry Pi (Trading) Ltd.
