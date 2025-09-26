# ESP32用OV7670ドライバー

## 概要
カメラOV7670（FIFOなし）のESP32用ドライバーです。
既存のESP32用ドライバーと比べて構造がシンプルなので設定が簡単、安定して動作します。
技術的な説明はソースコードおよび、ソースコード中のコメントを見てください。

## 特徴
PratformIOにインストールされるArduino-ESP32(3.x)環境で利用できるI2Sライブラリは<driver/i2s.h>です。
この制限の中、シンプルな記述でI2S-Cameraモードを動かしています。
- QQVGA:20.00fps
- QVGA:6.66fps
- VGA:1.66fps

## 使い方
- PratformIOで適当なプロジェクトを作成し、<lib>フォルダとmain.cppをプロジェクトへコピーしてください。
- OV7670Device.hpp, LcdST77xx.hpp のピンアサインに基づき、ESP32-DevKitC-V4とOV7670とLCDを配線してください。
- ビルドし、転送すると、デモンストレーションを見ることができます。
  ### 動画
  『昼夜逆転』工作室 (@jsdiy) <a href="https://twitter.com/jsdiy/status/1971547894042984603">September 26, 2025</a>

## 動作環境
- PratformIO + Arduino Framework
- ESP32-DevKitC-V4
- OV7670（FIFOなし）
- SPI-LCD(ST7789/QVGA)

### 開発経緯
既存のESP32用OV7670ドライバーは大抵複雑な構造をしており、
複雑な（あるいはシビアな）タイミング制御で画素データをキャプチャしています。
それゆえカメラ制御の調整が難しく、また、動作しないドライバーの修正も容易ではありません。
このような状態なので、Arduino-ESP32環境で安定動作するシンプルな構造のドライバーを作りました。
