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
#include "adc_buffer.h"

#include <cstdint>
#include <cstdlib>
#include <cstdio>

#include "pico/stdlib.h"

#include "utility_macro.h"

/*** MACRO ***/
#define TAG "AdcBuffer"
#define PRINT(...)   UTILITY_MACRO_PRINT(TAG, __VA_ARGS__)
#define PRINT_E(...) UTILITY_MACRO_PRINT_E(TAG, __VA_ARGS__)

/*** FUNCTION ***/

/*** GLOBAL VARIABLE ***/
std::function<void(void)> AdcBuffer::irq_handler_static_;

static void dma_handler() {
    AdcBuffer::irq_handler_static_();
}

void AdcBuffer::IrqHandler() {
    /* Clear the interrupt request. */
    dma_hw->ints0 = 1u << dma_channel_;
    // PRINT_TIME();
    // PRINT("dma_handler\n");

    /* Restart DMS */
    uint8_t* p = adc_block_buffer_.WritePtr();
    if (p == nullptr) {
        PRINT_E("AdcBuffer: overflow\n");
        p = adc_block_buffer_.GetLatestWritePtr();
    }
    dma_channel_configure(dma_channel_, &dma_config_,
        p,              // dst
        &adc_hw->fifo,  // src
        capture_depth_, // transfer count
        true            // start immediately
    );

}


int32_t AdcBuffer::Initialize(const Config& config) {
    irq_handler_static_ = [this] { IrqHandler(); };
    /* Set parameters */
    buffer_num_ = config.buffer_num;
    capture_channel_ = config.capture_channel;
    capture_depth_ = config.capture_depth;
    sampling_rate_ = config.sampling_rate;

    /* Reset buffer */
    adc_block_buffer_.Initialize(buffer_num_, capture_depth_);

    /* Initialize ADC */
    adc_init();
    adc_gpio_init(26 + capture_channel_);
    adc_select_input(capture_channel_);
    adc_fifo_setup(
        true,    // Write each completed conversion to the sample FIFO
        true,    // Enable DMA data request (DREQ)
        1,       // DREQ (and IRQ) asserted when at least 1 sample present
        false,   // We won't see the ERR bit because of 8 bit reads; disable.
        true     // Shift each sample to 8 bits when pushing to FIFO
    );
    adc_set_clkdiv(ADC_CLOCK / sampling_rate_);

    /* Initialize DMA */
    // Set up the DMA to start transferring data as soon as it appears in FIFO
    dma_channel_ = dma_claim_unused_channel(true);
    dma_config_ = dma_channel_get_default_config(dma_channel_);

    // Reading from constant address, writing to incrementing byte addresses
    channel_config_set_transfer_data_size(&dma_config_, DMA_SIZE_8);
    channel_config_set_read_increment(&dma_config_, false);
    channel_config_set_write_increment(&dma_config_, true);

    // Pace transfers based on availability of ADC samples
    channel_config_set_dreq(&dma_config_, DREQ_ADC);

    /* Initialize IRQ for DMA */
    /* Manually call IRQ handler to config DMA */
    dma_handler();

    dma_channel_set_irq0_enabled(dma_channel_, true);
    irq_set_exclusive_handler(DMA_IRQ_0 + dma_channel_, dma_handler);
    irq_set_enabled(DMA_IRQ_0 + dma_channel_, true);

    return kRetOk;
}

int32_t AdcBuffer::Finalize(void) {
    return kRetOk;
}

int32_t AdcBuffer::Start(void) {
    adc_run(true);
    return kRetOk;
}

int32_t AdcBuffer::Stop(void) {
    dma_channel_wait_for_finish_blocking(dma_channel_);
    adc_run(false);
    adc_fifo_drain();
    return kRetOk;
}

RingBlockBuffer<uint8_t>& AdcBuffer::GetRingBlockBuffer(void) {
    return adc_block_buffer_;
}
