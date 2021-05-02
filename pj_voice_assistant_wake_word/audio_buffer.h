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

#ifndef AUDIO_BUFFER_H_
#define AUDIO_BUFFER_H_

#include <cstdint>
#include "ring_block_buffer.h"

class AudioBuffer {
public:
    enum {
        kRetOk = 0,
        kRetErr = -1,
    };

    typedef struct {
        int32_t buffer_num;
        int32_t capture_channel;
        int32_t capture_depth;  /* = block size */
        int32_t sampling_rate;
    } Config;

public:
    virtual int32_t Initialize(const Config& config) = 0;
    virtual int32_t Finalize(void) = 0;
    virtual int32_t Start(void) = 0;
    virtual int32_t Stop(void) = 0;
    virtual RingBlockBuffer<uint8_t>& GetRingBlockBuffer(void) = 0;
};

#endif  // AUDIO_BUFFER_H_
