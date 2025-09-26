//グラフィックLCD用SPI-DMA基本操作
//『昼夜逆転』工作室	@jsdiy	https://github.com/jsdiy
//	2025/09

#pragma	once

#include <Arduino.h>
#include "SpiDma.hpp"

//SPI-DMAを利用するグラフィックLCDの基本制御
//・グラフィックLCD一つごとにインスタンス化する。
class	GLcdSpiDma
{
private:
	gpio_num_t	pinDC;	//LCDのDCピン（コマンド/データ）
	spi_device_handle_t	devHandle;
	void	SetPinMode(gpio_num_t pinSS, gpio_num_t pinDC);
	void	SetDeviceConfig(uint8_t spiMode, int32_t busSpeedHz, gpio_num_t pinSS, spi_device_interface_config_t* devConfig);
	void	ChangeCommandMode(void) { gpio_set_level(pinDC, 0); }	//digitalWrite()より高速（コールバック関数からの利用を考慮して速さ重視）
	void	ChangeDataMode(void) { gpio_set_level(pinDC, 1); }	//digitalWrite()より高速（コールバック関数からの利用を考慮して速さ重視）

public:
	GLcdSpiDma(void) { devHandle = nullptr;	pinDC = GPIO_NUM_NC; }
	void	Initialize(uint8_t spiMode, int32_t busSpeedHz, gpio_num_t pinSS, gpio_num_t pinDC);
	void	BeginTransaction(void) { spiDma.TakeBusControll(); }	//排他制御開始
	void	EndTransaction(void)   { spiDma.GiveBusControll(); }	//排他制御終了
	void	SendCommand(uint8_t cmd) { ChangeCommandMode();	spiDma.Transmit(devHandle, cmd); }
	void	SendCommand(uint8_t cmd, uint8_t data) { SendCommand(cmd);	SendData(data); };
	void	SendData(uint8_t data) { ChangeDataMode();	spiDma.Transmit(devHandle, data); }
	void	SendData(const uint8_t* datas, size_t length);
	void	SetGRamRange(uint8_t cmd, int16_t start, int16_t end);
};
