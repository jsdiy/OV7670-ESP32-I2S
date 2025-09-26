//カメラ操作 - OV7670
//『昼夜逆転』工作室	@jsdiy	https://github.com/jsdiy
//	2024/04 - 2025/07

#pragma	once

#include <Arduino.h>
#include "OV7670Register.hpp"
using	namespace 	OV7670RegVal;

//カメラモジュールに関する設定
namespace	CameraConfig
{
	//ピンアサイン
	namespace	CamPin
	{
		constexpr	gpio_num_t
			SCL		= GPIO_NUM_22,	//GPIO22:WireのSCL
			SDA		= GPIO_NUM_21,	//GPIO21:WireのSDA
			VSYNC	= GPIO_NUM_27,
			HREF	= GPIO_NUM_2,
			PCLK	= GPIO_NUM_14,
			XCLK	= GPIO_NUM_4,
			D7		= GPIO_NUM_36,	//GPIO36(VP):入力専用ピン
			D6		= GPIO_NUM_39,	//GPIO39(VN):入力専用ピン
			D5		= GPIO_NUM_34,	//GPIO34:入力専用ピン
			D4		= GPIO_NUM_35,	//GPIO35:入力専用ピン
			D3		= GPIO_NUM_32,
			D2		= GPIO_NUM_33,
			D1		= GPIO_NUM_25,
			D0		= GPIO_NUM_26;

			/*	【OV7670モジュールのピン配列】
		トップビュー
		←外側	レンズ側→
		3V3		GND
		SCL		SDA		※SIOC,SIODと書かれている場合もある
		VSYNC	HREF	※HREFがHSYNCと書かれている場合もある
		PCLK	XCLK
		D7		D6
		D5		D4
		D3		D2
		D1		D0
		RESET	PWDN	※RESETはLoでリセット	※PWDNはHiで動作停止
		*/
	}

	//フレームの速さ（XCLKの分周値）
	namespace	SpeedLevel
	{
		constexpr	uint8_t
			VGA		= 12,	//12～64
			QVGA	= 3,	//3～64
			QQVGA	= 1;	//1～64

		/*	【XCLKの分周値】
		分周値は1～64で設定する。アプリの内容（描画負荷）によって調整する。
		値が小さいほど速い。下限値は解像度により異なる。
		→キャプチャと描画の両方をコア1で動かす場合、経験則として VGA:12～64, QVGA:3～64, QQVGA:1～64.
		*/
	}
}

//カメラ映像の解像度
enum	class	ECamResolution	: uint8_t
{
	VGA,	//640x480
	QVGA,	//320x240
	QQVGA	//160x120
};

//カメラ映像のカラーモード
enum	class	ECamColorMode	: uint8_t
{
	RGB565,
	YUYV	//グレースケールへの利用を想定（未実装）
};

//カメラ操作
class	OV7670Device
{
public:
	static	const	uint8_t	ProductID = 0x76;

private:
	static	const	uint8_t	SlaveAddress = 0x21;	//7bitアドレス	※(W:0x42,R:0x43)>>1
	int16_t	width, height;
	int16_t	bytePerPixel;
	void	SupplyXclk(void);
	uint8_t	ReadRegister(uint8_t addr);
	void	WriteRegister(uint8_t addr, uint8_t data);
	void	WriteRegisterList(const regval_list* list);
	void	SoftwareReset(void);
	void	SetResolution(ECamResolution camRes);
	void	SetClockDiv(uint8_t xclkPreScale);	//引数: 1～64
	void	RefreshCLKRC(void);
	void	SetWindow(uint16_t hStart, uint16_t vStart);
	void	SetColorMode(ECamColorMode colMode);

public:
	OV7670Device(void);
	int16_t	Width(void) { return width; }
	int16_t	Height(void) { return height; }
	int16_t	BytePerPixel(void) { return bytePerPixel; }
	void	Initialize(void);
	//void	DeviceConfigure(ECamResolution resolution, ECamColorMode colorMode);
	void	DeviceConfigure(ECamResolution resolution, ECamColorMode colorMode, uint8_t xclkPreScale);
	//
	uint16_t	GetProductID(void);
	void	ColorBar(bool isOn);
	void	ColorBarTr(bool isOn);	//半透明なカラーバー
	void	Flip(ERegVal mvfp);	//引数はMVFP_XXX
	void	GetAwbGain(uint8_t* gainR, uint8_t* gainG, uint8_t* gainB);	//AutoWhiteBalanceの現在値を取得する
	void	SetAwbGain(uint8_t gainR, uint8_t gainG, uint8_t gainB);	//AutoWhiteBalanceを設定する
	uint8_t	GetBrightness(void);
	void	SetBrightness(uint8_t val);
	uint8_t	GetContrast(void);
	void	SetContrast(uint8_t val);
};
