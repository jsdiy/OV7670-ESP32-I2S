//カメラ操作 - OV7670
//『昼夜逆転』工作室	@jsdiy	https://github.com/jsdiy
//	2025/08 - 2025/09
/*
Camera
	+-- OV7670Device			:カメラの制御
	|		+-- OV7670Register	:カメラのレジスタ設定
	+-- I2sCamCapture			:画素データのキャプチャ、出力関数の呼び出し
*/

#pragma	once

#include <Arduino.h>
#include "OV7670Device.hpp"
#include "I2sCamCapture.hpp"

//利便性のためOV7670DeviceとI2sCamCaptureを統合的に扱えるようにしたクラス
class	Camera	: public OV7670Device, public I2sCamCapture
{
private:
	//フレーム速度（XCLKのn分周）
	uint8_t	GetSpeedLevel(ECamResolution resolution)
	{
		switch (resolution)
		{
		case	ECamResolution::VGA:	return CameraConfig::SpeedLevel::VGA;
		case	ECamResolution::QVGA:	return CameraConfig::SpeedLevel::QVGA;
		case	ECamResolution::QQVGA:	return CameraConfig::SpeedLevel::QQVGA;
		default:	break;
		}
		return 0;
	}

public:
	Camera(void) {}

	void	Initialize(void)
	{
		OV7670Device::Initialize();
		I2sCamCapture::Initialize();
	}

	void	Configure(ECamResolution resolution, ECamColorMode colorMode, FnCamPixelData func)
	{
		uint8_t speedLevel = GetSpeedLevel(resolution);
		DeviceConfigure(resolution, colorMode, speedLevel);
		CaptureConfigure(Width(), Height(), BytePerPixel(), func);
	}
};
