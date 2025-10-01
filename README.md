# ESP32用OV7670ドライバー

## 概要
カメラOV7670（FIFOなし）のESP32用ドライバーです。
既存のESP32用ドライバーと比べて構造がシンプルなので設定が簡単、安定して動作します。
技術的な説明はソースコードおよび、ソースコード中のコメントを見てください。

## 特徴
PlatformIO＋Arduino-ESP32環境ではI2Sライブラリ＜driver/i2s.h＞が標準でインストールされています。
これを利用してシンプルな記述でI2Sカメラモードを動かしています。
- QQVGA: 20.00fps
- QVGA: 6.66fps
- VGA: 1.66fps

## 使い方
- PlatformIOで適当なプロジェクトを作成し、＜lib＞フォルダとmain.cppをプロジェクトへコピーしてください。
- OV7670Device.hpp, LcdST77xx.hpp のピンアサインに基づき、ESP32-DevKitC-V4とOV7670とLCDを配線してください。
- ビルドし、転送すると、デモンストレーションを見ることができます。
	### 動画(Twitter/X)
	[『昼夜逆転』工作室 (@jsdiy) September 26, 2025](https://twitter.com/jsdiy/status/1971547894042984603)

## 動作確認環境
- PlatformIO + Arduino Framework
- ESP32-DevKitC-V4
- OV7670 (FIFOなし)
- SPI-LCD (ST7789/QVGA)

	### 開発経緯
	PlatformIO/Arduino-ESP32環境/ESP32(PSRAMなし)で動作するOV7670(FIFOなし)用ドライバーはほぼ見付かりません。
	Espressif製カメラドライバーの解析を試みた際、改造や移植（ESP-IDFからArduinoFrameworkへ）は容易ではないと思いました。
	ESP32のI2Sカメラモードを調査しているうちに、既存ドライバーの内容はI2Sライブラリに含まれるi2s_driver_install()と似ていることが分かりました。
	そのようなわけで、上記条件で動作するシンプルなカメラドライバーを自作しました。  
	既存ドライバーが自前で記述しているI2S-DMA動作の部分などはi2s_driver_install()に置き換えることができます。
	その引数であるi2s_config_tでは特に.dma_buf_countと.dma_buf_lenを適切に設定することが重要です。

