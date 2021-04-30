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

#ifndef AUDIO_PROVIDER_H_
#define AUDIO_PROVIDER_H_

#include <cstdint>
#include <cstring>
#include <memory>

#include "audio_buffer.h"

class AudioProvider
{
public:
    enum {
        kRetOk = 0,
        kRetErr = -1,
    };

private:
    static constexpr int32_t kSamplingRate = 16000;
    static constexpr int32_t kBlockSize = 512;
    static constexpr int32_t kBufferSize = 10;
    static constexpr int32_t kSamplePerMs = kSamplingRate / 1000;           // 16 sample = 1msec
    static constexpr int32_t kDurationPerBlock = kBlockSize / kSamplePerMs; // 32msec

public:
    AudioProvider()
        : audio_buffer_(nullptr)
        , time_ms_at_index0_(0)
        , valid_data_num_(0) {
        memset(local_buffer_, 0, sizeof(local_buffer_));
    }
    ~AudioProvider() {}

    int32_t Initialize();
    int32_t Finalize();
    int32_t GetAudioSamples(
        int32_t start_time_ms,
        int32_t duration_time_ms,
        int32_t* audio_samples_size,
        int16_t** audio_samples);
    int32_t GetLatestAudioTimestamp();
    void DebugWriteData(int32_t updated_time_duration);

private:
    std::unique_ptr<AudioBuffer> audio_buffer_;
    int16_t local_buffer_[kBlockSize * 2];
    int32_t time_ms_at_index0_;
    int32_t valid_data_num_;
};

#endif  // AUDIO_PROVIDER_H_
