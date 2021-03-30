#include <cstdint>
#include <cstdio>
#include <deque>
// #include <cstdlib>
// #include <cstring>

#include "pico/stdlib.h"
#include "LcdIli9341SPI.h"
#include "TpTsc2046SPI.h"
#include "AdcBuffer.h"

#include "pico/multicore.h"


static constexpr std::array<uint8_t, 2> COLOR_BG = { 0x00, 0x00 };
static constexpr std::array<uint8_t, 2> COLOR_LINE = { 0xF8, 0x00 };

#ifndef BUILD_ON_PC
#define HALT() do{while(1) sleep_ms(100);}while(0)
#define PRINT_TIME() do{printf("TIME: %d [ms]\n", to_ms_since_boot(get_absolute_time()));}while(0)
#else
#define HALT() do{}while(0)
#define PRINT_TIME() do{}while(0)
#endif

static void reset(LcdIli9341SPI& lcd);

int main() {
	stdio_init_all();
	printf("Hello, world!\n");

	LcdIli9341SPI::CONFIG lcdConfig;
	lcdConfig.spiPortNum = 0;
	lcdConfig.pinSck = 2;
	lcdConfig.pinMosi = 3;
	lcdConfig.pinMiso = 4;
	lcdConfig.pinCs = 5;
	lcdConfig.pinDc = 7;
	lcdConfig.pinReset = 6;
	LcdIli9341SPI lcd;
	lcd.initialize(lcdConfig);
	lcd.test();

	TpTsc2046SPI::CONFIG tpConfig;
	tpConfig.spiPortNum = 1;
	tpConfig.pinSck = 10;
	tpConfig.pinMosi = 11;
	tpConfig.pinMiso = 12;
	tpConfig.pinCs = 13;
	tpConfig.pinIrq = 14;
	tpConfig.callback = nullptr;
	TpTsc2046SPI tp;
	tp.initialize(tpConfig);

	AdcBuffer::CONFIG adcConfig;
	adcConfig.captureChannel = 0;
	adcConfig.captureDepth = LcdIli9341SPI::WIDTH;
	adcConfig.samplingRate = 20000;
	AdcBuffer adcBuffer;
	adcBuffer.initialize(adcConfig);
	
	reset(lcd);
	adcBuffer.start();
	while(1) {
		// printf("%d\n", adcBuffer.getBufferSize());
		if (adcBuffer.getBufferSize() > 2) {
			auto& adcBufferPrevious = adcBuffer.getBuffer(0);
			auto& adcBufferLatest = adcBuffer.getBuffer(1);
			for (int32_t i = 0; i < adcBufferPrevious.size(); i++) {
				lcd.drawRect(i, (adcBufferPrevious[i] * LcdIli9341SPI::HEIGHT) / 256, 2, 2, COLOR_BG);
			}
			for (int32_t i = 1; i < adcBufferLatest.size(); i++) {
				lcd.drawRect(i, (adcBufferLatest[i] * LcdIli9341SPI::HEIGHT) / 256, 2, 2, COLOR_LINE);
			}
			adcBuffer.deleteFront();
		} else {
			sleep_ms(1);
		}
	}

	adcBuffer.stop();
	adcBuffer.finalize();
	lcd.finalize();
	tp.finalize();

	return 0;
}

static void reset(LcdIli9341SPI& lcd)
{
	std::array<uint8_t, 2> colorBg = { 0x00, 0x1F };
	lcd.drawRect(0, 0, LcdIli9341SPI::WIDTH, LcdIli9341SPI::HEIGHT, COLOR_BG);
	lcd.setCharPos(0, 0);
}
