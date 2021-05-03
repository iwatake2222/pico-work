#ifndef OLED_SEPS525_SPI_H_
#define OLED_SEPS525_SPI_H_

#include <cstdint>
#include <array>
#include <vector>
#include <string>

class OledSeps525Spi {
public:
    static constexpr int32_t kWidth = 160;
    static constexpr int32_t kHeight = 128;

    enum {
        kRetOk = 0,
        kRetErr = -1,
    };

    typedef struct {
        int32_t spi_port_num;
        int32_t pin_sck;
        int32_t pin_mosi;
        int32_t pin_cs;
        int32_t pin_dc;
        int32_t pin_reset;
    } Config;

public:
	OledSeps525Spi() {}
	~OledSeps525Spi() {}
	int32_t Initialize(const Config& config);
	int32_t Finalize(void);
	void Test();
	void SetArea(int32_t x, int32_t y, int32_t w, int32_t h);
	void PutPixel(int32_t x, int32_t y, std::array<uint8_t, 2> color);
	void DrawRect(int32_t x, int32_t y, int32_t w, int32_t h, std::array<uint8_t, 2> color);
	void DrawBuffer(int32_t x, int32_t y, int32_t w, int32_t h, std::vector<uint8_t> buffer);

private:
	void InitializeIo(void);
	void InitializeDevice(void);
	void EnableCs(void);
	void DisableCs(void);
    void WriteInitializeCmd(uint8_t cmd, uint8_t data);
	void WriteCmd(uint8_t cmd);
	void WriteData(uint8_t data);
	void WriteData(uint8_t data_buffer[], int32_t len);
	void ReadData(uint8_t cmd, uint8_t data_buffer[], int32_t len);
	

private:
	int32_t spi_port_num_;
	int32_t pin_sck_;
	int32_t pin_mosi_;
	int32_t pin_cs_;
	int32_t pin_dc_;
	int32_t pin_reset_;

};

#endif
