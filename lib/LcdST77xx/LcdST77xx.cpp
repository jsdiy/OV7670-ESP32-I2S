//LCDコントローラ ST77xx	SPI-DMA基本操作
//『昼夜逆転』工作室	@jsdiy	https://github.com/jsdiy
//	2025/09

#include <Arduino.h>
#include "LcdST77xx.hpp"

//ST77XXのコマンド（利用頻度が高そうなもの）
enum	ECommand	: uint8_t
{
	CmdSWRESET	= 0x01,	//Software reset
	CmdSLPIN	= 0x10,	//Sleep in & booster off	(Sleep-outからSleep-inした場合、120ms待つ必要がある)
	CmdSLPOUT	= 0x11,	//Sleep out & booster on	(Sleep-inからSleep-outした場合、120ms待つ必要がある)
	CmdINVON	= 0x21,	//Display Inversion On	（色反転。RGB_00:00:00が白, FF:FF:FFが黒）
	CmdDISPOFF	= 0x28,	//Display off
	CmdDISPON	= 0x29,	//Display on
	CmdCASET	= 0x2A,	//Column address set
	CmdRASET	= 0x2B,	//Row address set
	CmdRAMWR	= 0x2C,	//Memory write
	CmdMADCTL	= 0x36,	//Memory data access control
	CmdCOLMOD	= 0x3A,	//Interface pixel format
};
/*	ST7735のコマンド一覧
//System Function Command List and Description
static	const	uint8_t
	NOP		= 0x00,	//No Operation
	SWRESET	= 0x01,	//Software reset
	SLPIN	= 0x10,	//Sleep in & booster off	(Sleep-outからSleep-inした場合、120ms待つ必要がある)
	SLPOUT	= 0x11,	//Sleep out & booster on	(Sleep-inからSleep-outした場合、120ms待つ必要がある)
	PTLON	= 0x12,	//Partial mode on
	NORON	= 0x13,	//Partial off (Normal mode on)
	INVOFF	= 0x20,	//Display inversion off
	INVON	= 0x21,	//Display inversion on
	GAMSET	= 0x26,	//Gamma curve select
	DISPOFF	= 0x28,	//Display off
	DISPON	= 0x29,	//Display on
	CASET	= 0x2A,	//Column address set
	RASET	= 0x2B,	//Row address set
	RAMWR	= 0x2C,	//Memory write
	PTLAR	= 0x30,	//Partial start/end address set
	TEOFF	= 0x34,	//Tearing effect line off
	TEON	= 0x35,	//Tearing effect mode set & on
	MADCTL	= 0x36,	//Memory data access control
	IDMOFF	= 0x38,	//Idle mode off
	IDMON	= 0x39,	//Idle mode on
	COLMOD	= 0x3A;	//Interface pixel format

//Panel Function Command List and Description
static	const uint8_t
	FRMCTR1	= 0xB1,	//In normal mode (Full colors)
	FRMCTR2	= 0xB2,	//In Idle mode (8-colors)
	FRMCTR3	= 0xB3,	//In partial mode + Full colors
	INVCTR	= 0xB4,	//Display inversion control
	DISSET5	= 0xB6,	//Display function setting
	PWCTR1	= 0xC0,	//Power control setting
	PWCTR2	= 0xC1,	//Power control setting
	PWCTR3	= 0xC2,	//Power control setting / In normal mode (Full colors)
	PWCTR4	= 0xC3,	//Power control setting / In Idle mode (8-colors)
	PWCTR5	= 0xC4,	//Power control setting / In partial mode + Full colors
	VMCTR1	= 0xC5,	//VCOM control 1
	VMOFCTR	= 0xC7,	//Set VCOM offset control
	WRID2	= 0xD1,	//Set LCM version code
	WRID3	= 0xD2,	//Customer Project code
	NVCTR1	= 0xD9,	//EEPROM control status
	NVCTR2	= 0xDE,	//EEPROM Read Command
	NVCTR3	= 0xDF,	//EEPROM Write Command
	GAMCTRP1	= 0xE0,	//Set Gamma adjustment ('+' polarity)
	GAMCTRN1	= 0xE1,	//Set Gamma adjustment ('-' polarity)
	EXTCTRL	= 0xF0,	//Extension Command Control
	PWCTR6	= 0xFC,	//Power control setting / In partial mode + Idle
	VCOM4L	= 0xFF;	//Vcom 4 Level control
*/

//コンストラクタ
LcdST77xx::LcdST77xx(void)
{
	using	namespace	ST77xxConfig;
	if (Controller == EController::ST7735) { screenWidth = 128;	screenHeight = 160; }
	if (Controller == EController::ST7789) { screenWidth = 240;	screenHeight = 320; }
	grBuffer = nullptr;
}

//初期化
void	LcdST77xx::Initialize(void)
{
	using	namespace	ST77xxConfig;

	//SPIバス占有（以下に長時間の時間待ちを含むが、setup()時なら他のSPI利用デバイスへの影響は小さい）
	glcd.BeginTransaction();

	//SPIデバイス情報を初期化する
	glcd.Initialize(SpiMode, SpiClock, PinSS, PinDC);

	//LCDをリセットする
	glcd.SendCommand(CmdSWRESET);	delay(200);	//120ms以上

	//スリープ解除する（リセット直後はスリープ状態にあるので）
	glcd.SendCommand(CmdSLPOUT);	delay(200);	//120ms以上

	//カラーモード（色深度）を指定する
	if (Controller == EController::ST7735) { glcd.SendCommand(CmdCOLMOD, 0x05); }	//RGB565
	if (Controller == EController::ST7789) { glcd.SendCommand(CmdCOLMOD, 0x55); }	//RGB565

	//色反転（これが必要かはLCDモジュールによる）
	//・手持ちのST7735では不要、ST7789では必要だった。
	if (Controller == EController::ST7789) { glcd.SendCommand(CmdINVON); }

	//画面表示オン
	glcd.SendCommand(CmdDISPON);

	//SPIバス解放
	glcd.EndTransaction();

	//ラインバッファ確保
	size_t bufLength = std::max(screenWidth, screenHeight) * Color::Length;
	grBuffer = (uint8_t*)heap_caps_malloc(bufLength, MALLOC_CAP_DMA | MALLOC_CAP_32BIT);
	Serial.printf("LCD grBuffer size: %d byte\n", bufLength);
}

//ハードウェアリセット
void	LcdST77xx::HwReset(void)
{
	using	namespace	ST77xxConfig;
	if (PinRESET == GPIO_NUM_NC) { return; }
	pinMode(PinRESET, OUTPUT);
	digitalWrite(PinRESET, LOW);	delay(1);	//10us以上
	digitalWrite(PinRESET, HIGH);	delay(200);	//120ms以上
}

//画面を回転／反転させる
void	LcdST77xx::RotateFlip(ERotFlip param)
{
	glcd.BeginTransaction();
	glcd.SendCommand(CmdMADCTL, static_cast<uint8_t>(param));
	glcd.EndTransaction();
	if (param & ERotFlip::Rot90) { std::swap(screenWidth, screenHeight); }
}

//データ書込み先のGRAM領域を設定する
//引数:	データ書込み先のGRAM領域。画面内に収まっていること。
void	LcdST77xx::SetGRamArea(int16_t x, int16_t y, int16_t w, int16_t h)
{
	glcd.SetGRamRange(CmdCASET, x, x + w - 1);	//startX,endX
	glcd.SetGRamRange(CmdRASET, y, y + h - 1);	//startY,endY
}

//単色で矩形領域を塗りつぶす
//引数	x,y,w,h:	画面上の四角形領域（画面に収まっていること）
void	LcdST77xx::FillRect(int16_t x, int16_t y, int16_t w, int16_t h, const Color& color)
{
	size_t dataLength = w * Color::Length;
	for (size_t i = 0; i < dataLength; i += Color::Length) { memcpy(&grBuffer[i], color.bytes, Color::Length); }

	glcd.BeginTransaction();
	SetGRamArea(x, y, w, h);
	glcd.SendCommand(CmdRAMWR);
	for (int16_t i = 0; i < h; i++) { glcd.SendData(grBuffer, dataLength); }
	glcd.EndTransaction();
}

//画像を描く
//引数	x,y,w,h:	画面上の四角形領域（画面に収まっていること）
//		imgData:	画素データ
//		dataLength:	データ長(byte)
void	LcdST77xx::DrawImage(int16_t x, int16_t y, int16_t w, int16_t h, const uint8_t* imgData, size_t dataLength)
{
	glcd.BeginTransaction();
	SetGRamArea(x, y, w, h);
	glcd.SendCommand(CmdRAMWR);
	glcd.SendData(imgData, dataLength);
	glcd.EndTransaction();
}

//画面全体を塗りつぶす
void	LcdST77xx::ClearScreen(const Color& color)
{
	FillRect(0, 0, screenWidth, screenHeight, color);
}
