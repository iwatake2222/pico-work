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

/*** INCLUDE ***/
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <algorithm>

#ifndef BUILD_ON_PC
#include "pico/stdlib.h"
#endif

#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/version.h"
#include "micro_features/model.h"
#include "micro_features/yes_micro_features_data.h"
#include "micro_features/no_micro_features_data.h"
#include "micro_features/micro_model_settings.h"
#include "micro_features/model.h"
#include "feature_provider.h"

#include "utility_macro.h"
#include "audio_provider.h"
#include "majority_vote.h"
#include "LcdIli9341SPI.h"
#include "TpTsc2046SPI.h"

/*** MACRO ***/
#define TAG "main"
#define PRINT(...)   UTILITY_MACRO_PRINT(TAG, __VA_ARGS__)
#define PRINT_E(...) UTILITY_MACRO_PRINT_E(TAG, __VA_ARGS__)

/*** GLOBAL_VARIABLE ***/
static tflite::MicroErrorReporter micro_error_reporter;
static tflite::ErrorReporter* error_reporter = &micro_error_reporter;

/*** FUNCTION ***/
static tflite::MicroInterpreter* createStaticInterpreter(void)
{
    constexpr int32_t kTensorArenaSize = 1088 + 24 + 5968;
    static uint8_t tensor_arena[kTensorArenaSize];
    const tflite::Model* model = tflite::GetModel(g_model);
    if (model->version() != TFLITE_SCHEMA_VERSION) {
        PRINT_E("Model provided is schema version %d not equal to supported version %d.", model->version(), TFLITE_SCHEMA_VERSION);
        return nullptr;
    }

    static tflite::AllOpsResolver resolver;
    static tflite::MicroInterpreter static_interpreter(model, resolver, tensor_arena, kTensorArenaSize, error_reporter);
    tflite::MicroInterpreter* interpreter = &static_interpreter;
    TfLiteStatus allocate_status = interpreter->AllocateTensors();
    if (allocate_status != kTfLiteOk) {
        PRINT_E("AllocateTensors() failed");
        return nullptr;
    }

    TfLiteTensor* input = interpreter->input(0);
    TfLiteTensor* output = interpreter->output(0);
    if (input->bytes != kFeatureElementCount) {
        PRINT_E("Input size seems wrong: %d\n", static_cast<int32_t>(input->bytes));
        return nullptr;
    }

    if (output->bytes != kCategoryCount) {
        PRINT_E("Output size seems wrong: %d\n", static_cast<int32_t>(output->bytes));
        return nullptr;
    }

    return interpreter;
}

static LcdIli9341SPI& createStaticLcd(void)
{
    static LcdIli9341SPI lcd;
    LcdIli9341SPI::CONFIG lcdConfig;
    lcdConfig.spiPortNum = 0;
    lcdConfig.pinSck = 2;
    lcdConfig.pinMosi = 3;
    lcdConfig.pinMiso = 4;
    lcdConfig.pinCs = 5;
    lcdConfig.pinDc = 7;
    lcdConfig.pinReset = 6;
    lcd.initialize(lcdConfig);
    lcd.test();
    return lcd;
}

static TpTsc2046SPI& createStaticTp(void)
{
    static TpTsc2046SPI tp;
    TpTsc2046SPI::CONFIG tpConfig;
    tpConfig.spiPortNum = 1;
    tpConfig.pinSck = 10;
    tpConfig.pinMosi = 11;
    tpConfig.pinMiso = 12;
    tpConfig.pinCs = 13;
    tpConfig.pinIrq = 14;
    tpConfig.callback = nullptr;
    tp.initialize(tpConfig);
    return tp;
}

static void reset(LcdIli9341SPI& lcd)
{
    static constexpr std::array<uint8_t, 2> COLOR_BG = { 0x00, 0x00 };
    lcd.drawRect(0, 0, LcdIli9341SPI::WIDTH, LcdIli9341SPI::HEIGHT, COLOR_BG);
    lcd.setCharPos(0, 0);
}

int main(void) {
    /*** Initilization ***/
    /* Initialize system */
#ifndef BUILD_ON_PC
    stdio_init_all();
    sleep_ms(1000);		// wait until UART connected
#endif
    PRINT("Hello, world!\n");

    /* Initialize device */
    LcdIli9341SPI& lcd = createStaticLcd();
    TpTsc2046SPI& tp = createStaticTp();
    reset(lcd);

    /* Create interpreter */
    tflite::MicroInterpreter* interpreter = createStaticInterpreter();
    if (!interpreter) {
        PRINT_E("createStaticInterpreter failed\n");
        HALT();
    }
    TfLiteTensor* input = interpreter->input(0);
    TfLiteTensor* output = interpreter->output(0);

    /* Create feature provider */
    static int8_t feature_buffer[kFeatureElementCount];
    static FeatureProvider feature_provider(kFeatureElementCount, feature_buffer);
    static AudioProvider audio_provider;
    audio_provider.Initialize();
    int32_t previous_time = 0;

    /* Create majority vote to remove noise from the result (use int8 to avoid unnecessary dequantization (calculation)) */
    //MajorityVote<float> majority_vote;
    MajorityVote<int32_t> majority_vote;

    /*** Main loop ***/
    while (1) {
        /* Generate feature */
        audio_provider.DebugWriteData(32);
        const int32_t current_time = audio_provider.GetLatestAudioTimestamp();
        if (current_time < 0 || current_time == previous_time) continue;

        int32_t how_many_new_slices = 0;
        TfLiteStatus feature_status = feature_provider.PopulateFeatureData(&audio_provider, error_reporter, previous_time, current_time, &how_many_new_slices);
        if (feature_status != kTfLiteOk) {
            /* It may reach here when underflow happens */
            PRINT_E("Feature generation failed\n");
            // HALT();
        }
        previous_time = current_time;
        if (how_many_new_slices == 0) continue;

        /* Copy the generated feature data to input tensor buffer */
        for (int32_t i = 0; i < kFeatureElementCount; i++) {
            input->data.int8[i] = feature_buffer[i];
        }

        /* Run inference */
        TfLiteStatus invoke_status = interpreter->Invoke();
        if (invoke_status != kTfLiteOk) {
            PRINT_E("Invoke failed\n");
            HALT();
        }

        /*** Show result ***/
        /* Current result */
        int8_t* y_quantized = output->data.int8;
        std::array<int32_t, kCategoryCount> current_score_list;
        for (int32_t i = 0; i < kCategoryCount; i++) {
            current_score_list[i] = y_quantized[i];
            // float y = (y_quantized[i] - output->params.zero_point) * output->params.scale;
            // if (y > 0.8 && (i != 0 && i != 1)) {
            //     PRINT("%s: %f\n", kCategoryLabels[i], y);
            // }
        }
        // PRINT("----\n");

        /* Average result, and check if the average result updated */
        static int32_t s_previous_first_index = -1;
        int32_t first_index = -1;
        int32_t score;
        majority_vote.vote(current_score_list, first_index, score);
        float score_dequantized = (score - output->params.zero_point) * output->params.scale;
        if (score_dequantized > 0.7 && (first_index != 0 && first_index != 1)) {
            PRINT("%s: %f\n", kCategoryLabels[first_index], score_dequantized);
            if (s_previous_first_index != first_index) {
                s_previous_first_index = first_index;   // new label
            } else {
                first_index = -1;   // the same as the previous
            }
        } else {
            // PRINT("%s: %f\n", "unknown", 1 - score_dequantized);
            first_index = -1;   // not recognized
        }
        // PRINT("--------\n");

        /* Display the recognized label */
        if (first_index != -1) {
            lcd.setCharPos(100, 100);
            lcd.putText(kCategoryLabels[first_index]);
        }

        /* Display feature data */
        static std::vector<uint8_t> buffer(kFeatureElementCount * 2, 0);
        for (int i = 0; i < kFeatureElementCount; i++ ) {
            buffer[2 * i] = feature_buffer[i];
        }
        lcd.drawBuffer(10, 10, kFeatureSliceSize, kFeatureSliceCount, buffer);
    }

    /*** Finalization ***/
    lcd.finalize();
    tp.finalize();
    audio_provider.Finalize();

    return 0;
}
