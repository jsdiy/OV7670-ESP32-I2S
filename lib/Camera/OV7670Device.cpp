//カメラ操作 - OV7670
//『昼夜逆転』工作室	@jsdiy	https://github.com/jsdiy
//	2024/04 - 2025/07

#include <Arduino.h>
#include <Wire.h>
#include "OV7670Device.hpp"

//コンストラクタ
OV7670Device::OV7670Device(void)
{
	width = height = 0;
	bytePerPixel = 0;
}

//初期化
void	OV7670Device::Initialize(void)
{
	using	namespace	CameraConfig;	//CamPinのため

	//SCCB通信開始
	//・カメラはクロックを供給されていないと通信できないので、通信するまでにXCLKを出力すること。
	Wire.begin(CamPin::SDA, CamPin::SCL, 400000UL);
	SupplyXclk();

	//GPIO設定
	pinMode(CamPin::VSYNC, INPUT_PULLUP);
	pinMode(CamPin::PCLK, INPUT_PULLUP);
	pinMode(CamPin::HREF, INPUT_PULLUP);
	pinMode(CamPin::D0, INPUT_PULLUP);	pinMode(CamPin::D1, INPUT_PULLUP);
	pinMode(CamPin::D2, INPUT_PULLUP);	pinMode(CamPin::D3, INPUT_PULLUP);
	pinMode(CamPin::D4, INPUT_PULLUP);	pinMode(CamPin::D5, INPUT_PULLUP);
	pinMode(CamPin::D6, INPUT_PULLUP);	pinMode(CamPin::D7, INPUT_PULLUP);
}

//カメラの動作モードを設定する
//引数	xclkPreScale:	VGA:12～64, QVGA:3～64, QQVGA:1～64
void	OV7670Device::DeviceConfigure(ECamResolution resolution, ECamColorMode colorMode, uint8_t xclkPreScale)
{
	//レジスタをリセットする
	SoftwareReset();

	//レジスタ初期値をセットする
	WriteRegisterList(OV7670_default_regs);

	//解像度をセットする
	SetResolution(resolution);

	//カラーモードをセットする
	SetColorMode(colorMode);

	//内部クロックをセットする
	SetClockDiv(xclkPreScale);
}

//クロックを供給する
void	OV7670Device::SupplyXclk(void)
{
	using	namespace	CameraConfig;	//CamPinのため

	constexpr	uint32_t	pwmFrequency = 16UL * 1000000;	//XCLKへの入力(10～48MHz)
	constexpr	uint8_t	pwmResolutionBits = 2;	//pwmFrequencyによって決める（2: 20MHzまで生成可）
	constexpr	uint8_t	pwmChannel = 0;			//内部のPWM生成チャンネル割り当て（必要に応じて変更可）
	constexpr	uint8_t	pwmDutyPercent = 50;
	uint32_t	dutyValue = std::pow(2, pwmResolutionBits) * pwmDutyPercent / 100;

	pinMode(CamPin::XCLK, OUTPUT);
	digitalWrite(CamPin::XCLK, LOW);
	ledcSetup(pwmChannel, pwmFrequency, pwmResolutionBits);
	ledcAttachPin(CamPin::XCLK, pwmChannel);
	ledcWrite(pwmChannel, dutyValue);

	/*	PWMの解像度（分解能）と最大周波数の関係
	pwmResolutionBits:1 = 分解能:2^1 -> 最大周波数:40MHz
	pwmResolutionBits:2 = 分解能:2^2 -> 最大周波数:20MHz
	pwmResolutionBits:3 = 分解能:2^3 -> 最大周波数:10MHz
	pwmResolutionBits:4 = 分解能:2^4 -> 最大周波数: 5MHz
	*/
	/*	XCLKの分周と逓倍について
	・OV7670の仕様は InputClock(XCLK): 10MHz <= (Typ.24MHz) <= 48MHz
	・OV7670がXCLKを1～64分周するよう設定可。これによりフレームレート(fps)を落とせる。
		- 分周はCLKRCレジスタの設定で「入力XCLK / (1～64)」。分母は1刻み。デフォルトは「入力XCLK / 1」。
			F_internalClock = F_inputClock / (bit[5:0] + 1)
	・PCLKはデフォルトではXCLKと同値（逓倍処理をバイパス）。
		- フレームレートを落としたい（＝画像のキャプチャが追いつかない）実行環境ではPCLKを逓倍する必要はない。
	*/
	/*	参考: XCLK=16MHz,smp_rate=300kでの分週別fps
	QQVGA:	XCLK=16MHz/分周なし,smp_rate=300k:20.0fps(416us/line)	※100k-NG
			XCLK=16MHz/2分周,smp_rate=300k:10.0fps(833us/line)	※100k-OK,fps同じ
			XCLK=16MHz/3分周,smp_rate=300k:6.66fps(1.25ms/line)
			XCLK=16MHz/4分周,smp_rate=300k:5.00fps
			XCLK=16MHz/5分周,smp_rate=300k:4.00fps
			XCLK=16MHz/8分周,smp_rate=300k:2.50fps
			XCLK=16MHz/10分周,smp_rate=300k:2.00fps
			XCLK=16MHz/12分周,smp_rate=300k:1.66fps
			XCLK=16MHz/16分周,smp_rate=300k:1.25fps(6.66ms/line)

	QVGA:	XCLK=16MHz/分周なし:正常に表示できず
			XCLK=16MHz/2分周:正常に表示できず
			XCLK=16MHz/3分周,smp_rate=300k:6.66fps(625us/line)	※全て100k-OK,fps同じ
			XCLK=16MHz/4分周,smp_rate=300k:5.00fps
			XCLK=16MHz/5分周,smp_rate=300k:4.00fps
			XCLK=16MHz/6分周,smp_rate=300k:3.33fps
			XCLK=16MHz/7分周,smp_rate=300k:2.86fps
			XCLK=16MHz/8分周,smp_rate=300k:2.50fps
			XCLK=16MHz/10分周,smp_rate=300k:2.00fps
			XCLK=16MHz/12分周,smp_rate=300k:1.66fps
			XCLK=16MHz/15分周,smp_rate=300k:1.33fps
			XCLK=16MHz/16分周,smp_rate=300k:1.25fps(3.33ms/line)

	VGA:	XCLK=16MHz/11分周,smp_rate=300k:正常に表示できず
			XCLK=16MHz/12分周,smp_rate=300k:1.66fps
			XCLK=16MHz/13分周,smp_rate=300k:1.54fps
			XCLK=16MHz/14分周,smp_rate=300k:1.43fps
			XCLK=16MHz/15分周,smp_rate=300k:1.33fps
			XCLK=16MHz/16分周,smp_rate=300k:1.25fps
			XCLK=16MHz/20分周,smp_rate=300k:1.00fps
			※QVGAのLCDへ擬似的に表示して確認
	*/
}

//レジスタを読み込む
uint8_t	OV7670Device::ReadRegister(uint8_t reg)
{
	delayMicroseconds(5);
	Wire.beginTransmission(SlaveAddress);
	Wire.write(reg);
	Wire.endTransmission(true);

	delayMicroseconds(5);
	Wire.requestFrom(SlaveAddress, static_cast<size_t>(1), true);
	uint8_t data = static_cast<uint8_t>(Wire.read());
	return data;

	/*	【SCCB通信の注意点】
	・OV7670_DS(v1.4) - P6 - Table 4. Functional and AC Characteristics
		SCCB Timing - BUF Bus free time before new START - [Min] 1.3 μs
		→新しいスタート前のバスの空き時間：最小1.3us
	・SCCBSpec_AN(v2.2) - 3.1.1 Start of Data Transmission
		START時、前回のSTOPから1.25us以上経過している必要がある
	よって、readもwriteもSTART前に時間待ちすれば確実
	*/
}

//レジスタに書き込む
void	OV7670Device::WriteRegister(uint8_t reg, uint8_t dat)
{
	delayMicroseconds(5);
	Wire.beginTransmission(SlaveAddress);
	Wire.write(reg);
	Wire.write(dat);
	Wire.endTransmission(true);
}

//レジスタに書き込む
void	OV7670Device::WriteRegisterList(const regval_list* list)
{
	while (1)
	{
		uint8_t addr = list->reg_num;
		uint8_t data = list->value;
		if ((addr == 0xFF) && (data == 0xFF)) { break; }
		WriteRegister(addr, data);
		list++;
	}
}

//レジスタをリセットする
void	OV7670Device::SoftwareReset(void)
{
	//レジスタのリセット完了まで最大1msかかる（データシートより）
	WriteRegister(REG_COM7, COM7_RESET);
	delayMicroseconds(5);
}

//製品IDとバージョンを取得する
//・SCCB通信テストとしての利用を想定。
uint16_t	OV7670Device::GetProductID(void)
{
	uint16_t pid = ReadRegister(REG_PID);	//0x76（OV7670固有の値）
	uint16_t ver = ReadRegister(REG_VER);	//0x73（他のバージョンは出回っていない模様）
	//uint8_t midh = ReadRegister(REG_MIDH);	//製造者ID_Hi
	//uint8_t midl = ReadRegister(REG_MIDL);	//製造者ID_Lo

	return ((pid << 8) | ver);
}

//解像度に応じたwindowサイズと内部クロックを設定する
void	OV7670Device::SetResolution(ECamResolution camRes)
{
	uint8_t val = ReadRegister(REG_COM7);
	val &= ~COM7_FMT_MASK;	//bit[5:3]

	switch (camRes)
	{
	case	ECamResolution::VGA:
		val |= COM7_FMT_VGA;
		WriteRegister(REG_COM7, val);
		WriteRegisterList(OV7670_vga);
		SetWindow(158, 10);
		width = 640;	height = 480;
		//SetClockDiv(64 - 1);	※DeviceConfigure()で設定するよう変更した
		break;

	case	ECamResolution::QVGA:
		val |= COM7_FMT_VGA;
		WriteRegister(REG_COM7, val);
		WriteRegisterList(OV7670_qvga);
		SetWindow(176, 12);
		width = 320;	height = 240;

		/*	ATmega328P@16MHz
		//内部クロックをセットする:QVGA	
		//ポーリング方式で1行ごとに、
		//・描画のみ			(19～64)-1
		//・データ保存のみ		(23～64)-1
		//・描画＋データ保存	(41～64)-1
		//SetClockDiv(23 - 1);
		*/
		/*
		//ESP32@XCLK=10MHz,割り込み＋タスク方式
		SetClockDiv(36 - 1);	//(35～64)-1
		*/
		//ESP32 I2S＋タスク＋SPI_DMA方式
		//XCLK=16MHz/3分周で動作する
		//SetClockDiv(3);	//3～64		※DeviceConfigure()で設定するよう変更した
		break;

	default:
	case	ECamResolution::QQVGA:
		val |= COM7_FMT_VGA;
		WriteRegister(REG_COM7, val);
		WriteRegisterList(OV7670_qqvga);
		SetWindow(184, 10);
		width = 160;	height = 120;

		//内部クロックをセットする:QQVGA
		/*	ATmega328P@16MHz
		//ポーリング方式で1行ごとに、
		//・描画のみ			(4～64)-1
		//・データ保存のみ		(5～64)-1
		//・描画＋データ保存	(8～64)-1
		SetClockDiv(5 - 1);
		*/
		/*
		//ESP32@XCLK=10MHz,割り込み＋タスク＋SPI_DMA方式
		SetClockDiv(15 - 1);	//(15～64)-1	※-Osと-O2とで変わらず
		*/
		//ESP32 I2S＋タスク＋SPI_DMA方式
		//XCLK=16MHz/分周なしで動作する
		//SetClockDiv(1);	//1～64		※DeviceConfigure()で設定するよう変更した
		break;
	}
}

//CLKRCレジスタをリフレッシュする
void	OV7670Device::RefreshCLKRC(void)
{
	uint8_t val = ReadRegister(REG_CLKRC);
	WriteRegister(REG_CLKRC, val);
}

//レジスタ設定：ウインドウ
/*
・VGA,QVGA,QQVGAを想定。
・QVGA,QQVGAであってもVGAの縦横サイズを設定する（ダウンサンプリング元の解像度を設定する）
・VGA系では次式の関係がある（CIF系はCIFの縦横サイズで計算）
	hStop = hStart + 640 - 784;
	vStop = vStart + 480;

「784」の出どころは、
・データシート(v1.4)
	Figure 6. VGA Frame Timing
		HREF: t_Line = 784t_P
・OV7670 Implementation Guide (V1.0) - P14
	Table 3-4. Dummy Pixel and Row
		Dummy Pixel： 1 digital count is equal to 1/784 row period
・OV7670 Implementation Guide (V1.0) - P15
	The OV7670/OV7171 array always outputs VGA resolution so the row interval is:
		tROW INTERVAL = 2 x (784 + Dummy Pixels) x tINT CLK
*/
void	OV7670Device::SetWindow(uint16_t hStart, uint16_t vStart)
{
	uint8_t val;

	//HSTART,HSTOPにそれぞれ上位8ビットを設定する。HREFにそれぞれの下位3ビットを設定する。
	uint16_t hStop = (hStart + 640) % 784;
	uint8_t valHStart = (hStart >> 3) & 0xFF;
	uint8_t valHStop = (hStop >> 3) & 0xFF;
	uint8_t valHref = ((hStop & 0x07) << 3) | ((hStart & 0x07) << 0);	//注意：stopが上位ビット、startが下位ビット
	WriteRegister(REG_HSTART, valHStart);
	WriteRegister(REG_HSTOP, valHStop);
	val = ReadRegister(REG_HREF);
	val &= ~(HREF_HSP_MASK | HREF_HST_MASK);	//HSTOP,HSTARTのマスク
	val |= valHref;
	WriteRegister(REG_HREF, val);

	//VSTART,VSTOPにそれぞれ上位8ビットを設定する。VREFにそれぞれの下位2ビットを設定する。
	uint16_t vStop = vStart + 480;
	uint8_t valVStart = (vStart >> 2) & 0xFF;
	uint8_t valVStop = (vStop >> 2) & 0xFF;
	uint8_t valVref = ((vStop & 0x03) << 2) | ((vStart & 0x03) << 0);	//注意：stopが上位ビット、startが下位ビット
	WriteRegister(REG_VSTART, valVStart);
	WriteRegister(REG_VSTOP, valVStop);
	val = ReadRegister(REG_VREF);
	val &= ~(VREF_VSP_MASK | VREF_VST_MASK);	//VSTOP,VSTARTのマスク
	val |= valVref;
	WriteRegister(REG_VREF, val);
}

//カラーモードを設定する
void	OV7670Device::SetColorMode(ECamColorMode colMode)
{
	uint8_t val = ReadRegister(REG_COM7);
	val &= ~COM7_COLOR_MASK;	//bit[2,0]

	switch (colMode)
	{
	default:
	case	ECamColorMode::RGB565:
		val |= COM7_RGB;
		WriteRegister(REG_COM7, val);
		WriteRegisterList(OV7670_rgb565);
		bytePerPixel = 2;
		break;

	case	ECamColorMode::YUYV:
		val |= COM7_YUV;
		WriteRegister(REG_COM7, val);
		WriteRegisterList(OV7670_yuv422);
		bytePerPixel = 2;
		break;
	}
}

//内部クロックを設定する
//引数:	XCLKの分周値	1分周(分周なし), 2分周, 3分周, …, 64分周
void	OV7670Device::SetClockDiv(uint8_t xclkPreScale)
{
	//分周値(1～64)をレジスタ値(0～63)に変換する
	if (xclkPreScale == 0) { xclkPreScale = 1; }
	xclkPreScale = xclkPreScale - 1;

	//分周値	F(internal clock) = F(input clock) / (Bit[5:0] + 1)
	uint8_t val = ReadRegister(REG_CLKRC);
	WriteRegister(REG_CLKRC, (val & ~CLKRC_PRESCALE_MASK) | xclkPreScale);

	//逓倍値	DBLV_BYPASS, DBLV_CLK_x4, _x6, _x8
	val = ReadRegister(REG_DBLV);
	WriteRegister(REG_DBLV, (val & ~DBLV_CLK_MASK) | DBLV_BYPASS);
	
	RefreshCLKRC();
}

//カラーバーを表示する／しない
void	OV7670Device::ColorBar(bool isOn)
{
	uint8_t val = ReadRegister(REG_COM17);
	val = isOn ? (val | COM17_CBAR) : (val & ~COM17_CBAR);
	WriteRegister(REG_COM17, val);
}
//
void	OV7670Device::ColorBarTr(bool isOn)
{
	uint8_t val = ReadRegister(REG_COM7);
	val = isOn ? (val | COM7_CBAR) : (val & ~COM7_CBAR);
	WriteRegister(REG_COM7, val);
}

//水平・垂直反転
//引数	ERegVal::MVFP_MIRROR:	Mirror image
//		ERegVal::MVFP_FLIP:		Vertical flip
//		MVFP_MIRROR|MVFP_FLIP:	rotate 180-degree
//		0:	normal image
void	OV7670Device::Flip(ERegVal mvfp)
{
	uint8_t val = ReadRegister(REG_MVFP);
	val &= ~MVFP_FLIP_MASK;
	val |= (mvfp & MVFP_FLIP_MASK);
	WriteRegister(REG_MVFP, val);
}

//AWB(Automatic White Balance)：現在の設定値を取得する	
void	OV7670Device::GetAwbGain(uint8_t* gainR, uint8_t* gainG, uint8_t* gainB)
{
	*gainG = ReadRegister(REG_GGAIN);	// AWB Green gain	※default:00
	*gainB = ReadRegister(REG_BLUE);	// AWB Blue gain (00-ff)default:80
	*gainR = ReadRegister(REG_RED);		// AWB Red gain (00-ff)default:80
}

//AWB(Automatic White Balance)：ゲインを設定する
void	OV7670Device::SetAwbGain(uint8_t gainR, uint8_t gainG, uint8_t gainB)
{
	WriteRegister(REG_GGAIN, gainG);	// AWB Green gain	※default:00
	WriteRegister(REG_BLUE, gainB);		// AWB Blue gain (00-ff)default:80
	WriteRegister(REG_RED, gainR);		// AWB Red gain (00-ff)default:80
}

//明るさ	デフォルト値は0x00
uint8_t	OV7670Device::GetBrightness(void) { return ReadRegister(REG_BRIGHT); }
void	OV7670Device::SetBrightness(uint8_t val) { WriteRegister(REG_BRIGHT, val); }

//濃さ	デフォルト値は0x40
uint8_t	OV7670Device::GetContrast(void) { return ReadRegister(REG_CONTRAS); }
void	OV7670Device::SetContrast(uint8_t val) { WriteRegister(REG_CONTRAS, val); }
