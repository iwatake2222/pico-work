#ifndef ADC_BUFFER_H_
#define ADC_BUFFER_H_

#include <cstdint>
#include <vector>
#include <deque>
#include <functional>
#include "hardware/adc.h"
#include "hardware/dma.h"
#include "hardware/irq.h"

/*** Buffer structure
 *   basic
 *   |----------------------------------|
 *   | buffer[0]: data of CAPTURE_DEPTH |  <--- Read then delete (The oldest data but not read yet)
 *   | buffer[1]: data of CAPTURE_DEPTH |
 *   | buffer[2]: data of CAPTURE_DEPTH |
 *   | buffer[3]: data of CAPTURE_DEPTH |  <--- Write from ADC via DMA (The latest data)
 *   |----------------------------------|
 * 
 *   actual use
 *   |----------------------------------|
 *   | buffer[0]: data of CAPTURE_DEPTH |  <--- UI: 3. Delete
 *   | buffer[1]: data of CAPTURE_DEPTH |
 *   | buffer[2]: data of CAPTURE_DEPTH |  <--- UI: 2. Read as the previous data,  FFT: 1. Read as the latest data
 *   | buffer[3]: data of CAPTURE_DEPTH |  <--- UI: 1. Read as the latest data
 *   | buffer[4]: data of CAPTURE_DEPTH |
 *   | buffer[5]: data of CAPTURE_DEPTH |  <--- Write from ADC via DMA (The latest data)
 *   | buffer[6]: data of CAPTURE_DEPTH |
 *   | buffer[7]: data of CAPTURE_DEPTH |
 *   |----------------------------------|
 ***/

class AdcBuffer {
public:
	static constexpr int32_t ADC_CLOCK  = (48 * 1000 * 1000);        // Fixed value (48MHz)
	static constexpr int32_t BUFFER_NUM = 8;

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
	int32_t startNext(void);
	int32_t stop(void);
	int32_t getBufferSize();
	std::vector<uint8_t>& getBuffer(int32_t index);
	void deleteFront();
	bool isOverflow();

public:
	static std::function<void(void)> irqHandlerStatic;
private:
	void irqHandler();

private:
	int32_t m_captureChannel;
	int32_t m_captureDepth;
	int32_t m_samplingRate;
	std::deque<std::vector<uint8_t>> m_adcBufferList;
	int32_t m_dmaChannel;
	dma_channel_config m_dmaConfig;
	bool m_adcDone;
};

#endif
