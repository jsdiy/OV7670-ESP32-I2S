//OV7670 Register
//『昼夜逆転』工作室	https://github.com/jsdiy
//	2025/03 - 2025/07	@jsdiy

//このファイルの内容は下記リンク先のlinux版をベースに、QVGA,QQVGA,RGB565に対応したもの
//※linux版はVGA,YUV固定
//	https://github.com/torvalds/linux/blob/master/drivers/media/i2c/ov7670.c

#include <Arduino.h>
#include "OV7670Register.hpp"

namespace	OV7670RegVal
{

//基本設定（解像度/カラーモードによらないレジスタ値）
const	regval_list	OV7670_default_regs[]	=
{
	//	デフォルト値は0x0D=0b00001101.	Bit[2:1](0bxxxxx10x)はReserved.
	//	Bit[3](UYVY or VYUY)とBit[0](Auto output window)を0とするので、0b00000100=0x04となる。
	{ REG_TSLB, 0x04 },	// Line Buffer Test Option

	/*	linux版はVGA固定なのでREG_COM3に0x00を設定しているが、解像度変更に対応する場合はDCWENを設定する必要あり。
		また、解像度変更時、REG_COM3はREG_COM14と共に設定するので、ここでは設定しないこととする。
	{ REG_COM3, COM3_DCWEN },
	*/

	//	デフォルト値0x80=0b10000000で、Bit[7]はReserved.
	//	Bit[5:0]は分周値。ここでは明示的に0x00（分周なし）を設定している。必要に応じて後から変更する。
	{ REG_CLKRC, CLK_RSVD | 0x00 },

	// Gamma curve values
	/*	linux版は下記となっているが、色味はディスプレイ側の調整や個人の好みによるので、
		ここでは設定しないこととする（デフォルト値とする）。
	{ REG_SLOP, 0x20 }, { REG_GAM1, 0x10 },
	{ REG_GAM2, 0x1e }, { REG_GAM3, 0x35 },
	{ REG_GAM4, 0x5a }, { REG_GAM5, 0x69 },
	{ REG_GAM6, 0x76 }, { REG_GAM7, 0x80 },
	{ REG_GAM8, 0x88 }, { REG_GAM9, 0x8f },
	{ REG_GAM10, 0x96 }, { REG_GAM11, 0xa3 },
	{ REG_GAM12, 0xaf }, { REG_GAM13, 0xc4 },
	{ REG_GAM14, 0xd7 }, { REG_GAM15, 0xe8 },
	*/

	// AGC and AEC parameters.  Note we start by disabling those features,
	//	then turn them only after tweaking the values.
	//	※ AGC(Auto Gain Celling), AEC(Auto Exposure(露出) Control)
	//{ REG_COM8, COM8_FASTAEC | COM8_AECSTEP | COM8_BFILT },	※REG_COM8を3回設定しているので、最後以外は削除する（画質に影響はなかった）
	{ REG_GAIN, 0x00 },	//※0x00はデフォルト値
	{ REG_AECH, 0x00 },	//※AEC値(16bit)のうちのAEC[9:2]をAECHレジスタにセットする。AEC[1:0]はCOM1レジスタ、AEC[15:10]はAECHHレジスタ。
	{ REG_COM4, 0x40 }, // magic reserved bit	※linux版ではCOM4/COM17ともAEC_FULLとなっている
	{ REG_COM9, COM9_AGC_GAIN_4x | 0x08 }, // 4x gain | magic rsvd bit

	{ REG_BD50MAX, 0x05 },	//※デフォルト値は0x0F
	{ REG_BD60MAX, 0x07 },	//※デフォルト値は0x0F

	{ REG_AEW, 0x95 },	{ REG_AEB, 0x33 },	//※デフォルト値はそれぞれ0x75, 0x63
	{ REG_VPT, 0xe3 },	{ REG_HAECC1, 0x78 },	//※デフォルト値はそれぞれ0xD4, 0xC0
	{ REG_HAECC2, 0x68 },	{ 0xa1, 0x03 },	// magic	//※HAECC2のデフォルト値は0x90
	{ REG_HAECC3, 0xd8 },	{ REG_HAECC4, 0xd8 },	//※デフォルト値はそれぞれ0xF0, 0xC1
	{ REG_HAECC5, 0xf0 },	{ REG_HAECC6, 0x90 },	//※デフォルト値はそれぞれ0xF0, 0xC1
	{ REG_HAECC7, 0x94 },	//※デフォルト値は0x14
	//{ REG_COM8, COM8_FASTAEC | COM8_AECSTEP | COM8_BFILT | COM8_AGC | COM8_AEC },

	// Almost all of these are magic "reserved" values.
	//	※OV7670では「reserved」は予約済というより「非公開」といった意味で使われている
	{ REG_COM5, 0x61 },	// Reserved
	{ REG_COM6, 0x4b },	// Reset all timing when format changes
	/*	デフォルト値はXXとなっている。無くても画質に影響は見られなかったので削除する。
	{ 0x16, 0x02 },		// Reserved
	*/
	{ REG_MVFP, 0x07 },	// 水平垂直反転	※linux版では反転なし
	/*	デフォルト値がXXだったり、bit[7:0]Reservedだったり。無くても画質に影響は見られなかったので削除する。
	{ REG_ADCCTR1, 0x02 },// Reserved
	{ REG_ADCCTR2, 0x91 },// Reserved
	{ 0x29, 0x07 }, { 0x33, 0x0b }, { 0x35, 0x0b }, { 0x37, 0x1d }, { 0x38, 0x71 }, { 0x39, 0x2a },	// All Reserved
	*/
	//	COM12レジスタのデフォルト値は0x68.
	//	差分であるBit[5]については、Bit[6:0]がReservedとなっているので理由不明。
	{ REG_COM12, 0x78 },
	/*	デフォルト値はXXとなっている。無くても画質に影響は見られなかったので削除する。
	{ 0x4d, 0x40 }, { 0x4e, 0x20 },	// Reserved
	*/
	/*	REG_GFIXのデフォルト値は0x00となっている。よって、設定する必要がないので削除する。
	{ REG_GFIX, 0x00 },	// AWB Pre gain control
	*/
	{ REG_DBLV, DBLV_BYPASS },	// PLL control,Regulator control
	//	デフォルト値は0x00（Digital gain が Bypass）. linux版は0x19で1x（1倍）の設定としている。
	{ REG_REG74, 0x19 },	// Digital gain manual control
	/*	デフォルト値はXXとなっている。無くても画質に影響は見られなかったので削除する。
	{ 0x8d, 0x4f }, { 0x8e, 0x00 }, { 0x8f, 0x00 }, { 0x90, 0x00 }, { 0x91, 0x00 },	// Reserved
	*/
	/*	REG_DM_LNL(Dummy Row low 8bit)のデフォルト値は0x00.	REG_DM_LNH(Dummy Row high 8bit)は設定していない。
		よって、REG_DM_LNLを0x00に設定する必要がないので削除する。
	{ REG_DM_LNL, 0x00 },
	*/
	/*	デフォルト値はXXとなっている。無くても画質に影響は見られなかったので削除する。
	{ 0x96, 0x00 }, { 0x9a, 0x80 },
	*/
	/*	0xb0,0xb2,0xb8はレジスタ名がなく(RSVD)、デフォルト値はXXとなっている。
		0xb0は、この設定を削除すると色が崩れる（赤・青が無くなったような色になる）ので削除しない。
		0xb2,0xb8は、無くても画質に影響は見られなかったので削除する。
	{ 0xb0, 0x84 },
	{ REG_ABLC1, 0x0c },
	{ 0xb2, 0x0e },
	{ REG_THL_ST, 0x82 },
	{ 0xb8, 0x0a },
	*/
	{ 0xb0, 0x84 },
	{ REG_ABLC1, 0x0c },	//REG_ABLC1=0xB1	※デフォルト値は0x00. linux版の設定値やコメントの値の理由は不明
	{ REG_THL_ST, 0x82 },	//REG_THL_ST=0xB3	※デフォルト値は0x80. linux版の設定値やコメントの値の理由は不明

	// More reserved magic, some of which tweaks white balance
	/*	linux版は下記となっているが、色味はディスプレイ側の調整や個人の好みによるので、
		ここでは設定しないこととする（デフォルト値とする）
	{ REG_AWBC1, 0x0A },	//AWB Control 1	
	{ REG_AWBC2, 0xf0 },	//AWB Control 2
	{ REG_AWBC3, 0x34 },	//AWB Control 3
	{ REG_AWBC4, 0x58 },	//AWB Control 4
	{ REG_AWBC5, 0x28 },	//AWB Control 5
	{ REG_AWBC6, 0x3a },	//AWB Control 6
	{ REG_AWBC7, 0x88 },	//AWB Control 7
	{ REG_AWBC8, 0x88 },	//AWB Control 8
	{ REG_AWBC9, 0x44 },	//AWB Control 9
	{ REG_AWBC10, 0x67 },	//AWB Control 10
	{ REG_AWBC11, 0x49 },	//AWB Control 11
	{ REG_AWBC12, 0x0e },	//AWB Control 12
	*/
	//	デフォルト値はAWBCTR3～0の順に0x02,0x55,0xC0,0x9A.
	//	linux版では下記だが、データシートに値の説明がないので、デフォルト値との差分の理由は不明
	//	※デフォルト値だと白が緑に強く寄った色味だった
	{ REG_AWBCTR3, 0x0a },
	{ REG_AWBCTR2, 0x55 },
	{ REG_AWBCTR1, 0x11 },
	{ REG_AWBCTR0, 0x9f },	// "9e for advance AWB"

	// linux版では下記	※デフォルト値だと白が緑に強く寄った色味だった
	{ REG_GGAIN,0x40 },	// AWB Green gain		   default:00
	{ REG_BLUE, 0x40 },	// AWB Blue gain	(00-ff)default:80
	{ REG_RED,	0x60 },	// AWB Red gain		(00-ff)default:80

	{ REG_COM8, COM8_FASTAEC | COM8_AECSTEP | COM8_BFILT | COM8_AGC | COM8_AEC | COM8_AWB },

	/*	linux版にはない設定だが、いくつかの独自ドライバに見られる設定
	・PCLKのポーリングや割り込みによって画素データをキャプチャする方式では、COM10_PCLK_HBを設定すると都合がよい。
	・ESP32でI2Sカメラモードのキャプチャ条件(VSYNC=Hi|…)をデバイス側で実現させる場合、COM10_VS_NEGを設定する。
		→GPIO Matrixで反転入力を設定するならCOM10_VS_NEGの設定は不要。
	{ REG_COM10, COM10_PCLK_HB | COM10_VS_NEG },	// PCLK does not toggle during horizontal blank & VSYNC negative
	*/

	/*	色変換マトリクスはカラーモード(RGB/YUV)により異なるので、
		カラーモード設定時に色変換マトリクスを設定することとする（ここでは省略する）
		※下記、linux版ではYUV時の色変換マトリクスが設定されている
	// Matrix coefficients
	{ REG_MTX1, 0x80 },	// Matrix Coefficient 1
	{ REG_MTX2, 0x80 },	// Matrix Coefficient 2
	{ REG_MTX3, 0x00 },	// Matrix Coefficient 3
	{ REG_MTX4, 0x22 },	// Matrix Coefficient 4
	{ REG_MTX5, 0x5e },	// Matrix Coefficient 5
	{ REG_MTX6, 0x80 },	// Matrix Coefficient 6
	{ REG_MTXS, 0x9e },	// Matrix Coefficient Sign
	*/

	{ REG_COM16, COM16_AWBGAIN },	// AWB gain enable
	{ REG_EDGE, 0x00 },	// Edge enhancement factor
	{ REG_REG75, 0x05 },	// Edge enhanced lower limit
	{ REG_REG76, 0xe1 },	// Edge enhanced higher limit ,Black/white pixcel correction enable
	{ REG_DNSTH, 0x00 },	// De-noise Threshold
	{ REG_REG77, 0x01 },	// Offset, de-noise range control
	{ REG_COM13, 0xC3 },	// Gamma enable, UV saturation auto adjustment	※bit[0]はカラーモードYUVのUV-swap
	/*	REG_4Bはbit[7:1]がReservedで、bit[0]は UV average enable とされている。
		レジスタのデフォルト値は0x00だが、Linux版は0x09を設定している（bit[3]が何の設定かは不明）。
		無くても画質に影響は見られなかったので削除する。
	{ REG_4B, 0x09 },
	*/
	{ REG_SATCTR, 0x60 },	// UV saturatin control min
	{ REG_COM16, COM16_YUV_ENHANC | COM16_DE_NOISE | COM16_AWBGAIN },
	{ REG_CONTRAS, 0x40 },	// Contrast Control

	/*	REG_ARBLMは全ビットがReservedでデフォルト値は0x11となっている。
		よって、設定する必要がないので削除する。
	{ REG_ARBLM, 0x11 },
	*/
	{ REG_COM11, COM11_EXP | COM11_HZAUTO },	// Exposure timing can be less than limit of banding filter when light is too strong	※50/60Hz自動検出
	{ REG_NT_CTRL, 0x80 | NT_CTRL_ROWPF | NT_CTRL_DMR_2x },	// Auto frame rate adjustment dummy row selection	※0x80はreservedされた値
	/*	Reservedレジスタ群。
		無くても画質に影響は見られなかったので削除する。
	{ 0x96, 0x00 }, { 0x97, 0x30 },
	{ 0x98, 0x20 }, { 0x99, 0x30 },
	{ 0x9a, 0x84 }, { 0x9b, 0x29 },
	{ 0x9c, 0x03 },
	*/
	/*	REG_COM11でCOM11_HZAUTOビットを立てているので、ここでデフォルト値(各0x99/0x7F)と異なる値を設定しても意味はない。
		無くても画質に影響は見られなかったので削除する。
	{ REG_BD50ST, 0x4c },
	{ REG_BD60ST, 0x3f },
	*/
	/*	0x78はレジスタ名がなく(RSVD)、デフォルト値はXXとなっている。無くても画質に影響は見られなかったので削除する。
	{ 0x78, 0x04 },
	*/

	/*	linux版は「超奇妙なもの」として内容が分からないまま下記を設定している。
		無くても画質に影響は見られなかったので削除する。
	// Extra-weird stuff.  Some sort of multiplexor register
	{ 0x79, 0x01 }, { 0xc8, 0xf0 },
	{ 0x79, 0x0f }, { 0xc8, 0x00 },
	{ 0x79, 0x10 }, { 0xc8, 0x7e },
	{ 0x79, 0x0a }, { 0xc8, 0x80 },
	{ 0x79, 0x0b }, { 0xc8, 0x01 },
	{ 0x79, 0x0c }, { 0xc8, 0x0f },
	{ 0x79, 0x0d }, { 0xc8, 0x20 },
	{ 0x79, 0x09 }, { 0xc8, 0x80 },
	{ 0x79, 0x02 }, { 0xc8, 0xc0 },
	{ 0x79, 0x03 }, { 0xc8, 0x40 },
	{ 0x79, 0x05 }, { 0xc8, 0x30 },
	{ 0x79, 0x26 },
	*/
	
	{ 0xff, 0xff }	// END MARKER
};

//RGB565
const	regval_list	OV7670_rgb565[]	=
{
	//{ REG_RGB444, 0x00 },	// No RGB444 please	※わざわざ書く必要なし
	{ REG_COM1, 0x00 },
	{ REG_COM15, COM15_R00FF | COM15_RGB565 },	// RGB565
	//{ REG_TSLB, 0x04 },	※default_regs[]で設定しているのでここでは不要
	//{ REG_COM9, 0x08 },	// 0x08 is reserved bit	※default_regs[]で設定しているのでここでは不要
	{ REG_MTX1, 0xb3 },	// matrix coefficient 1
	{ REG_MTX2, 0xb3 },	// matrix coefficient 2
	{ REG_MTX3, 0x00 },	// vb
	{ REG_MTX4, 0x3d },	// matrix coefficient 4
	{ REG_MTX5, 0xa7 },	// matrix coefficient 5
	{ REG_MTX6, 0xe4 },	// matrix coefficient 6
	{ REG_MTXS, 0x9e },	// Matrix Coefficient Sign
	{ REG_COM13, COM13_GAMMA | COM13_UVSAT },
	{ 0xff, 0xff }	// END MARKER
};

//YUV442
const	regval_list	OV7670_yuv422[]	=
{
	//{ REG_RGB444, 0x00 },	// No RGB444 please	※わざわざ書く必要なし
	{ REG_COM1, 0x00 },
	{ REG_COM15, COM15_R00FF },
	//{ REG_COM9, 0x08 },	// 0x08 is reserved bit	※default_regs[]で設定しているのでここでは不要
	{ REG_MTX1, 0x80 },	// matrix coefficient 1
	{ REG_MTX2, 0x80 },	// matrix coefficient 2
	{ REG_MTX3, 0x00 },	// vb
	{ REG_MTX4, 0x22 },	// matrix coefficient 4
	{ REG_MTX5, 0x5e },	// matrix coefficient 5
	{ REG_MTX6, 0x80 },	// matrix coefficient 6
	{ REG_MTXS, 0x9e },	// Matrix Coefficient Sign
	{ REG_COM13, COM13_GAMMA | COM13_UVSAT | COM13_UVSWAP },
	{ 0xff, 0xff }		// END MARKER
};

//VGA	(640x480)
const	regval_list	OV7670_vga[]	=
{
	{ REG_COM3,	0x00 },	// No scaling
	{ REG_COM14, 0x00 },
	{ REG_SCALING_XSC, 0x3a },	// Horizontal scale factor	//Mystery scaling numbers
	{ REG_SCALING_YSC, 0x35 },	// Vertical scale factor	//Mystery scaling numbers
	{ REG_SCALING_DCWCTR, 0x00 },	// Down sampling nothing
	{ REG_SCALING_PCLK_DIV, SCALING_PCLK_DIV_RSVD | SCALING_PCLK_DIV_1 },	// DSP scale control Clock divide by 1
	{ REG_SCALING_PCLK_DELAY, 0x02 },
	{ 0xff, 0xff }	// END MARKER
};

//QVGA	(320x240)
const	regval_list	OV7670_qvga[]	=
{
	{ REG_COM3, COM3_DCWEN},	// Enable format scaling	※REG_COM14とともに設定する
	{ REG_COM14, COM14_DCWEN | COM14_PCLKDIV_2 },	// divide by 2	※VGAの1/2縮小
	{ REG_SCALING_XSC, 0x3a },	// Horizontal scale factor	//Mystery scaling numbers
	{ REG_SCALING_YSC, 0x35 },	// Vertical scale factor	//Mystery scaling numbers
	{ REG_SCALING_DCWCTR, SCALING_DCWCTR_VDS_by_2 | SCALING_DCWCTR_HDS_by_2 },	// down sampling by 2
	{ REG_SCALING_PCLK_DIV, SCALING_PCLK_DIV_RSVD | SCALING_PCLK_DIV_2 },	// DSP scale control Clock divide by 2
	{ REG_SCALING_PCLK_DELAY, 0x02 },
	{ 0xff, 0xff }	// END MARKER
};

//QQVGA	(160x120)
const	regval_list OV7670_qqvga[]	=
{
	{ REG_COM3, COM3_DCWEN},	// Enable format scaling	※REG_COM14とともに設定する
	{ REG_COM14, COM14_DCWEN | COM14_MANUAL | COM14_PCLKDIV_4 },	// divide by 4	※VGAの1/4縮小
	{ REG_SCALING_XSC, 0x3a },	// Horizontal scale factor	//Mystery scaling numbers
	{ REG_SCALING_YSC, 0x35 },	// Vertical scale factor	//Mystery scaling numbers
	{ REG_SCALING_DCWCTR, SCALING_DCWCTR_VDS_by_4 | SCALING_DCWCTR_HDS_by_4 },	// down sampling by 4
	{ REG_SCALING_PCLK_DIV, SCALING_PCLK_DIV_RSVD | SCALING_PCLK_DIV_4 },	// DSP scale control Clock divide by 4
	{ REG_SCALING_PCLK_DELAY, 0x02 },
	{ 0xff, 0xff }	// END MARKER
};

}	//namespace
