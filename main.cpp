//カメラ操作 - OV7670
//『昼夜逆転』工作室	@jsdiy	https://github.com/jsdiy
//	2024/04 - 2025/09

/*	構成
	[OV7670  ] -- [ESP32     ] -- [SPI-LCD    ]
	 non-FIFO      DevKitC-V4      ST7789/QVGA
*/

#include <Arduino.h>
#include "Camera.hpp"
#include "LcdST77xx.hpp"

//オブジェクト
static	Camera	cam;
static	LcdST77xx	lcd;
static	Color	bgColor = Color::CreateRGB565(0x00, 0xFF, 0x00);	//色の定義

//変数
static	volatile	uint64_t	fpsDrawCount;	//test
static	ulong	fpsStartTime;	//test
static	ECamResolution	camReso;
static	ulong	prevTimeMSec;

//関数
void	APP_DrawLine(int16_t lineIndex, uint8_t* pixelData, size_t dataLength);
void	APP_DrawLineVGA(int16_t lineIndex, uint8_t* pixelData, size_t dataLength);
static	void	APP_ReportFps(uint16_t frameCount);

void	setup(void)
{
	Serial.begin(115200);
	delay(1000);

	//初期化
	cam.Initialize();
	lcd.Initialize();
	
	//LCD設定
	lcd.RotateFlip(ERotFlip::Rot270);
	lcd.ClearScreen(bgColor);	//表示テストを兼ねて白・黒以外で塗る

	//カメラ通信テスト
	Serial.printf("PID-VER(%02X-xx): %04X\n", OV7670Device::ProductID, cam.GetProductID());

	//カメラ設定
	camReso = ECamResolution::QQVGA;
	cam.Configure(camReso, ECamColorMode::RGB565, APP_DrawLine);	//QVGA,QQVGA
	cam.Flip(MVFP_FLIP);	//実験基板の都合により、LCDの設置方向にカメラ映像の出力方向を合わせている
	//cam.ColorBar(true);	//白,黄,水色(シアン),緑,紫(マゼンタ),赤,青,黒
	cam.CaptureStart();

	fpsDrawCount = 0;	//test
	fpsStartTime = millis();	//test
}

void	loop(void)
{
	//一定時間ごとに解像度を変更する
	ulong timeMSec = millis();
	if (10UL * 1000 < timeMSec - prevTimeMSec)
	{
		prevTimeMSec = timeMSec;
		cam.CaptureStop();
		Serial.println("CaptureStop()...");

		switch (camReso)
		{
		case	ECamResolution::QQVGA:
			camReso = ECamResolution::QVGA;
			cam.Configure(camReso, ECamColorMode::RGB565, APP_DrawLine);
			break;
		case	ECamResolution::QVGA:
			camReso = ECamResolution::VGA;
			cam.Configure(camReso, ECamColorMode::RGB565, APP_DrawLineVGA);
			break;
		case	ECamResolution::VGA:
			camReso = ECamResolution::QQVGA;
			cam.Configure(camReso, ECamColorMode::RGB565, APP_DrawLine);
			break;
		default:	break;
		}
		cam.Flip(MVFP_FLIP);	//実験基板の都合により、LCDの設置方向にカメラ映像の出力方向を合わせている
		lcd.ClearScreen(bgColor);

		fpsDrawCount = 0;	//test
		fpsStartTime = millis();	//test
		Serial.println("CaptureStart()");
		cam.CaptureStart();
	}

	//FPS調査
	if (fpsDrawCount == (cam.Height() * 10)) { APP_ReportFps(10); }	//10フレームごとに平均を求める
}

//タスクから呼び出される関数（1ライン分の画素データを受け取る）
void	APP_DrawLine(int16_t lineIndex, uint8_t* pixelData, size_t dataLength)
{
	lcd.DrawImage(0, lineIndex, cam.Width(), 1, pixelData, dataLength);
	fpsDrawCount++;
}
//
void	APP_DrawLineVGA(int16_t lineIndex, uint8_t* pixelData, size_t dataLength)
{
	//VGAのうち横方向は真ん中のQVGAの幅を表示する（x=0..639のうちx=160から幅320を描く）
	//	→それを2回描くことでVGAと同じ幅を描いたことにする
	//VGAのうち縦方向は2ラインをQVGAの1ラインに重ねて表示する（縦に圧縮された見た目になる）
	lineIndex /= 2;		//0,1,2,3,4,5,... -> 0,0,1,1,2,2,...
	size_t pos = dataLength / 4 * 1;
	dataLength /= 2;	//640*2 -> 320*2
	lcd.DrawImage(0, lineIndex, 320, 1, &pixelData[pos], dataLength);
	lcd.DrawImage(0, lineIndex, 320, 1, &pixelData[pos], dataLength);
	fpsDrawCount++;
}

//FPS調査
static	void	APP_ReportFps(uint16_t frameCount)
{
	auto drawMSec = millis() - fpsStartTime;	//nフレームにかかった時間(ms), n=frameCount
	float fps = (1000.0f * frameCount / drawMSec);	//1secあたりのフレーム数
	Serial.printf("fps=%f ", fps);
	fpsDrawCount = 0;
	fpsStartTime = millis();
}
