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

#include "test_buffer.h"

#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cmath>

#include "utility_macro.h"
#include "test_audio_data.h"

/*** MACRO ***/
#define TAG "TestBuffer"
#define PRINT(...)   UTILITY_MACRO_PRINT(TAG, __VA_ARGS__)
#define PRINT_E(...) UTILITY_MACRO_PRINT_E(TAG, __VA_ARGS__)

/*** GLOBAL VARIABLE ***/
static std::vector<uint8_t> s_test_data;
static int32_t s_current_test_data_index = 0;

/*** FUNCTION ***/
int32_t TestBuffer::Initialize(const Config& config) {
    /* Set parameters */
    buffer_num_ = config.buffer_num;
    capture_channel_ = config.capture_channel;	// not in use
    capture_depth_ = config.capture_depth;
    sampling_rate_ = config.sampling_rate;

    /* Reset buffer */
    test_block_buffer_.Initialize(buffer_num_, capture_depth_);

    const int32_t kTestDataNum = sizeof(s_testAudioData) / sizeof(int16_t);
    for (int32_t i = 0; i < kTestDataNum; i++) {
        //s_test_data.push_back(i);
        //s_test_data.push_back( (1 + sin((3.14 * i) / 16000.0 * 400)) * 128  );
        s_test_data.push_back(s_testAudioData[i] / 256 + 128);
    }

    return kRetOk;
}

int32_t TestBuffer::Finalize(void) {
    return kRetOk;
}

int32_t TestBuffer::Start(void) {
    return kRetOk;
}

int32_t TestBuffer::Stop(void) {
    return kRetOk;
}


RingBlockBuffer<uint8_t>& TestBuffer::GetRingBlockBuffer(void) {
    return test_block_buffer_;
}

void TestBuffer::DebugWriteData(int32_t duration_ms) {
    const int32_t kUpdateNum = duration_ms * sampling_rate_ / 1000 / capture_depth_;
    const int32_t kTestDataNum = static_cast<int32_t>(s_test_data.size());

    if (s_current_test_data_index == kTestDataNum) return;  // do not write debug data exceed prepared test data

    for (int32_t update_count = 0; update_count < kUpdateNum; update_count++) {
        uint8_t* p = test_block_buffer_.WritePtr();
        if (p == nullptr) {
            //printf("AdcBuffer: overflow\n");
            //p = m_testBufferList.getLatestWritePtr();
            break;
        }

        for (int32_t i = 0; i < capture_depth_; i++) {
            p[i] = s_test_data[s_current_test_data_index];
            s_current_test_data_index++;
            if (s_current_test_data_index == kTestDataNum) return;
            //s_current_test_data_index %= SIZE_DATA_NUM;
        }
    }
}
