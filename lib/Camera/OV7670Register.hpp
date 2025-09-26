//OV7670 Register
//『昼夜逆転』工作室	https://github.com/jsdiy
//	2025/03 - 2025/07	@jsdiy

//このファイルの内容は下記リンク先のlinux版をベースに、記述を整理したもの
//	https://github.com/torvalds/linux/blob/master/drivers/media/i2c/ov7670.c

#pragma	once

#include <Arduino.h>

namespace	OV7670RegVal
{

//レジスタ値構造体
struct	regval_list
{
	uint8_t	reg_num;
	uint8_t	value;
};

extern	const	regval_list	OV7670_default_regs[];
extern	const	regval_list	OV7670_rgb565[];
extern	const	regval_list	OV7670_yuv422[];
extern	const	regval_list	OV7670_vga[];
extern	const	regval_list	OV7670_qvga[];
extern	const	regval_list	OV7670_qqvga[];

//Register Address, Bit Value
enum	ERegVal	: uint8_t
{
	/*
	* A V4L2 driver for OmniVision OV7670 cameras.
	*
	* Copyright 2006 One Laptop Per Child Association, Inc.  Written
	* by Jonathan Corbet with substantial inspiration from Mark
	* McClelland's ovcamchip code.
	*
	* Copyright 2006-7 Jonathan Corbet <corbet@lwn.net>
	*/
	/*
	MODULE_AUTHOR("Jonathan Corbet <corbet@lwn.net>");
	MODULE_DESCRIPTION("A low-level driver for OmniVision ov7670 sensors");
	MODULE_LICENSE("GPL");
	*/

	REG_GAIN	= 0x00,	// AGC[9:0] lower 8bit (higher 2bit VREF[7:6])
	REG_BLUE	= 0x01,	// AWB Blue gain (00-ff)
	REG_RED		= 0x02,	// AWB Red gain (00-ff)
	REG_VREF	= 0x03,	// Pieces of AGC[9:8], VSTOP[1:0], VSTART[1:0]
		VREF_VSP_MASK	= 0x0C,	//VSTOPのマスク
		VREF_VST_MASK	= 0x03,	//VSTARTのマスク
	REG_COM1	= 0x04,	// Control 1
		COM1_CCIR656	= 0x40,	// CCIR656 enable
	REG_BAVE	= 0x05,	// U/B Average level
	REG_GbAVE	= 0x06,	// Y/Gb Average level
	REG_AECHH	= 0x07,	// AEC MS 5 bits
	REG_RAVE	= 0x08,	// V/R Average level
	REG_COM2	= 0x09,	// Control 2
		COM2_SSLEEP		= 0x10,	// Soft sleep mode
		COM2_OUT_DRIVE_1x	= 0x00,	// Output drive capability 1x
		COM2_OUT_DRIVE_2x	= 0x01,	// Output drive capability 2x
		COM2_OUT_DRIVE_3x	= 0x02,	// Output drive capability 3x
		COM2_OUT_DRIVE_4x	= 0x03,	// Output drive capability 4x
	REG_PID		= 0x0a,	// Product ID MSB
	REG_VER		= 0x0b,	// Product ID LSB
	REG_COM3	= 0x0c,	// Control 3
		COM3_SWAP		= 0x40,	// Byte swap
		COM3_SCALEEN	= 0x08,	// Enable Scaling
		COM3_DCWEN		= 0x04,	// Enable Downsampling/Dropping/Windowing
	REG_COM4	= 0x0d,	// Control 4
		COM4_AEC_FULL	= 0x00,	// AEC evaluate full window
		COM4_AEC_1_2	= 0x10,	// AEC evaluate 1/2 window
		COM4_AEC_1_4	= 0x20,	// AEC evaluate 1/4 window
		COM4_AEC_2_3	= 0x30,	// AEC evaluate 2/3 window
	REG_COM5	= 0x0e,	// All "reserved"
	REG_COM6	= 0x0f,	// Control 6
		COM6_BLK_LINE	= 0x80,	// Enable HREF at optional black
		COM6_RESET_TIM	= 0x02,	// Reset all timing when format changes
		COM6_RESERVE	= 0x41,	// Reserved bit
	REG_AECH	= 0x10,	// More bits of AEC value
	REG_CLKRC	= 0x11,	// Clocl control
		CLK_RSVD	= 0x80,	// Reserved
		CLK_EXT		= 0x40,	// Use external clock directly
		CLK_SCALE	= 0x3f,	// Mask for internal clock scale
		CLKRC_PRESCALE_MASK	= 0x3F,	//分周ビットのマスク
	REG_COM7	= 0x12,	// Control 7
		COM7_RESET	= 0x80,	// Register reset
		COM7_FMT_MASK	= 0x38,
		COM7_FMT_VGA	= 0x00,
		COM7_FMT_CIF	= 0x20,	// CIF format
		COM7_FMT_QVGA	= 0x10,	// QVGA format
		COM7_FMT_QCIF	= 0x08,	// QCIF format
		COM7_COLOR_MASK	= 0x05,	// bit:2 | bit:0
		COM7_RGB	= 0x04,	// bits 0 and 2 - RGB format
		COM7_YUV	= 0x00,	// YUV
		COM7_BAYER	= 0x01,	// Bayer format
		COM7_PBAYER	= 0x05,	// "Processed bayer"
		COM7_CBAR	= 0x02,	// Color bar	半透明
	REG_COM8	= 0x13,	// Control 8
		COM8_FASTAEC	= 0x80,	// Enable fast AGC/AEC
		COM8_AECSTEP	= 0x40,	// Unlimited AEC step size
		COM8_BFILT		= 0x20,	// Band filter enable
		COM8_RSVD		= 0x08,	// Reserved fefault 1
		COM8_AGC		= 0x04,	// Auto gain enable
		COM8_AWB		= 0x02,	// Auto White Balance enable
		COM8_AEC		= 0x01,	// Auto exposure enable
	REG_COM9	= 0x14,	// Control 9- gain ceiling
		COM9_AGC_GAIN_2x	= 0x00,	// Automatic Gain Ceiling 2x
		COM9_AGC_GAIN_4x	= 0x10,	// Automatic Gain Ceiling 4x
		COM9_AGC_GAIN_8x	= 0x20,	// Automatic Gain Ceiling 8x
		COM9_AGC_GAIN_16x	= 0x30,	// Automatic Gain Ceiling 16x
		COM9_AGC_GAIN_32x	= 0x40,	// Automatic Gain Ceiling 32x
		COM9_AGC_GAIN_64x	= 0x50,	// Automatic Gain Ceiling 64x
		COM9_AGC_GAIN_128x	= 0x60,	// Automatic Gain Ceiling 128x
		COM9_AGC_GAIN_NOT	= 0x70,	// Automatic Gain Ceiling Not allowed
		COM9_FREEZE_AGC_AEC	= 0x01,
	REG_COM10	= 0x15,	// Control 10
		COM10_HSYNC		= 0x40,	// HSYNC instead of HREF
		COM10_PCLK_HB	= 0x20,	// PCLK does not toggle during horizontal blank
		COM10_PCLK_REV 	= 0x10,	// Reverse PCLK
		COM10_HREF_REV	= 0x08,	// Reverse HREF
		COM10_VS_LEAD	= 0x04,	// VSYNC on clock leading edge
		COM10_VS_NEG	= 0x02,	// VSYNC negative
		COM10_HS_NEG	= 0x01,	// HSYNC negative
	REG_HSTART	= 0x17,	// Horiz start high bits
	REG_HSTOP	= 0x18,	// Horiz stop high bits
	REG_VSTART	= 0x19,	// Vert start high bits
	REG_VSTOP	= 0x1a,	// Vert stop high bits
	REG_PSHFT	= 0x1b,	// Pixel delay after HREF
	REG_MIDH	= 0x1c,	// Manuf. ID high
	REG_MIDL	= 0x1d,	// Manuf. ID low
	REG_MVFP	= 0x1e,	// Mirror / vflip
		MVFP_MIRROR		= 0x20,	// Mirror image
		MVFP_FLIP		= 0x10,	// Vertical flip
		MVFP_BLACK_SUN	= 0x04,	// black sun enable
		MVFP_FLIP_MASK	= 0x30,	// MIRROR,FLIPのマスク
	REG_LAEC	= 0x1f,	// Reserved - Fine AEC Value - defines exposure value less than one row period
	REG_ADCCTR0	= 0x20,	// ADC range adjustment
	REG_ADCCTR1	= 0x21,	// Reserved
	REG_ADCCTR2	= 0x22,	// Reserved
	REG_ADCCTR3	= 0x23,	// Reserved
	REG_AEW		= 0x24,	// AGC upper limit
	REG_AEB		= 0x25,	// AGC lower limit
	REG_VPT		= 0x26,	// AGC/AEC fast mode op region
	REG_BBIAS	= 0x27,	// B Channel Signal Output Bias
	REG_GbBIAS	= 0x28,	// Gb Channel Signal Output Bias
	REG_EXHCH	= 0x2a,	// Dummy Pixel Insert MSB
	REG_EXHCL	= 0x2b,	// Dummy Pixel Insert LSB
	REG_RBIAS	= 0x2c,	// R Channel Signal Output Bias
	REG_ADVFL	= 0x2d,	// LSB of insert dummy rows in vertical direction (1 bit equals 1 row)
	REG_ADVFH	= 0x2e,	// MSB of insert dummy rows in vertical direction
	REG_YAVE	= 0x2f,	// Y/G Channel Average Value
	REG_HSYST	= 0x30,	// HSYNC rising edge delay
	REG_HSYEN	= 0x31,	// HSYNC falling edge delay
	REG_HREF	= 0x32,	// HREF pieces
		HREF_HSP_MASK	= 0x38,	// HSTOPのマスク
		HREF_HST_MASK	= 0x07,	// HSTARTのマスク
	REG_CHLF	= 0x33,	// Array Current Control	 ( Reserved )
	REG_ARBLM	= 0x34,	// Array Referrence Control	 ( Reserved )
	REG_ADC		= 0x37,	// ADC Control				 ( Reserved )
	REG_ACOM	= 0x38,	// ADC and Analog Common MOde  Control ( Reserved )
	REG_OFON	= 0x39,	// Reserved - ADC Offset Control ( Reserved )
	REG_TSLB	= 0x3a,	// Line Buffer Test Option
		TSLB_NEGATE	= 0x20,	// Negative image - see MANU & MANV
		TSLB_UVOUT	= 0x10,	// Use fixed UV value
		TSLB_YLAST	= 0x08,	// UYVY or VYUY - see com13
		TSLB_AUTO	= 0x01,	// Auto output window
	REG_COM11	= 0x3b,	// Control 11
		COM11_NIGHT		= 0x80,	// NIght mode enable
		COM11_NMFR		= 0x60,	// Two bit NM frame rate
		COM11_FR_BY_2	= 0x20,	// 1/2 of normal mode frame rate
		COM11_FR_BY_4	= 0x40,	// 1/4 of normal mode frame rate
		COM11_FR_BY_8	= 0x60,	// 1/8 of normal mode frame rate
		COM11_HZAUTO	= 0x10,	// Auto detect 50/60 Hz
		COM11_50HZ		= 0x08,	// Manual 50Hz select
		COM11_EXP		= 0x02,	// Exposure timing can be less than limit of banding filter when light is too strong
	REG_COM12	= 0x3c,	// Control 12
		COM12_HREF	= 0x80,	// HREF always
		COM12_RSVD	= 0x68,	// reserved bit
	REG_COM13	= 0x3d,	// Control 13
		COM13_GAMMA		= 0x80,	// Gamma enable
		COM13_UVSAT		= 0x40,	// UV saturation auto adjustment
		COM13_UVSWAP	= 0x01,	// V before U - w/TSLB
		COM13_RESV		= 0x08,	// reserved bit
	REG_COM14	= 0x3e,	// Control 14
		COM14_DCWEN			= 0x10,	// DCW/PCLK-scale enable
		COM14_MANUAL		= 0x08,	// Manual scaling enable
		COM14_PCLKDIV_1		= 0x00,	// PCLK Divided by 1
		COM14_PCLKDIV_2		= 0x01,	// PCLK Divided by 2
		COM14_PCLKDIV_4		= 0x02,	// PCLK Divided by 4
		COM14_PCLKDIV_8		= 0x03,	// PCLK Divided by 8
		COM14_PCLKDIV_16	= 0x04,	// PCLK Divided by 16
	REG_EDGE	= 0x3f,	// Edge enhancement factor
	REG_COM15	= 0x40,	// Control 15
		COM15_R10F0		= 0x00,	// Data range 10 to F0
		COM15_R01FE		= 0x80,	//			  01 to FE
		COM15_R00FF		= 0xc0,	//			  00 to FF
		COM15_RGB565	= 0x10,	// RGB565 output
		COM15_RGB555	= 0x30,	// RGB555 output
	REG_COM16	= 0x41,	// Control 16
		COM16_YUV_ENHANC	= 0x20,
		COM16_DE_NOISE		= 0x10,
		COM16_AWBGAIN		= 0x08,	// AWB gain enable
	REG_COM17	= 0x42,	// Control 17
		COM17_AECWIN	= 0xc0,	// AEC window - must match COM4
		COM17_AEC_FULL	= 0x00,	// AEC evaluate full window
		COM17_AEC_1_2	= 0x40,	// AEC evaluate 1/2 window
		COM17_AEC_1_4	= 0x80,	// AEC evaluate 1/4 window
		COM17_AEC_2_3	= 0xC0,	// AEC evaluate 2/3 window
		COM17_CBAR		= 0x08,	// DSP Color bar

	REG_AWBC1	= 0x43,	// AWB Control 1 (Reserved ?)
	REG_AWBC2	= 0x44,	// AWB Control 2 (Reserved ?)
	REG_AWBC3	= 0x45,	// AWB Control 3 (Reserved ?)
	REG_AWBC4	= 0x46,	// AWB Control 4 (Reserved ?)
	REG_AWBC5	= 0x47,	// AWB Control 5 (Reserved ?)
	REG_AWBC6	= 0x48,	// AWB Control 6 (Reserved ?)

	REG_4B		= 0x4b,	// UV average ebnable
		UV_AVR_EN		= 0x01,	// UV average enable
	REG_DNSTH	= 0x4c,	// De-noise Threshold
	REG_DM_POS	= 0x4d,	// Reserved - Dummy row position
/*
	REG_CMATRIX_BASE	= 0x4f
		CMATRIX_LEN		6
	REG_CMATRIX_SIGN	= 0x58
*/
	REG_MTX1	= 0x4f,	// Matrix Coefficient 1
	REG_MTX2	= 0x50,	// Matrix Coefficient 2
	REG_MTX3	= 0x51,	// Matrix Coefficient 3
	REG_MTX4	= 0x52,	// Matrix Coefficient 4
	REG_MTX5	= 0x53,	// Matrix Coefficient 5
	REG_MTX6	= 0x54,	// Matrix Coefficient 6

	REG_BRIGHT	= 0x55,	// Brightness
	REG_CONTRAS	= 0x56,	// Contrast control
	REG_CONTRAS_CENTER	= 0x57,	// Contrast Center
	REG_MTXS	= 0x58,	// Matrix Coefficient Sign
	REG_AWBC7	= 0x59,	// AWB Control 7
	REG_AWBC8	= 0x5a,	// AWB Control 8
	REG_AWBC9	= 0x5b,	// AWB Control 9
	REG_AWBC10	= 0x5c,	// AWB Control 10
	REG_AWBC11	= 0x5d,	// AWB Control 11
	REG_AWBC12	= 0x5e,	// AWB Control 12
	REG_B_LMT	= 0x5f,	// AWB B Gain Range
	REG_R_LMT	= 0x60,	// AWB R Gain Range
	REG_G_LMT	= 0x61,	// AWB G Gain Range
	REG_LCC1	= 0x62,	// Lens Correction Option 1 - X Coordinate
	REG_LCC2	= 0x63,	// Lens Correction Option 2 - Y Coordinate
	REG_LCC3	= 0x64,	// Lens Correction Option 3
	REG_LCC4	= 0x65,	// Lens Correction Option 4
	REG_LCC5	= 0x66,	// Lens Correction Control
	REG_MANU	= 0x67,	// Manual U Value
	REG_MANV	= 0x68,	// Manual V Value
	REG_GFIX	= 0x69,	// AWB Pre gain control
	REG_GGAIN	= 0x6a,	// G Channel AWB Gain
	REG_DBLV	= 0x6b,	// PLL control,Regulator control
		DBLV_BYPASS		= 0x00,	// Bypass PLL	※clock x1 ということ
		DBLV_CLK_x4		= 0x40,	// input clock x4
		DBLV_CLK_x6		= 0x80,	// input clock x6
		DBLV_CLK_x8		= 0xC0,	// input clock x8
		DBLV_CLK_MASK	= 0xC0,	//CLKのマスク
		DBLV_RSVD		= 0x0A,	// reserved bit
	REG_AWBCTR3	= 0x6c,	// AWB Control 3
	REG_AWBCTR2	= 0x6d,	// AWB Control 2
	REG_AWBCTR1	= 0x6e,	// AWB Control 1
	REG_AWBCTR0	= 0x6f,	// AWB Control 0
	REG_SCALING_XSC	= 0x70,	// test pattern, Horizontal scale factor
	REG_SCALING_YSC	= 0x71,	// test pattern, Vertical scale factor
	REG_SCALING_DCWCTR	= 0x72,	// DCW Control
		SCALING_DCWCTR_VDS_by_2	= 0x10,	// Vertical Down Sampling rate by 2
		SCALING_DCWCTR_VDS_by_4	= 0x20,	// Vertical Down Sampling rate by 4
		SCALING_DCWCTR_VDS_by_8	= 0x30,	// Vertical Down Sampling rate by 8
		SCALING_DCWCTR_HDS_by_2	= 0x01,	// Horizontal Down Sampling rate by 2
		SCALING_DCWCTR_HDS_by_4	= 0x02,	// Horizontal Down Sampling rate by 2
		SCALING_DCWCTR_HDS_by_8	= 0x03,	// Horizontal Down Sampling rate by 2

	REG_SCALING_PCLK_DIV	= 0x73,	// Clock divider control for DSP scale
		SCALING_PCLK_DIV_RSVD	= 0xf0,	// Reserved
		SCALING_PCLK_DIV_DIS	= 0x08,	// Bypass clock divider
		SCALING_PCLK_DIV_1		= 0x00,	// Divided by 1
		SCALING_PCLK_DIV_2		= 0x01,	// Divided by 2
		SCALING_PCLK_DIV_4		= 0x02,	// Divided by 4
		SCALING_PCLK_DIV_8		= 0x03,	// Divided by 8
		SCALING_PCLK_DIV_16		= 0x04,	// Divided by 16
	REG_REG74	= 0x74,	// Digital gain manual control
	REG_REG75	= 0x75,	// Edge enhanced lower limit
	REG_REG76	= 0x76,	// OV's name
		R76_BLKPCOR		= 0x80,	// Black pixel correction enable
		R76_WHTPCOR		= 0x40,	// White pixel correction enable
	REG_REG77	= 0x77,	// Offset, de-noise range control

// 0x7a - 0x89 Ganma Curve registor
	REG_SLOP	= 0x7a,	// SLOP = (256-GAM15)x40/30
	REG_GAM1	= 0x7b,	// XREF1 4
	REG_GAM2	= 0x7c,	// XREF2 8
	REG_GAM3	= 0x7d,	// XREF3 16
	REG_GAM4	= 0x7e,	// XREF4 32
	REG_GAM5	= 0x7f,	// XREF5 40
	REG_GAM6	= 0x80,	// XREF6 48
	REG_GAM7	= 0x81,	// XREF7 56
	REG_GAM8	= 0x82,	// XREF8 64
	REG_GAM9	= 0x83,	// XREF9 72
	REG_GAM10	= 0x84,	// XREF10 80
	REG_GAM11	= 0x85,	// XREF11 96
	REG_GAM12	= 0x86,	// XREF12 112
	REG_GAM13	= 0x87,	// XREF13 144
	REG_GAM14	= 0x88,	// XREF14 176
	REG_GAM15	= 0x89,	// XREF15 208

	REG_RGB444	= 0x8c,	// RGB 444 control
		R444_DISABLE	= 0x00,
		R444_ENABLE		= 0x02,	// Turn on RGB444, overrides 5x5
		R444_RGBX		= 0x01,	// Empty nibble at end
	REG_DM_LNL	= 0x92,	// Dummy Row low 8bit
	REG_DM_LNH	= 0x93,	// Dummy Row high 8bit
	REG_LCC6	= 0x94,	// Lens Correction Optin 6
	REG_LCC7	= 0x95,	// Lens Correction Optin 7

	REG_BD50ST	= 0x9d,	// 50Hz Banding Filter Value
	REG_BD60ST	= 0x9e,	// 60Hz Banding Filter Value

	REG_HAECC1	= 0x9f,	// Hist AEC/AGC control 1
	REG_HAECC2	= 0xa0,	// Hist AEC/AGC control 2

	REG_SCALING_PCLK_DELAY	= 0xa2,	// Pixel Clock Delay

	REG_NT_CTRL	= 0xa4,	// Auto frame rate adjustment
		NT_CTRL_ROWPF	= 0x08,	// Auto frame rate adjust dummy row per frame
		NT_CTRL_DMR_2x	= 0x00,	// insert dummy row at 2x gain
		NT_CTRL_DMR_4x	= 0x01,	// insert dummy row at 4x gain
		NT_CTRL_DMR_8x	= 0x02,	// insert dummy row at 28 gain
	REG_BD50MAX	= 0xa5,	// 50hz banding step limit,	// Maximum Banding Filter Step
	REG_HAECC3	= 0xa6,	// Hist AEC/AGC control 3,	// Low Limit of Probability for HRL
	REG_HAECC4	= 0xa7,	// Hist AEC/AGC control 4,	// Upper Limit of Probability for LRL
	REG_HAECC5	= 0xa8,	// Hist AEC/AGC control 5,	// Probablility Threshold for LRL to control AEC/AGC speed
	REG_HAECC6	= 0xa9,	// Hist AEC/AGC control 6,	// Probablility Threshold for HRL to control AEC/AGC speed
	REG_HAECC7	= 0xaa,	// Hist AEC/AGC control 7,	// AEC Algorithm selection
	REG_BD60MAX	= 0xab,	// 60hz banding step limit
	REG_STR_OPT	= 0xac,	// R/G/B gain control
	REG_STR_R	= 0xad,	// R Gain for LED Output Frame
	REG_STR_G	= 0xae,	// G Gain for LED Output Frame
	REG_STR_B	= 0xaf,	// B Gain for LED Output Frame
	REG_ABLC1	= 0xb1,	// ABLC enable
		ABLC1_EN		= 0x04,	// ABLC enable
	REG_THL_ST	= 0xb3,	// ABLC Target
	REG_THL_DLT	= 0xb5,	// ABLC Stable Range
	REG_AD_CHB	= 0xbe,	// Blue Channel Black Level Compensation
	REG_AD_CHR	= 0xbf,	// Red Channel Black Level Compensation
	REG_AD_CHGb	= 0xc0,	// Gb Channel Black Level Compensation
	REG_AD_CHGr	= 0xc1,	// Gr Channel Black Level Compensation
	REG_SATCTR	= 0xc9,	// Saturation Control
};

//ビットOR演算子'|'のオーバーロード
//・func(EnumType n)に対して、func((EnumType)(A|B)) → func(A|B)と書ける。
//・constexprはinlineと同様に、関数（的なもの）の実装はヘッダーに記述する。
constexpr	ERegVal	operator | (ERegVal lhs, ERegVal rhs)
{
	return static_cast<ERegVal>(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

}	//namespace
