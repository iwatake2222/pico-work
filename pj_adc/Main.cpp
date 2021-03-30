#include <cstdint>
#include <cstdio>
#define _USE_MATH_DEFINES
#include <cmath>
#include <deque>
// #include <cstdlib>
// #include <cstring>

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "LcdIli9341SPI.h"
#include "TpTsc2046SPI.h"
#include "AdcBuffer.h"

/*** CONST VALUE ***/
static constexpr std::array<uint8_t, 2> COLOR_BG = { 0x00, 0x00 };
static constexpr std::array<uint8_t, 2> COLOR_LINE = { 0xF8, 0x00 };
static constexpr int32_t SCALE = 2;
static constexpr int32_t SCALE_FFT = 20;
static constexpr int32_t BUFFER_SIZE = 256;	// 2^x

/*** MACRO ***/
#ifndef BUILD_ON_PC
#define HALT() do{while(1) sleep_ms(100);}while(0)
#define PRINT_TIME() do{printf("TIME: %d [ms]\n", to_ms_since_boot(get_absolute_time()));}while(0)
#else
#define HALT() do{}while(0)
#define PRINT_TIME() do{}while(0)
#endif

/*** FUNCTION ***/
void core1_main();
static void reset(LcdIli9341SPI& lcd);

/*** GLOBAL VARIABLE ***/
AdcBuffer adcBuffer;
std::deque<std::vector<float>> g_fftResultList;

int main() {
	stdio_init_all();
	sleep_ms(1000);	// wait until UART connected
	printf("Hello, world!\n");

	multicore_launch_core1(core1_main);

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
	// adcConfig.captureDepth = LcdIli9341SPI::WIDTH;
	adcConfig.captureDepth = BUFFER_SIZE;
	adcConfig.samplingRate = 20000;
	adcBuffer.initialize(adcConfig);
	
	reset(lcd);
	adcBuffer.start();
	while(1) {
		// PRINT_TIME();
		// printf("%d\n", adcBuffer.getBufferSize());
		if (adcBuffer.getBufferSize() > 2) {  // buffer size needs to be 3 (when the size is 2, data may be not filled yet)
			/* Display wave */
			auto& adcBufferPrevious = adcBuffer.getBuffer(0);
			auto& adcBufferLatest = adcBuffer.getBuffer(1);
			for (int32_t i = 1; i < adcBufferPrevious.size(); i++) {
				// lcd.drawRect(i, (adcBufferPrevious[i] / 256.0 - 0.5) * SCALE * LcdIli9341SPI::HEIGHT + LcdIli9341SPI::HEIGHT / 2, 2, 2, COLOR_BG);
				lcd.drawLine(
					i - 1, (adcBufferPrevious[i - 1] / 256.0 - 0.5) * SCALE * LcdIli9341SPI::HEIGHT + LcdIli9341SPI::HEIGHT / 2,
					i, (adcBufferPrevious[i] / 256.0 - 0.5) * SCALE * LcdIli9341SPI::HEIGHT + LcdIli9341SPI::HEIGHT / 2,
					1, COLOR_BG);
			}
			for (int32_t i = 1; i < adcBufferLatest.size(); i++) {
				// lcd.drawRect(i, (adcBufferLatest[i] / 256.0 - 0.5) * SCALE * LcdIli9341SPI::HEIGHT + LcdIli9341SPI::HEIGHT / 2, 2, 2, COLOR_LINE);
				lcd.drawLine(
					i - 1, (adcBufferLatest[i - 1] / 256.0 - 0.5) * SCALE * LcdIli9341SPI::HEIGHT + LcdIli9341SPI::HEIGHT / 2,
					i, (adcBufferLatest[i] / 256.0 - 0.5) * SCALE * LcdIli9341SPI::HEIGHT + LcdIli9341SPI::HEIGHT / 2,
					1, COLOR_LINE);
			}
			if (adcBuffer.getBufferSize() > AdcBuffer::BUFFER_NUM  * 0.6) {	// do not pop data immedeately, because fft may be using it
				adcBuffer.deleteFront();
			}
		} else {
			sleep_ms(1);
		}

		if (g_fftResultList.size() > 2) {  // buffer size needs to be 3 (when the size is 2, data may be not filled yet)
			/* Display FFT */
			auto& fftPrevious = g_fftResultList[0];
			auto& fftLatest = g_fftResultList[1];
			for (int32_t i = 1; i < fftPrevious.size() / 2; i++) {
				// lcd.putPixel(LcdIli9341SPI::WIDTH - i, fftPrevious[i] * LcdIli9341SPI::HEIGHT, COLOR_BG);
				// lcd.drawRect(LcdIli9341SPI::WIDTH - i, fftPrevious[i] * LcdIli9341SPI::HEIGHT, 2, 2, COLOR_BG);
				lcd.drawLine(
					LcdIli9341SPI::WIDTH - (i - 1), fftPrevious[i - 1] * LcdIli9341SPI::HEIGHT,
					LcdIli9341SPI::WIDTH - i, fftPrevious[i] * LcdIli9341SPI::HEIGHT,
					1, COLOR_BG);
			}
			for (int32_t i = 1; i < fftLatest.size() / 2; i++) {
				// lcd.putPixel(LcdIli9341SPI::WIDTH - i, fftLatest[i] * LcdIli9341SPI::HEIGHT, COLOR_LINE);
				// lcd.drawRect(LcdIli9341SPI::WIDTH - i, fftLatest[i] * LcdIli9341SPI::HEIGHT, 2, 2, COLOR_LINE);
				lcd.drawLine(
					LcdIli9341SPI::WIDTH - (i - 1), fftLatest[i - 1] * LcdIli9341SPI::HEIGHT,
					LcdIli9341SPI::WIDTH - i, fftLatest[i] * LcdIli9341SPI::HEIGHT,
					1, COLOR_LINE);
			}
			(void)g_fftResultList.pop_front();
		}

		// PRINT_TIME();
		// printf("---\n");
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



extern int fft(int n, float x[], float y[]);
double hammingWindow(double x)
{
	double val = 0.54 - 0.46 * std::cos(2 * M_PI * x);
	return val;
}

void core1_main()
{
	printf("Hello, core1!\n");
#if 0
	/* test FFT*/
	#define    N 256
	static float x[N], y[N];

	for(int32_t i = 0; i < N; i++){
		x[i] = std::sin((1.0 * i * 100) / N * 2 * M_PI);
		x[i] *= hammingWindow((double)(i)/N);
		y[i] = 0;
	}

	if (fft(N, x, y)) {
		printf("error\n");
	}

	for (int32_t i = 0; i < N / 2; i++){
		double p = sqrt(x[i] * x[i] + y[i] * y[i]);
		printf("%d: %.03f %.03f %.03f\n",i, p, x[i], y[i]); // power real-part imaginary-part
	}

	while(1) {
		sleep_ms(100);
	}
#else
	while(1) {
		if (adcBuffer.getBufferSize() > 2) {
			auto& data = adcBuffer.getBuffer(1);
			std::vector<float> x(data.size());
			for (int32_t i = 0; i < x.size(); i++) {
				x[i] = (data[i] / 256.0 - 0.5) * 2;	// -1 ~ +1
				x[i] *= SCALE_FFT;
				x[i] *= hammingWindow((double)(i) / x.size());
			}
			if (g_fftResultList.size() > 10) {
				printf("overflow at g_fftResultList\n");
			} else {
				g_fftResultList.resize(g_fftResultList.size() + 1);
				g_fftResultList.back().resize(x.size());
			}
			auto resultBuffer = g_fftResultList.back().data();
			for (int32_t i = 0; i < x.size(); i++) resultBuffer[i] = 0;

			(void)fft(x.size(), x.data(), resultBuffer);
			for (int32_t i = 0; i < x.size() / 2; i++){
				resultBuffer[i] = std::sqrt(x[i] * x[i] + resultBuffer[i] * resultBuffer[i]);
			}
		} else {
			sleep_ms(1);
		}
	}
#endif
}

