//LCDコントローラ ST77xx	SPI-DMA基本操作
//『昼夜逆転』工作室	@jsdiy	https://github.com/jsdiy
//	2020/05 - 2025/08
//前提：アプリで使用するグラフィックLCDは1つ

#pragma	once

#include <Arduino.h>
#include "Color.hpp"
#include "GLcdSpiDma.hpp"

//ユーザー設定
namespace	ST77xxConfig
{
	//ピンアサイン
	//参考	ESP32のVSPI: SS = GPIO_NUM_5, MOSI = GPIO_NUM_23, (MISO = GPIO_NUM_19), SCK = GPIO_NUM_18
	constexpr	gpio_num_t
		PinSS		= GPIO_NUM_5,	//ST7735:CS, ST7789:CS
		PinDC		= GPIO_NUM_16,	//ST7735:A0, ST7789:DC
		PinRESET	= GPIO_NUM_NC;	//ST7735:RESET, ST7789:RST

	//コントローラー
	enum	class	EController	{ ST7735, ST7789 };
	constexpr	auto	Controller = EController::ST7789;
}

//Memory data access control
enum	class	ERotFlip	: uint8_t
{
	YFlip	= (1 << 7),		//bit7(MY)	MCUからメモリへの書き込み方向…	0:順方向, 1:逆方向（垂直反転）
	XFlip	= (1 << 6),		//bit6(MX)	MCUからメモリへの書き込み方向…	0:順方向, 1:逆方向（水平反転）
	Rot90	= (1 << 5),		//bit5(MV)	MCUからメモリへの書き込み方向…	0:水平方向優先, 1:垂直方向優先（90度回転）
	RefleshBtoT	= (1 << 4),	//bit4(ML)	LCDパネルのリフレッシュ方向…	0:Top行→Bottom行方向, 1:Bottom行→Top行方向
	PixOrderBGR	= (1 << 3),	//bit3(RGB)	メモリ上のRGBデータとLCDパネルのRGB画素の並び順の対応…	0:RGB, 1:BGR
	RefleshRtoL	= (1 << 2),	//bit2(MH)	LCDパネルのリフレッシュ方向…	0:Left列→Right列方向, 1:Right列→Left列方向
	Rot180	= (YFlip | XFlip),	//※Rot180,Rot270に対してYFlip,XFlipを'OR'しても効果はないことに注意。
	Rot270	= (Rot90 | Rot180),
	/*
	・LCDの画像を垂直反転／水平反転／90度回転させるには MY/MX/MVビットを指定する。
	・表示色のRGB成分が反対に解釈されていたらRGBビットを指定する。ドライバICとLCDパネルの組み合わせによる現象。
	・画像がスクロールしている場合、リフレッシュ方向を設定するとチラつきが軽減される可能性がある。
	*/
};

//グラフィックLCDクラス
class	LcdST77xx	//: public Graphics
{
private:
	static	constexpr	uint8_t		SpiMode = 0;	//SPIモード(0-3)
	static	constexpr	uint32_t	SpiClock = 16UL * 1000000;	//SPIクロック(Hz)	※80MHzの分周値が最適
	//ST7735の4線モードは SCLK_min = 66ns -> SPI_busclock = 15.2MHz
	//ST7789の4線モードは SCL(TSCYCW: Serial clock cycle (Write)) = 66ns [write command & data ram]

private:
	GLcdSpiDma	glcd;
	uint8_t*	grBuffer;	//画素データを効率よく出力するための汎用バッファ
	int16_t	screenWidth, screenHeight;
	void	SetGRamArea(int16_t x, int16_t y, int16_t w, int16_t h);

public:
	LcdST77xx(void);
	int16_t	Width(void) { return screenWidth; }
	int16_t	Height(void) { return screenHeight; }
	void	Initialize(void);
	void	HwReset(void);
	void	RotateFlip(ERotFlip param);
	void	FillRect(int16_t x, int16_t y, int16_t w, int16_t h, const Color& color);
	void	DrawImage(int16_t x, int16_t y, int16_t w, int16_t h, const uint8_t* imgData, size_t dataLength);
	void	ClearScreen(const Color& color);
};

//ビットOR演算子'|'のオーバーロード
//・func(EnumType n)に対して、func((EnumType)(A|B)) → func(A|B)と書ける。
//・constexprはinlineと同様に、関数（的なもの）の実装はヘッダーに記述する。
constexpr	ERotFlip	operator | (ERotFlip lhs, ERotFlip rhs)
{
	return static_cast<ERotFlip>(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

//ビットAND演算子'&'のオーバーロード
constexpr	uint8_t	operator & (ERotFlip lhs, ERotFlip rhs)
{
	return static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs);
}
