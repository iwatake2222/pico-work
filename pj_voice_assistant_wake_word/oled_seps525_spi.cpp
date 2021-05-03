#include <cstdint>
#include <cstdlib>
#include <cstdio>
#define _USE_MATH_DEFINES
#include <cmath>
#include <array>
#include <vector>
#include "pico/stdlib.h"
#include "hardware/spi.h"

#include "oled_seps525_spi.h"

static inline auto GetSpi(int32_t spiNum)
{
    return (spiNum == 0) ? spi0 : spi1;
}

int32_t OledSeps525Spi::Initialize(const Config& config)
{
    spi_port_num_ = config.spi_port_num;
    pin_sck_ = config.pin_sck;
    pin_mosi_ = config.pin_mosi;
    pin_cs_ = config.pin_cs;
    pin_dc_ = config.pin_dc;
    pin_reset_ = config.pin_reset;

    InitializeIo();
    InitializeDevice();
    SetArea(0, 0, kWidth, kHeight);

    return kRetOk;
}

void OledSeps525Spi::InitializeIo(void)
{
    spi_init(GetSpi(spi_port_num_), 50 * 1000 * 1000);
    gpio_set_function(pin_sck_ , GPIO_FUNC_SPI);
    gpio_set_function(pin_mosi_ , GPIO_FUNC_SPI);
    // gpio_set_function(pin_miso_, GPIO_FUNC_SPI);

    // gpio_set_function(pin_cs_, GPIO_FUNC_SPI);
    gpio_init(pin_cs_);
    gpio_set_dir(pin_cs_, GPIO_OUT);
    DisableCs();

    gpio_init(pin_dc_);
    gpio_set_dir(pin_dc_, GPIO_OUT);

    gpio_init(pin_reset_);
    gpio_set_dir(pin_reset_, GPIO_OUT);
}

void OledSeps525Spi::InitializeDevice(void)
{
    gpio_put(pin_reset_, 0);
    sleep_ms(50);
    gpio_put(pin_reset_, 1);
    sleep_ms(50);

    WriteInitializeCmd(0x04, 0x03);
    sleep_ms(5);
    WriteInitializeCmd(0x04, 0x00);
    sleep_ms(5);
    WriteInitializeCmd(0x3B, 0x00);
    WriteInitializeCmd(0x02, 0x01);
    WriteInitializeCmd(0x03, 0x90);
    WriteInitializeCmd(0x80, 0x01);
    WriteInitializeCmd(0x08, 0x04);
    WriteInitializeCmd(0x09, 0x05);
    WriteInitializeCmd(0x0A, 0x05);
    WriteInitializeCmd(0x0B, 0x9D);
    WriteInitializeCmd(0x0C, 0x8C);
    WriteInitializeCmd(0x0D, 0x57);
    WriteInitializeCmd(0x10, 0x56);
    WriteInitializeCmd(0x11, 0x4D);
    WriteInitializeCmd(0x12, 0x46);
    WriteInitializeCmd(0x14, 0x11);
#if 1
    WriteInitializeCmd(0x13, 0xB0); // RGB swap. reverse scan direction (row and column)
    // WriteInitializeCmd(0x13, 0x30); // reverse scan direction (row and column)
    WriteInitializeCmd(0x16, 0x66);  /* Dual Transfer 65k */
#else
    WriteInitializeCmd(0x13, 0x80); // RGB swap
    WriteInitializeCmd(0x16, 0x66);  /* Dual Transfer 65k */
#endif
    WriteInitializeCmd(0x20, 0x00);
    WriteInitializeCmd(0x21, 0x00);
    WriteInitializeCmd(0x28, 0x7F);
    WriteInitializeCmd(0x29, 0x00);
    WriteInitializeCmd(0x06, 0x01);
    WriteInitializeCmd(0x05, 0x00);
    WriteInitializeCmd(0x15, 0x00);

    WriteInitializeCmd(0x17, 0x00);
    WriteInitializeCmd(0x18, kWidth - 1);
    WriteInitializeCmd(0x19, 0x00);
    WriteInitializeCmd(0x1A, kHeight - 1);
    WriteInitializeCmd(0x20, 0x00);
    WriteInitializeCmd(0x21, 0x00);
}

int32_t OledSeps525Spi::Finalize(void)
{
    spi_deinit(GetSpi(spi_port_num_));
    return kRetOk;
}

void OledSeps525Spi::SetArea(int32_t x, int32_t y, int32_t w, int32_t h)
{
    WriteInitializeCmd(0x17, x);
    WriteInitializeCmd(0x18, x + w - 1);
    WriteInitializeCmd(0x19, y);
    WriteInitializeCmd(0x1A, y + h - 1);
    WriteInitializeCmd(0x20, x);
    WriteInitializeCmd(0x21, y);

}

void OledSeps525Spi::PutPixel(int32_t x, int32_t y, std::array<uint8_t, 2> color)
{
    WriteInitializeCmd(0x20, x);
    WriteInitializeCmd(0x21, y);
    WriteCmd(0x22);
    WriteData(color.data(), 2);
}

void OledSeps525Spi::DrawRect(int32_t x, int32_t y, int32_t w, int32_t h, std::array<uint8_t, 2> color)
{
    SetArea(x, y, w, h);
    WriteCmd(0x22);
    for (int32_t i = 0; i < w * h; i++) {
        WriteData(color.data(), 2);
    }
}

void OledSeps525Spi::DrawBuffer(int32_t x, int32_t y, int32_t w, int32_t h, std::vector<uint8_t> buffer)
{
    SetArea(x, y, w, h);
    WriteCmd(0x22);
    WriteData(buffer.data(), buffer.size());
}

void OledSeps525Spi::EnableCs(void)
{
    // asm volatile("nop \n nop \n nop");
    gpio_put(pin_cs_, 0);	// Active low
    // asm volatile("nop \n nop \n nop");
}

void OledSeps525Spi::DisableCs(void)
{
    // asm volatile("nop \n nop \n nop");
    gpio_put(pin_cs_, 1);	// Active low
    // asm volatile("nop \n nop \n nop");
}

void OledSeps525Spi::WriteInitializeCmd(uint8_t cmd, uint8_t data)
{
    WriteCmd(cmd);
    WriteData(data);
}

void OledSeps525Spi::WriteCmd(uint8_t cmd)
{
    gpio_put(pin_dc_, 0);
    EnableCs();
    spi_write_blocking(GetSpi(spi_port_num_), &cmd, 1);
    DisableCs();
}

void OledSeps525Spi::WriteData(uint8_t data)
{
    gpio_put(pin_dc_, 1);
    EnableCs();
    spi_write_blocking(GetSpi(spi_port_num_), &data, 1);
    DisableCs();
}

void OledSeps525Spi::WriteData(uint8_t dataBuffer[], int32_t len)
{
    gpio_put(pin_dc_, 1);
    EnableCs();
    spi_write_blocking(GetSpi(spi_port_num_), dataBuffer, len);
    DisableCs();
}

void OledSeps525Spi::ReadData(uint8_t cmd, uint8_t dataBuffer[], int32_t len)
{
    gpio_put(pin_dc_, 0);
    EnableCs();
    spi_write_blocking(GetSpi(spi_port_num_), &cmd, 1);
    
    gpio_put(pin_dc_, 1);
    spi_read_blocking(GetSpi(spi_port_num_), 0, dataBuffer, len);
    DisableCs();
}

void OledSeps525Spi::Test()
{
    std::array<uint8_t, 2> colorBg = { 0x00, 0x00 };
    std::array<uint8_t, 2> colorTest = { 0x00, 0x1F };
    std::array<uint8_t, 2> colorTest2 = { 0x07, 0xE0 };
    DrawRect(0, 0, kWidth, kHeight, colorBg);
    DrawRect(0, 0, kWidth, kHeight, colorTest);
    DrawRect(20, 10, 60, 30, colorTest2);

    for(int32_t y = 100; y < 110; y++) {
        for(int32_t x = 100; x < 110; x++) {
            PutPixel(x, y, colorTest2);
        }
    }

    std::vector<uint8_t> colorBuffer;
    for(int32_t i = 0; i < 5 * 5; i++) {
        colorBuffer.push_back(0x07);
        colorBuffer.push_back(0xE0);
    }
    DrawBuffer(120, 120, 5, 5, colorBuffer);
}
