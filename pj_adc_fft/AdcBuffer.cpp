#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include "pico/stdlib.h"
#include "AdcBuffer.h"

/*** CONST VALUE ***/

/*** MACRO ***/
#ifndef BUILD_ON_PC
#define HALT() do{while(1) sleep_ms(100);}while(0)
#define PRINT_TIME() do{printf("TIME: %d [ms]\n", to_ms_since_boot(get_absolute_time()));}while(0)
#else
#define HALT() do{}while(0)
#define PRINT_TIME() do{}while(0)
#endif

/*** FUNCTION ***/

/*** GLOBAL VARIABLE ***/
std::function<void(void)> AdcBuffer::irqHandlerStatic;

void dma_handler()
{
	AdcBuffer::irqHandlerStatic();
}

void AdcBuffer::irqHandler()
{
	/* Clear the interrupt request. */
	dma_hw->ints0 = 1u << m_dmaChannel;
	// PRINT_TIME();
	// printf("dma_handler\n");

	/* Restart DMS */
	uint8_t* p = m_adcBufferList.writePtr();
	if (p == nullptr) {
		// printf("AdcBuffer: overflow\n");
		p = m_adcBufferList.getLatestWritePtr();
	}
	dma_channel_configure(m_dmaChannel, &m_dmaConfig,
		p,              // dst
		&adc_hw->fifo,  // src
		m_captureDepth, // transfer count
		true            // start immediately
	);

}


int32_t AdcBuffer::initialize(const CONFIG& config)
{
	irqHandlerStatic = [this] { irqHandler(); };
	/* Set parameters */
	m_captureChannel = config.captureChannel;
	m_captureDepth = config.captureDepth;
	m_samplingRate = config.samplingRate;

	/* Reset buffer */
	m_adcBufferList.initialize(BUFFER_NUM, m_captureDepth);


	/* Initialize ADC */
	adc_init();
	adc_gpio_init(26 + m_captureChannel);
	adc_select_input(m_captureChannel);
	adc_fifo_setup(
		true,    // Write each completed conversion to the sample FIFO
		true,    // Enable DMA data request (DREQ)
		1,       // DREQ (and IRQ) asserted when at least 1 sample present
		false,   // We won't see the ERR bit because of 8 bit reads; disable.
		true     // Shift each sample to 8 bits when pushing to FIFO
	);
	adc_set_clkdiv(ADC_CLOCK / m_samplingRate);

	/* Initialize DMA */
	// Set up the DMA to start transferring data as soon as it appears in FIFO
	m_dmaChannel = dma_claim_unused_channel(true);
	m_dmaConfig = dma_channel_get_default_config(m_dmaChannel);

	// Reading from constant address, writing to incrementing byte addresses
	channel_config_set_transfer_data_size(&m_dmaConfig, DMA_SIZE_8);
	channel_config_set_read_increment(&m_dmaConfig, false);
	channel_config_set_write_increment(&m_dmaConfig, true);

	// Pace transfers based on availability of ADC samples
	channel_config_set_dreq(&m_dmaConfig, DREQ_ADC);

	/* Initialize IRQ for DMA */
	/* Manually call IRQ handler to config DMA */
	dma_handler();

	dma_channel_set_irq0_enabled(m_dmaChannel, true);
	irq_set_exclusive_handler(DMA_IRQ_0 + m_dmaChannel, dma_handler);
	irq_set_enabled(DMA_IRQ_0 + m_dmaChannel, true);

	return RET_OK;
}

int32_t AdcBuffer::finalize(void)
{
	return RET_OK;
}

int32_t AdcBuffer::start(void)
{
	adc_run(true);
	return RET_OK;
}

int32_t AdcBuffer::stop(void)
{
	dma_channel_wait_for_finish_blocking(m_dmaChannel);
	adc_run(false);
	adc_fifo_drain();
	return RET_OK;
}

int32_t AdcBuffer::getBufferSize()
{
	return m_adcBufferList.getStoredDataNum();
}

std::vector<uint8_t>& AdcBuffer::getBuffer(int32_t next)
{
	return m_adcBufferList.refer(next);
}

void AdcBuffer::deleteFront()
{
	(void)m_adcBufferList.read();
}
