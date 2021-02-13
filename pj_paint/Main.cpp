#include <cstdint>
#include <cstdio>
// #include <cstdlib>
// #include <cstring>

#include "pico/stdlib.h"
#include "LcdIli9341SPI.h"
#include "TpTsc2046SPI.h"

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


	reset(lcd);
	int32_t tpXprevious = -1;
	int32_t tpYprevious = -1;
	std::array<uint8_t, 2> colorLine = { 0x07, 0xE0 };
	while(1) {
		int32_t c = getchar_timeout_us(10);
		if (0 < c && c < 0x80) {
			lcd.putChar(c);
		}

		float tpX, tpY, tpPressure;
		tp.getFromDevice(tpX, tpY, tpPressure);
		if (tpPressure > 50 && tpX < 0.95) {
			// printf("%.03f %.03f %.1f\n", tpX, tpY, tpPressure);
			tpX *= LcdIli9341SPI::WIDTH;
			tpY *= LcdIli9341SPI::HEIGHT;
			if (tpX < 50 && tpY < 50) {
					reset(lcd);
			}
			if (tpXprevious != -1) {
				lcd.drawLine(tpXprevious, tpYprevious, tpX, tpY, 2, colorLine);
			}
			tpXprevious = tpX;
			tpYprevious = tpY;			
		} else {
			tpXprevious = -1;
			tpYprevious = -1;
		}
	}

	lcd.finalize();
	tp.finalize();

	return 0;
}

static void reset(LcdIli9341SPI& lcd)
{
	std::array<uint8_t, 2> colorBg = { 0x00, 0x1F };
	lcd.drawRect(0, 0, LcdIli9341SPI::WIDTH, LcdIli9341SPI::HEIGHT, colorBg);
	lcd.setCharPos(0, 0);
	lcd.putText("CLEAR");
}
