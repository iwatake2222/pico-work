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
static constexpr std::array<uint8_t, 2> COLOR_LINE_FFT = { 0x00, 0x1F };
static constexpr int32_t SCALE = 2;
static constexpr int32_t SCALE_FFT = 10;
static constexpr int32_t BUFFER_SIZE = 256;	// 2^x
static constexpr int32_t SAMPLING_RATE = 10000;
#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif
static constexpr int32_t MIN_ADC_BUFFER_SIZE = MAX(2, (AdcBuffer::BUFFER_NUM + 1) / 2);
static constexpr int32_t FFT_BUFFER_NUM = 3;

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
static LcdIli9341SPI& createStaticLcd(void);
static TpTsc2046SPI& createStaticTp(void);
static AdcBuffer& createStaticAdcBuffer(void);
static void reset(LcdIli9341SPI& lcd);
static bool displayWave(AdcBuffer& adcBuffer, LcdIli9341SPI& lcd);
static void displayFft(LcdIli9341SPI& lcd);
static void displayTime(LcdIli9341SPI& lcd, bool isSkipDisplay, uint32_t core0, uint32_t core1);
static void switchMultiCore(TpTsc2046SPI& tp);

/*** GLOBAL VARIABLE ***/
AdcBuffer* g_adcBuffer;
std::deque<std::vector<float>> g_fftResultList;
static int32_t g_timeFFT = 0;	// [msec]
static bool g_multiCore = true;
static int32_t s_cntForSkipDisplayTime = 0;			// workaround to avoid freeze (it looks crash happens becuse core0 processing time is long)

int main() {
	/*** Initilization ***/
	/* Initialize system */
	stdio_init_all();
	sleep_ms(1000);	// wait until UART connected
	printf("Hello, world!\n");

	/* Initialize device */
	LcdIli9341SPI& lcd = createStaticLcd();
	TpTsc2046SPI& tp = createStaticTp();
	AdcBuffer& adcBuffer = createStaticAdcBuffer();
	g_adcBuffer = &adcBuffer;
	reset(lcd);
	
	/* Prepare core1 for FFT */
	if (g_multiCore) {
		multicore_launch_core1(core1_main);
	}

	/*** Main loop ***/
	adcBuffer.start();
	while(1) {
		uint32_t t0 = to_ms_since_boot(get_absolute_time());
		bool isSkipDisplay = displayWave(adcBuffer, lcd);
		displayFft(lcd);

		if (!g_multiCore) {
			core1_main();
		}

		uint32_t t1 = to_ms_since_boot(get_absolute_time());
		if (s_cntForSkipDisplayTime++ % 30 == 0) {
			displayTime(lcd, isSkipDisplay, t1 - t0, g_timeFFT);
		}
		switchMultiCore(tp);
	}

	/*** Finalization ***/
	adcBuffer.stop();
	adcBuffer.finalize();
	lcd.finalize();
	tp.finalize();

	return 0;
}

static LcdIli9341SPI& createStaticLcd(void)
{
	static LcdIli9341SPI lcd;
	LcdIli9341SPI::CONFIG lcdConfig;
	lcdConfig.spiPortNum = 0;
	lcdConfig.pinSck = 2;
	lcdConfig.pinMosi = 3;
	lcdConfig.pinMiso = 4;
	lcdConfig.pinCs = 5;
	lcdConfig.pinDc = 7;
	lcdConfig.pinReset = 6;
	lcd.initialize(lcdConfig);
	lcd.test();
	return lcd;
}

static TpTsc2046SPI& createStaticTp(void)
{
	static TpTsc2046SPI tp;
	TpTsc2046SPI::CONFIG tpConfig;
	tpConfig.spiPortNum = 1;
	tpConfig.pinSck = 10;
	tpConfig.pinMosi = 11;
	tpConfig.pinMiso = 12;
	tpConfig.pinCs = 13;
	tpConfig.pinIrq = 14;
	tpConfig.callback = nullptr;
	tp.initialize(tpConfig);
	return tp;
}

static AdcBuffer& createStaticAdcBuffer(void)
{
	static AdcBuffer adcBuffer;
	AdcBuffer::CONFIG adcConfig;
	adcConfig.captureChannel = 0;
	// adcConfig.captureDepth = LcdIli9341SPI::WIDTH;
	adcConfig.captureDepth = BUFFER_SIZE;
	adcConfig.samplingRate = SAMPLING_RATE;
	adcBuffer.initialize(adcConfig);
	return adcBuffer;
}

static void reset(LcdIli9341SPI& lcd)
{
	std::array<uint8_t, 2> colorBg = { 0x00, 0x1F };
	lcd.drawRect(0, 0, LcdIli9341SPI::WIDTH, LcdIli9341SPI::HEIGHT, COLOR_BG);
	lcd.setCharPos(0, 0);
}


static bool displayWave(AdcBuffer& adcBuffer, LcdIli9341SPI& lcd)
{
	/* When getting ADC data from buffer, use the halfway point index of buffer to avoid crash.
		*  If I use head point(older) index and delete the buffer after using, it may cause crash because FFT(core1) may still using it.
		*  (Note: FFT(core1) is just peeping the buffer)
		*  Good solution will be copying the data to another buffer and send it to core1 or using semaphore, but I do this just to make it easy
		* Also, do not include "equal" in the condition because adc(dma) is writing data into that index of buffer
	*/
	if (adcBuffer.getBufferSize() > MIN_ADC_BUFFER_SIZE) {
		auto& adcBufferPrevious = adcBuffer.getBuffer(MIN_ADC_BUFFER_SIZE - 2);
		auto& adcBufferLatest = adcBuffer.getBuffer(MIN_ADC_BUFFER_SIZE - 1);
		const float scale = 1 / 256.0 * SCALE * LcdIli9341SPI::HEIGHT;
		const float offset = - 0.5 * SCALE * LcdIli9341SPI::HEIGHT + LcdIli9341SPI::HEIGHT / 2 - 50;
		/* Delete previous line */
		for (int32_t i = 1; i < adcBufferPrevious.size(); i++) {
			// lcd.drawRect(i, (adcBufferPrevious[i] / 256.0 - 0.5) * SCALE * LcdIli9341SPI::HEIGHT + LcdIli9341SPI::HEIGHT / 2, 2, 2, COLOR_BG);
			lcd.drawLine(
				i - 1, adcBufferPrevious[i - 1] * scale + offset,
				i, adcBufferPrevious[i] * scale + offset + 1,
				2, COLOR_BG);
		}
		/* Draw new line */
		for (int32_t i = 1; i < adcBufferLatest.size(); i++) {
			// lcd.drawRect(i, (adcBufferLatest[i] / 256.0 - 0.5) * SCALE * LcdIli9341SPI::HEIGHT + LcdIli9341SPI::HEIGHT / 2, 2, 2, COLOR_LINE);
			lcd.drawLine(
				i - 1, adcBufferLatest[i - 1] * scale + offset,
				i, adcBufferLatest[i] * scale + offset + 1,
				2, COLOR_LINE);
		}
		adcBuffer.deleteFront();
		return false;
	} else {
		sleep_ms(1);
		return true;
	}
}

static void displayFft(LcdIli9341SPI& lcd)
{
	if (g_fftResultList.size() > 2) {
		auto& fftPrevious = g_fftResultList[0];
		auto& fftLatest = g_fftResultList[1];		// do not use the latest buffer (FFT_BUFFER_NUM - 1)
		/* Delete previous line */
		for (int32_t i = 1; i < fftPrevious.size() / 2; i++) {
			// lcd.drawRect(LcdIli9341SPI::WIDTH - i, fftPrevious[i] * LcdIli9341SPI::HEIGHT, 2, 2, COLOR_BG);
			lcd.drawLine(
				i - 1, LcdIli9341SPI::HEIGHT * (1 - fftPrevious[i - 1]),
				i, LcdIli9341SPI::HEIGHT * (1 - fftPrevious[i]) + 1,
				2, COLOR_BG);
		}
		/* Draw new line */
		for (int32_t i = 1; i < fftLatest.size() / 2; i++) {
			// lcd.drawRect(LcdIli9341SPI::WIDTH - i, fftLatest[i] * LcdIli9341SPI::HEIGHT, 2, 2, COLOR_LINE);
			lcd.drawLine(
				i - 1, LcdIli9341SPI::HEIGHT * (1 - fftLatest[i - 1]),
				i, LcdIli9341SPI::HEIGHT * (1 - fftLatest[i]) + 1,
				2, COLOR_LINE_FFT);
		}
		(void)g_fftResultList.pop_front();
	}
}


static void displayTime(LcdIli9341SPI& lcd, bool isSkipDisplay, uint32_t core0, uint32_t core1)
{
	// printf("Time[ms]: main = %d, FFT = %d [ms]\n", core0, core1);
	if (!isSkipDisplay) {
		lcd.setCharPos(120, 160);
		if (g_multiCore) {
			lcd.putText("Dual Core");
		} else {
			lcd.putText("Single Core");
		}
		char text[20];
		snprintf(text, sizeof(text), "Sa = %d kHz", SAMPLING_RATE / 1000);
		lcd.setCharPos(120, 180);
		lcd.putText(text);
		snprintf(text, sizeof(text), "Main(UI) = %d ms", core0);
		lcd.setCharPos(120, 200);
		lcd.putText(text);
		snprintf(text, sizeof(text), "FFT = %d ms", core1);
		lcd.setCharPos(120, 220);
		lcd.putText(text);
	}
}

static void switchMultiCore(TpTsc2046SPI& tp)
{
	float tpX, tpY, tpPressure;
	tp.getFromDevice(tpX, tpY, tpPressure);
	if (tpPressure > 50) {
		static uint32_t s_previousTpCheckTime = 0;
		if (to_ms_since_boot(get_absolute_time()) - s_previousTpCheckTime > 1000) {
			if (g_multiCore) {
				g_multiCore = false;
				multicore_reset_core1();
			} else {
				g_multiCore = true;
				multicore_launch_core1(core1_main);
			}
			s_previousTpCheckTime = to_ms_since_boot(get_absolute_time());
			s_cntForSkipDisplayTime = 0;
		}
	}		
}

extern int fft(int n, float x[], float y[]);
double hammingWindow(double x)
{
	double val = 0.54 - 0.46 * std::cos(2 * M_PI * x);
	return val;
}

void core1_main()
{
	if (g_multiCore) {
		printf("Hello, core1!\n");
	}
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
		uint32_t t0 = to_ms_since_boot(get_absolute_time());
		if (g_adcBuffer->getBufferSize() > MIN_ADC_BUFFER_SIZE - 1) {			// do not use the latest data because, the index may be decreased by main thread
			auto& data = g_adcBuffer->getBuffer(MIN_ADC_BUFFER_SIZE - 2);
			std::vector<float> x(data.size());
			for (int32_t i = 0; i < x.size(); i++) {
				x[i] = (data[i] / 256.0 - 0.5) * 2;	// -1 ~ +1
				x[i] *= SCALE_FFT;
				x[i] *= hammingWindow((double)(i) / x.size());
			}
			if (g_fftResultList.size() >= FFT_BUFFER_NUM) {
				// printf("overflow at g_fftResultList\n");
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

		uint32_t t1 = to_ms_since_boot(get_absolute_time());
		g_timeFFT = t1 - t0;
		if (!g_multiCore) {
			break;
		}
	}
#endif
}

