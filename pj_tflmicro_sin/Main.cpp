/* Use the following code as  reference:
	https://github.com/raspberrypi/pico-tflmicro/blob/main/examples/hello_world/main_functions.cpp
	Licensed under the Apache License, Version 2.0
*/

#include <cstdint>
#include <cstdio>
// #include <cstdlib>
// #include <cstring>

#ifndef BUILD_ON_PC
#include "pico/stdlib.h"
#endif
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/version.h"
#include "model.h"

constexpr int kTensorArenaSize = 2000;
static uint8_t tensor_arena[kTensorArenaSize];

int main() {
#ifndef BUILD_ON_PC
	stdio_init_all();
#endif
	printf("Hello, world!\n");

	static tflite::MicroErrorReporter micro_error_reporter;
	static tflite::ErrorReporter* error_reporter = &micro_error_reporter;

	const tflite::Model* model = tflite::GetModel(g_model);
	if (model->version() != TFLITE_SCHEMA_VERSION) {
		TF_LITE_REPORT_ERROR(error_reporter,
			"Model provided is schema version %d not equal "
			"to supported version %d.",
			model->version(), TFLITE_SCHEMA_VERSION);
			return -1;
	}

	static tflite::AllOpsResolver resolver;

	// Build an interpreter to run the model with.
	static tflite::MicroInterpreter static_interpreter(model, resolver, tensor_arena, kTensorArenaSize, error_reporter);
	tflite::MicroInterpreter* interpreter = &static_interpreter;

	// Allocate memory from the tensor_arena for the model's tensors.
	TfLiteStatus allocate_status = interpreter->AllocateTensors();
	if (allocate_status != kTfLiteOk) {
		TF_LITE_REPORT_ERROR(error_reporter, "AllocateTensors() failed");
		return -1;
	}

	// Obtain pointers to the model's input and output tensors.
	TfLiteTensor* input = interpreter->input(0);
	TfLiteTensor* output = interpreter->output(0);

	while (true) {
		for (int32_t deg = 0; deg < 360; deg++) {
			float x = deg / 180.0f * 3.141592654;

			// Quantize the input from floating-point to integer
			int8_t x_quantized = x / input->params.scale + input->params.zero_point;
			// Place the quantized input in the model's input tensor
			input->data.int8[0] = x_quantized;

			// Run inference, and report any error
			TfLiteStatus invoke_status = interpreter->Invoke();
			if (invoke_status != kTfLiteOk) {
				TF_LITE_REPORT_ERROR(error_reporter, "Invoke failed on x: %f\n",
									static_cast<double>(x));
				return -1;
			}

			// Obtain the quantized output from model's output tensor
			int8_t y_quantized = output->data.int8[0];
			// Dequantize the output from integer to floating-point
			float y = (y_quantized - output->params.zero_point) * output->params.scale;

			for(int32_t i = 0; i < 10 + static_cast<int32_t>(y * 10); i++) {
				printf(" ");
			}
			printf("*\n");
			// printf("sin(%d) = %.3f\n", deg, y);
		}
	}

	return 0;
}
