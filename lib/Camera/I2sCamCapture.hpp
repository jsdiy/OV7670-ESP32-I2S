//カメラから画像データを受信・処理する
//『昼夜逆転』工作室	@jsdiy	https://github.com/jsdiy
//	2025/08 - 2025/10

#pragma	once

#include <Arduino.h>

//コールバック関数の定義（関数ポインタの定義と同等）
//・画素データが格納されたバッファを呼び出し元へ渡す。
using	FnCamPixelData = std::function<void(int16_t, uint8_t*, size_t)>;

//カメラから画像データを受信・処理する
class	I2sCamCapture
{
private:
	TaskHandle_t	hLineProcTask;
	bool	isI2sDriverInstalled;
	void	SetGpioMatrix(void);
	void	WaitForFrameEnd(void);
	void	I2sDriverInstall(int16_t width, int16_t bytePerPixel);
	void	CreateI2sReadBuffer(size_t pixelDataLength);

public:
	I2sCamCapture(void) { hLineProcTask = nullptr;	isI2sDriverInstalled = false; }
	void	Initialize(void);
	void	CaptureConfigure(int16_t width, int16_t heigth, int16_t bytePerPixel, FnCamPixelData func);
	void	CaptureStart(void);
	void	CaptureStop(void);
};
