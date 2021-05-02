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

#ifndef ADC_BUFFER_H_
#define ADC_BUFFER_H_

#include <cstdint>
#include <functional>

#include "hardware/adc.h"
#include "hardware/dma.h"
#include "hardware/irq.h"

#include "ring_block_buffer.h"
#include "audio_buffer.h"

class AdcBuffer : public AudioBuffer {
public:
    static constexpr int32_t ADC_CLOCK  = (48 * 1000 * 1000);        // Fixed value (48MHz)
    static constexpr int32_t BUFFER_NUM = 4;

public:
    AdcBuffer()
        : buffer_num_(0)
        , capture_channel_(0)
        , capture_depth_(0)
        , sampling_rate_(0)
        , dma_channel_(0)
     {};
    ~AdcBuffer() {}

    int32_t Initialize(const Config& config) override;
    int32_t Finalize(void) override;
    int32_t Start(void) override;
    int32_t Stop(void) override;
    RingBlockBuffer<uint8_t>& GetRingBlockBuffer(void) override;

public:
    static std::function<void(void)> irq_handler_static_;

private:
    void IrqHandler();

private:
    int32_t buffer_num_;
    int32_t capture_channel_;
    int32_t capture_depth_;
    int32_t sampling_rate_;
    RingBlockBuffer<uint8_t> adc_block_buffer_;
    dma_channel_config dma_config_;
    int32_t dma_channel_;
};

#endif
