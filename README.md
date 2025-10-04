# ESP32用OV7670ドライバー

## 概要
OV7670 driver for ESP32. カメラOV7670（FIFOなし）のESP32用ドライバーです。
既存のESP32用ドライバーと比べて構造がシンプルなので設定が簡単、安定して動作します。
技術的な説明はソースコード中のコメントを見てください。

### 詳細記事
[https://jsdiy.github.io](https://jsdiy.github.io)　→　カメラモジュールOV7670ドライバー（ESP32用）

## 特徴
ESP32標準のI2Sライブラリを利用し、シンプルな記述でI2Sカメラモードを動かしています。
利用者はアプリケーションに応じてカメラのピンアサインとXCLKの分周値を設定するだけです。  
また、アプリケーション動作中に解像度を切り替えることができます。これによりx1,x2,x4ズームのような効果を得ることができます。
- QQVGA: 20.00fps
- QVGA: 6.66fps
- VGA: 1.66fps

## 使い方
- PlatformIOで適当なプロジェクトを作成し、＜lib＞フォルダとmain.cppをプロジェクトへコピーしてください。
- OV7670Device.hpp, LcdST77xx.hpp のピンアサインに基づき、ESP32-DevKitC-V4とOV7670とLCDを配線してください。
- ビルドし、ESP32へ書き込むと、デモンストレーションを見ることができます。

### 動画(Twitter/X)
[『昼夜逆転』工作室 (@jsdiy) September 26, 2025](https://twitter.com/jsdiy/status/1971547894042984603)

## 設定箇所
- OV7670Device.hpp
	- カメラのピンアサイン
	- フレーム速度(FPS)の調整値　※XCLKの分周値
- アプリケーション内
	- キャプチャした画素データを適宜処理する関数。関数名と引数名は任意。

## 動作確認作環境
- PlatformIO + ArduinoFramework
- ESP32-DevKitC-V4 (No PSRAM)
- OV7670 (No FIFO)
- SPI-LCD (ST7789/QVGA, ST7735/QQVGA)

### 開発経緯
PlatformIO/Arduino-ESP32環境/ESP32(PSRAMなし)で動作するOV7670(FIFOなし)用ドライバーはほぼ見付かりません。
Espressif製カメラドライバーの解析を試みて、改造や移植（ESP-IDFからArduinoFrameworkへ）は容易ではないと判断し、
自作することにしました。  
既存ドライバーが自前で記述しているI2S-DMA動作などの部分は
I2Sライブラリに含まれるi2s_driver_install()の処理内容と似ており、 工夫すれば置き換えられます。
引数i2s_config_tの.dma_buf_countと.dma_buf_lenを適切に設定することが重要です。
