/* Copyright 2021 iwatake2222

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#ifndef UTILITY_MACRO_H_
#define UTILITY_MACRO_H_

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#ifndef BUILD_ON_PC
#include "pico/stdlib.h"
#endif


#ifdef BUILD_ON_PC
#define HALT() do { exit(1); } while(0);
#define UTILITY_MACRO_PRINT_(...) printf(__VA_ARGS__)
#else
#define HALT() do { while(1) sleep_ms(100); } while(0);
//#define PRINT() do {  }while(0)
#define UTILITY_MACRO_PRINT_(...) printf(__VA_ARGS__)
#endif

#define UTILITY_MACRO_PRINT(UTILITY_MACRO_PRINT_TAG, ...) do { \
	UTILITY_MACRO_PRINT_("[" UTILITY_MACRO_PRINT_TAG "][%d] ", __LINE__); \
	UTILITY_MACRO_PRINT_(__VA_ARGS__); \
} while(0);

#define UTILITY_MACRO_PRINT_E(UTILITY_MACRO_PRINT_TAG, ...) do { \
	UTILITY_MACRO_PRINT_("[ERR: " UTILITY_MACRO_PRINT_TAG "][%d] ", __LINE__); \
	UTILITY_MACRO_PRINT_(__VA_ARGS__); \
} while(0);


#endif  // UTILITY_MACRO_H_

