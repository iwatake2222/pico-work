#ifndef ADC_BUFFER_H_
#define ADC_BUFFER_H_

#include <cstdint>
#include <functional>
#include "RingBuffer.h"
#include "hardware/adc.h"
#include "hardware/dma.h"
#include "hardware/irq.h"

class AdcBuffer {
public:
	static constexpr int32_t ADC_CLOCK  = (48 * 1000 * 1000);        // Fixed value (48MHz)
	static constexpr int32_t BUFFER_NUM = 4;

	enum {
		RET_OK = 0,
		RET_ERR = -1,
	};

	typedef struct CONFIG_ {
		int32_t captureChannel;
		int32_t captureDepth;
		int32_t samplingRate;
	} CONFIG;

public:
	AdcBuffer() {};
	~AdcBuffer() {}
	int32_t initialize(const CONFIG& config);
	int32_t finalize(void);
	int32_t start(void);
	int32_t stop(void);
	int32_t getBufferSize();
	std::vector<uint8_t>& getBuffer(int32_t next);
	void deleteFront();

public:
	static std::function<void(void)> irqHandlerStatic;
private:
	void irqHandler();

private:
	int32_t m_captureChannel;
	int32_t m_captureDepth;
	int32_t m_samplingRate;
	RingBuffer<uint8_t> m_adcBufferList;
	int32_t m_dmaChannel;
	dma_channel_config m_dmaConfig;
};

#endif
