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

#include "audio_provider.h"

#include <cstdint>
#include <cstdlib>
#include <cstdio>

#include "utility_macro.h"
#include "test_buffer.h"


/*** MACRO ***/
#ifdef BUILD_ON_PC
#define USE_TEST_BUFFER
#endif

#define TAG "AudioProvider"
#define PRINT(...)   UTILITY_MACRO_PRINT(TAG, __VA_ARGS__)
#define PRINT_E(...) UTILITY_MACRO_PRINT_E(TAG, __VA_ARGS__)

/*** GLOBAL VARIABLE ***/

/*** FUNCTION ***/
int32_t AudioProvider::Initialize() {
    time_ms_at_index0_ = 0;
    valid_data_num_ = 0;

#ifdef USE_TEST_BUFFER
    audio_buffer_ = std::unique_ptr<AudioBuffer>(new TestBuffer());
#else
    audio_buffer_ = std::unique_ptr<AudioBuffer>(new AdcBuffer());
#endif
    if (!audio_buffer_) {
        PRINT_E("AudioBuffer null\n");
        return kRetErr;
    }

    AudioBuffer::Config audio_buffer_config;
    audio_buffer_config.buffer_num = kBufferSize;
    audio_buffer_config.capture_channel = 0;
    audio_buffer_config.capture_depth = kBlockSize;
    audio_buffer_config.sampling_rate = kSamplingRate;
    if (audio_buffer_->Initialize(audio_buffer_config) != AudioBuffer::kRetOk) {
        PRINT_E("AudioBuffer Initialize\n");
        return kRetErr;
    }
    if (audio_buffer_->Start() != AudioBuffer::kRetOk) {
        PRINT_E("AudioBuffer Start\n");
        return kRetErr;
    }

    return kRetOk;
}

int32_t AudioProvider::Finalize() {
    if (audio_buffer_->Stop() != AudioBuffer::kRetOk) {
        PRINT_E("AudioBuffer Stop\n");
        return kRetErr;
    }
    if (audio_buffer_->Finalize() != AudioBuffer::kRetOk) {
        PRINT_E("AudioBuffer Finalize\n");
        return kRetErr;
    }
    return kRetOk;
}

int32_t AudioProvider::GetAudioSamples(
    int32_t start_time_ms, int32_t duration_time_ms,
    int32_t* audio_samples_size, int16_t** audio_samples) {
    auto& ring_buffer = audio_buffer_->GetRingBlockBuffer();

    int32_t time_ms_at_end = time_ms_at_index0_ + valid_data_num_ / kSamplePerMs;
    if (start_time_ms < time_ms_at_index0_) {
        /* target data is not recorded yet */
    } else if (time_ms_at_index0_ <= start_time_ms && start_time_ms < time_ms_at_end) {
        /* (a part of) target data is already stored in the local buffer */
        /*   1. remove unnecessary data and move the target data to the beginning of the local buffer */
        int32_t start_index_local_buffer = (start_time_ms - time_ms_at_index0_) * kSamplePerMs;
        time_ms_at_index0_ = start_time_ms;
        valid_data_num_ -= start_index_local_buffer;
        memmove(&local_buffer_[0], &local_buffer_[start_index_local_buffer], valid_data_num_ * sizeof(int16_t));

        /*   2. copy the rest data from the ring buffer with format conversion (if there is data in the ring buffer and if there is enough space in the local buffer */
        if (!ring_buffer.IsUnderflow() && valid_data_num_ <= kBlockSize) {
            const auto& block = ring_buffer.Read();
            for (const auto& data : block) {
                if (valid_data_num_ >= kBlockSize * 2) {
                    PRINT("[AudioProvider::initialize] shouldn't reach here\n");
                    break;
                }
                local_buffer_[valid_data_num_] = (static_cast<int16_t>(data) - 128) * 256;	// uint8_t (0 - 255) -> int16_t (-32768 - 32767)
                //local_buffer_[valid_data_num_] = (static_cast<int16_t>(data) - 0) * 1;	// uint8_t (0 - 255) -> int16_t (-32768 - 32767)
                valid_data_num_++;
            }
        }
    } else {
        /* target data is not stored in the local buffer yet */
        valid_data_num_ = 0;
        while (!ring_buffer.IsUnderflow() && valid_data_num_ <= kBlockSize) {
            int32_t time_rp_ms = ring_buffer.accumulated_read_data_num() * kDurationPerBlock;
            if (time_rp_ms <= start_time_ms && start_time_ms < time_rp_ms + kDurationPerBlock) {
                const auto& block = ring_buffer.Read();
                for (const auto& data : block) {
                    if (valid_data_num_ >= kBlockSize * 2) {
                        PRINT("[AudioProvider::initialize] shouldn't reach here\n");
                        break;
                    }
                    local_buffer_[valid_data_num_] = (static_cast<int16_t>(data) - 128) * 256;	// uint8_t (0 - 255) -> int16_t (-32768 - 32767)
                    //local_buffer_[valid_data_num_] = (static_cast<int16_t>(data) - 0) * 1;	// uint8_t (0 - 255) -> int16_t (-32768 - 32767)
                    valid_data_num_++;
                }
                time_ms_at_index0_ = time_rp_ms;
                GetAudioSamples(start_time_ms, duration_time_ms, audio_samples_size, audio_samples);    // todo: better to avoid recursive call
                break;
            }
        }
    }

    *audio_samples_size = valid_data_num_;
    *audio_samples = local_buffer_;

    return kRetOk;
}

int32_t AudioProvider::GetLatestAudioTimestamp() {
    auto& ring_buffer = audio_buffer_->GetRingBlockBuffer();
    int32_t time_wp_ms = ring_buffer.accumulated_stored_data_num();
    time_wp_ms -= 1;	// use the beginning time of the block (to work with feature_privider logic to calculate slices_needed)
    time_wp_ms *= kDurationPerBlock;
    return time_wp_ms;
}

void AudioProvider::DebugWriteData(int32_t updated_time_duration) {
#ifdef USE_TEST_BUFFER
    dynamic_cast<TestBuffer*>(audio_buffer_.get())->DebugWriteData(updated_time_duration);
#endif
}

void testAudioProvider(void) {
    AudioProvider audio_provider;
    audio_provider.Initialize();

    for (int32_t start_time = 0; ; start_time += 20) {
        audio_provider.DebugWriteData(100);

        int32_t audio_samples_size;
        int16_t* audio_samples;

        audio_provider.GetLatestAudioTimestamp();
        audio_provider.GetAudioSamples(start_time, 30, &audio_samples_size, &audio_samples);
        PRINT("%d: %d\n", start_time, audio_samples[0]);
        if (audio_samples_size < 512) {
            PRINT_E("audio_samples_size = %d\n", audio_samples_size);
            //HALT();
        } else {
            for (int32_t i = 0; i < 512; i++) {
                int16_t expected_value = (start_time * 16 + i) % 16000;
                expected_value = (expected_value - 128) * 256;
                if (audio_samples[i] != expected_value) {
                    PRINT_E("audio_samples = %d\n", audio_samples[i]);
                    HALT();
                }
            }
        }
    }
}
