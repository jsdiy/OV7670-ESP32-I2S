//カメラから画像データを受信・処理する
//『昼夜逆転』工作室	@jsdiy	https://github.com/jsdiy
//	2025/08 - 2025/10

#include <Arduino.h>
#include <driver/i2s.h>
#include <driver/gpio.h>
#include "OV7670Device.hpp"	//namespace CameraConfig::CamPinのため
#include "I2sCamCapture.hpp"

//タスク
static	void	IRAM_ATTR	PixelDataProcessTask(void* pvParameters);

//タスクで参照する変数
static	int16_t	pixelDataLength;	//描画上の1ライン分のデータサイズ(byte)	※RGB565ならwidth*2
static	size_t	i2sDmaBufferLength;	//I2S-DMAバッファ上の（I2S-DMAフォーマットの）1ライン分の画素データサイズ
static	uint8_t*	i2sReadBuffer;	//i2s_read()で読み出すデータを格納するバッファ	※中身は気にせずbyte型配列として受け取ることとする
static	int16_t	camFrameHeight;
static	int16_t	lineIndex;
static	FnCamPixelData	FuncCB;

//初期化
void	I2sCamCapture::Initialize(void)
{
	pixelDataLength = 0;
	i2sDmaBufferLength = 0;
	i2sReadBuffer = nullptr;
	camFrameHeight = 0;
	lineIndex = 0;
	FuncCB = nullptr;

	SetGpioMatrix();
}

//GPIOマトリクスでカメラのピンをI2Sにマッピングする
void	I2sCamCapture::SetGpioMatrix(void)
{
	using	namespace	CameraConfig;

	//GPIOマトリクスでピンをI2Sにマッピング
	//・VSYNCはDMA転送開始条件を満たすように反転する/しないを考慮すること。
	//	→OV7670のVSYNCは負論理なので反転して接続することとなる。
	gpio_matrix_in(CamPin::D0, I2S0I_DATA_IN0_IDX, false);
	gpio_matrix_in(CamPin::D1, I2S0I_DATA_IN1_IDX, false);
	gpio_matrix_in(CamPin::D2, I2S0I_DATA_IN2_IDX, false);
	gpio_matrix_in(CamPin::D3, I2S0I_DATA_IN3_IDX, false);
	gpio_matrix_in(CamPin::D4, I2S0I_DATA_IN4_IDX, false);
	gpio_matrix_in(CamPin::D5, I2S0I_DATA_IN5_IDX, false);
	gpio_matrix_in(CamPin::D6, I2S0I_DATA_IN6_IDX, false);
	gpio_matrix_in(CamPin::D7, I2S0I_DATA_IN7_IDX, false);
	gpio_matrix_in(CamPin::VSYNC, I2S0I_V_SYNC_IDX, true);	//VSYNCを反転して接続（カメラ側が負論理の場合）
	//gpio_matrix_in(CamPin::VSYNC, I2S0I_V_SYNC_IDX, false);	//VSYNCを反転せずに接続（カメラ側が正論理の場合）
	gpio_matrix_in(CamPin::PCLK, I2S0I_WS_IN_IDX, false);		//PCLKをWSに接続
	gpio_matrix_in(CamPin::HREF, I2S0I_H_SYNC_IDX, false);		//HREFをH_SYNCに接続
	gpio_matrix_in(CamPin::HREF, I2S0I_H_ENABLE_IDX, false);	//HREFをH_ENABLEにも接続

	/*	説明：ピンの割り当て
	■データピン以外の割り当ては、
	・ESP32 Technical Rreference Manual(v5.2)の、
		12.5.2 Camera Slave Receiving Mode - Figure 12-16. Camera Slave Receiving Mode を参照。
	・gpio_matrix_in(I2S0I_V_SYNC_IDX)の反転指定は、カメラのVSYNCの極性を確認し、
		フレーム開始の検出条件(transmission_start)に合うように設定する。
		※transmission_start = (I2S_H_SYNC == 1) && (I2S_V_SYNC == 1) && (I2S_H_ENABLE == 1)

	■i2s_pin_config_t, i2s_set_pini2s_set_pin()は利用しない
	I2Sのカメラモードのピン設定にi2s_pin_config_tを利用しようとすると、.data_in_numの設定で詰む。
		.mck_io_num = I2S_PIN_NO_CHANGE;	// -1
		.bck_io_num = I2S_PIN_NO_CHANGE;	// -1
		.ws_io_num = CAM_PIN_PCLK;
		.data_out_num = I2S_PIN_NO_CHANGE;	// -1
		.data_in_num = ？？？;	←任意の8ピン分のGPIOを指定することができない。
	また、CAM_PIN_VSYNCやCAM_PIN_HREFは、gpio_matrix_in()で設定する必要がある。
	gpio_matrix_in()ではI2Sのカメラモードで必要なピン設定が全てできるので、
	それならば中途半端にi2s_pin_config_tを利用するより、全てgpio_matrix_in()で設定した方が混乱しなくてよい。

	参考：ESP32 TechRefMan - 4.2.2 Functional Description
	#define	GPIO_ALWAYS_LOW		0x30	//When GPIO_FUNCy_IN_SEL is 0x30, input_signal_x is always 0.
	#define	GPIO_ALWAYS_HIGH	0x38	//When GPIO_FUNCy_IN_SEL is 0x38, input_signal_x is always 1.
	I2S_H_ENABLEに0x30/0x38を接続してキャプチャ停止/開始のスイッチにすることもできそうだが未確認。
	→i2s_stop()/i2s_start()を利用した。
	*/
}

//フレーム（カメラの1画面）出力の区切りまで待つ
void	I2sCamCapture::WaitForFrameEnd(void)
{
	//VSYNC=Loは画面出力の途中なので（VSYNCは負論理）、その期間を待機する
	while(!gpio_get_level(CameraConfig::CamPin::VSYNC)) {}
}

//キャプチャ設定
void	I2sCamCapture::CaptureConfigure(int16_t width, int16_t heigth, int16_t bytePerPixel, FnCamPixelData func)
{
	//2回目以降の呼び出しへの対応
	if (hLineProcTask != nullptr) { vTaskDelete(hLineProcTask);	hLineProcTask = nullptr; }
	if (isI2sDriverInstalled) { i2s_driver_uninstall(I2S_NUM_0);	isI2sDriverInstalled = false; }

	pixelDataLength = width * bytePerPixel;
	camFrameHeight = heigth;
	FuncCB = func;

	//i2s_read()読み込み用バッファを確保する
	//・タスクで参照するのでタスク生成前に確保する。
	CreateI2sReadBuffer(pixelDataLength);

	//ドライバーをインストールする（即時動作開始）
	I2sDriverInstall(width, bytePerPixel);

	//キャプチャを停止する
	CaptureStop();

	//画素データ処理タスクを生成する
	//引数	PRO_CPU_NUM(0):	WiFi,BT,Serialと同じコア
	//		APP_CPU_NUM(1):	loop()と同じコア
	//・どちらのコアでも動作するが、キャプチャと描画を同じコアで処理すれば安定性が増す。
	//・i2s_read()を呼び出しているのでI2Sドライバーのインストール後にタスク生成した方が無難。
	xTaskCreatePinnedToCore(PixelDataProcessTask/*関数名*/, "PixDatProcTask"/*タスク名*/,
		2048, nullptr, 2, &hLineProcTask, APP_CPU_NUM);
}

//i2s_read()読み込み用バッファを確保する
//・アプリ実行中に小さい解像度へ変更された場合、再確保せずに使い回す。
void	I2sCamCapture::CreateI2sReadBuffer(size_t pixelDataLength)
{
	static size_t currentBufferLength = 0;

	i2sDmaBufferLength = pixelDataLength * sizeof(uint16_t);	//8bit採取値は16bit型でI2S-DMAバッファに格納されているので
	if (currentBufferLength < i2sDmaBufferLength)
	{
		currentBufferLength = i2sDmaBufferLength;
		if (i2sReadBuffer != nullptr) { heap_caps_free(i2sReadBuffer); }
		i2sReadBuffer = (uint8_t*)heap_caps_malloc(i2sDmaBufferLength, MALLOC_CAP_DMA | MALLOC_CAP_32BIT);
		Serial.printf("I2S i2sReadBuffer address: 0x%X, size: %d byte\n", i2sReadBuffer, i2sDmaBufferLength);
		Serial.printf("I2S DMA_BUFFER_SIZE_TOTAL: %d byte\n", (i2sDmaBufferLength / 2) * (2 + 1));	//値はI2sDriverInstall()のコメントを参照
	}
}

//I2Sドライバをインストールする
void	I2sCamCapture::I2sDriverInstall(int16_t width, int16_t bytePerPixel)
{
	int sampleCount = width * bytePerPixel;	//1ライン分のサンプリング回数（8bit採取なので画素データのサイズ(byte)と同値）
	int dmaFrameLength = sampleCount / 2;	//サンプリング回数の1/n倍(1<=n)を設定する
	int dmaBufferCount = 2 + 1;	//I2S-DMAバッファのデスクリプタの個数（dma_buf_lenを何個用意するか）
	/*	説明: .dma_buf_lenと.dma_buf_countの決め方
	■下記条件を満たすように決める。
		sampleCount < .dma_buf_len * .dma_buf_count
		8 <= dma_buf_len <= 1024, 2 <= dma_buf_count <= 128

	■.dma_buf_countの決め方にクセがある。
	i2s_read()で読みたいサイズがデスクリプタ1個分以下の場合、dma_buf_countは2以上で偶数（経験則）
	i2s_read()で読みたいサイズがデスクリプタ1個分より大きい場合、
		[i2s_read()で読みたい長さの分]のデスクリプタの個数]の整数倍]に1を加えた値]をdma_buf_countとしたとき、正常に動作した。
		※そうなるように.dma_buf_lenを分割したうえで。
	参考（経験則）：
	下記例で式の最後の「+1」は1以上なら何でもよさそうだが、
	VGAで「+2」では正常に動作しなかった。
		→dmaFrameLengthの分（デスクリプタ1個分）ずれたようなデータがi2s_read()で読み込まれた。
	QVGA,QQVGAで「+2」では正常に動作しなかった。
		→i2s_read()で無限待ちしているようだった（推測）。
	i2s_driver_install()やi2s_read()が内部でどのように動作しているかは不明。

	例）VGAの場合
	640*2 < .dma_buf_len * .dma_buf_count = 1280   * (1+1)	dma_buf_len <= 1024 を満たさないので不可
	640*2 < .dma_buf_len * .dma_buf_count = 1280/2 * (2+1)	()内が奇数で動作
	640*2 < .dma_buf_len * .dma_buf_count = 1280/4 * (4+1)	()内が奇数で動作	※メモリを無駄に消費

	例）QVGAの場合
	320*2 < .dma_buf_len * .dma_buf_count = 640   * (1+1)	()内が偶数で動作
	320*2 < .dma_buf_len * .dma_buf_count = 640/2 * (2+1)	()内が奇数で動作
	320*2 < .dma_buf_len * .dma_buf_count = 640/4 * (4+1)	()内が奇数で動作	※メモリを無駄に消費

	例）QQVGAの場合
	160*2 < .dma_buf_len * .dma_buf_count = 320   * (1+1)	()内が偶数で動作
	160*2 < .dma_buf_len * .dma_buf_count = 320/2 * (2+1)	()内が奇数で動作
	160*2 < .dma_buf_len * .dma_buf_count = 320/4 * (4+1)	()内が奇数で動作	※メモリを無駄に消費

	■QVGA,QQVGAはdma_buf_lenをn分割しない場合と分割した場合を比較して、スピード(fps)に差はなかった。
	VGAを含めて式の共通性から、dma_buf_lenを2分割して、デスクリプタ(dma_buf_count)を3個用意する方法を採用する。
	*/

	//I2Sコンフィグ設定
	i2s_config_t i2sConfig =
	{
		.mode = static_cast<i2s_mode_t>(I2S_MODE_SLAVE | I2S_MODE_RX),
		.sample_rate = 300000UL,	//安定動作のためにはエラーが出ない範囲でなるべく大きい値とする
		.bits_per_sample = I2S_BITS_PER_SAMPLE_8BIT,	//1サンプリングあたり8bit採取する
		.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,	//カメラモードではモノラルとする（RIGHTかLEFTかは関係ない）
		.communication_format = I2S_COMM_FORMAT_STAND_MSB,	//カメラモードはI2S標準ではなくMSB標準でないと動作しない
		.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,	//I2SのDMA処理の割り込み優先度（1か2で十分とされる）
		.dma_buf_count = dmaBufferCount,	//I2S buffer count less than 128 and more than 2	※i2s.c内、i2s_driver_install()のソースコードより
		.dma_buf_len = dmaFrameLength,		//I2S buffer length at most 1024 and more than 8	※i2s.c内、i2s_driver_install()のソースコードより
		.use_apll = false	//スレーブモードではAPLL不要（明示的に設定する必要がある）
	};
	/*	説明： i2s_config_t.dma_buf_lenについて
	■i2s_config_t.dma_buf_lenは、
	デスクリプタ1つが持つDMAバッファに格納されるフレーム数を表す。
	※i2s_config_t.dma_buf_countはデスクリプタの個数。
	※以下、「フレーム」「フレームサイズ」はI2SのDMA用語であり、カメラ画像のフレームのことではない。

	■dma_buf_lenの決め方は、
	アプリ側の利用効率を考慮し、「カメラ画像1ライン分の画素を採取するのに必要なサンプリング回数、の倍数」とする。
	カメラモードでは、モノラル（1チャンネル）で、1サンプルあたり8bit(1byte)採取であることは決定している。
	よって、単純に1ラインのデータサイズ(byte)がサンプリング回数となる。
	結論：
	RGB565を前提として、dma_buf_len = (1ラインの画素数 * 2)の倍数

	■dma_buf_lenには上限がある
	dma_buf_lenは次の式を満たす必要がある。
	　real_dma_buf_size = dma_buf_len * chan_num * bits_per_chan / 8[bit/byte] <= 4092byte
	ここで、
	・real_dma_buf_sizeは、I2Sのドライバが内部で確保する「実際のDMAバッファサイズ」。
	・(chan_num * bits_per_chan / 8) = フレームサイズ(byte)
	・カメラモード（モノラルで1サンプルあたり8bit採取）の場合、フレームサイズ = 2byte 【後述】
	改めて、
	　real_dma_buf_size = dma_buf_len * (chan_num * bits_per_chan / 8[bit/byte])
	　real_dma_buf_size = フレーム数 * フレームサイズ
	　real_dma_buf_size = 320の倍数 * 2byte <= 4092byte		※'320の倍数'はQQVGA,RGB565の場合

	■フレームサイズ
	モノラル（1チャンネル）で、1サンプルあたり8bit採取の場合、
	bits_per_chan = 8bit/ch ではなく、bits_per_chan = 16bit/ch となる。
	理由：
	・i2s_config_t.channel_format = I2S_CHANNEL_FMT_ONLY_RIGHTなどモノラル;
	・i2s_config_t.bits_per_sample = I2S_BITS_PER_SAMPLE_8BIT;
	を指定した場合、i2s_driver_install()は、I2S0.fifo_conf.rx_fifo_mod = 1 と設定する。
	FIFOモード1は「16-bit single channel data」を表すので、bits_per_chan = 16bit/ch となる。
	※FIFOモードは0～3がある。ESP32 TechRefMan(v5.2) - 12.4.5 Receiving Data を参照。
		0: 16-bit dual channel data,	1: 16-bit single channel data,
		2: 32-bit dual channel data,	3: 32-bit single channel data

	実際、DMAバッファの中身（サンプルデータ）は、0xAA00,0xBB00,0xCC00,0xDD00,... のようになっている。
	8bitデータが16bitデータとして格納されていることが分かる（エンディアンに注意）。
	※画素データ(RGB565)としては、0xAA-0xBB（これで1画素）,0xCC-0xDD（これで1画素）,... となっている。

	■更なる利用効率
	QQVGA,RGB565では、1つのデスクリプタで最大6ライン分のデータを持てる。
	　real_dma_buf_sizeの最大値4092byte / 640byte/ライン = 6ライン
	利点としては、I2SのDMA割り込み（デスクリプタ1つのバッファが満タンになった、の割り込み）の頻度が1/6になる。
	*/
	/*	説明： i2s_config_t.communication_formatについて
	■i2s_config_tの.communication_formatへの設定で、I2S_COMM_FORMAT_STAND_I2Sか、
	I2S_COMM_FORMAT_STAND_MSBか迷うかもしれないが、I2S_COMM_FORMAT_STAND_MSBを設定すればよい。
	・I2S_COMM_FORMAT_STAND_I2Sを設定すると、I2S Philips standardモードとなる。
		このモードは、BCKクロックの2つ目からデータが配信される（1クロック'シフト'する）。
		レジスタへの影響としては、I2S0.conf.rx_msb_shift = 1 となる。
	・I2S_COMM_FORMAT_STAND_MSBを設定すると、MSB alignment standardモードとなる。
		このモードは、BCKクロックの1つ目からデータが配信される（1クロック'シフト'しない）。
		レジスタへの影響としては、I2S0.conf.rx_msb_shift = 0 となる。

	■カメラモードとしては、I2S_COMM_FORMAT_STAND_MSB を設定する必要がある。
	仮に、I2S_COMM_FORMAT_STAND_I2S を設定した場合、カメラモードではBCKを使用しないので動作しない。
	※BCKを1クロック待ってからWS(PCLK)によりデータ採取を開始するモードなので、ここで無限待ちが発生してしまう。
	*/

	//I2Sドライバのインストール
	esp_err_t retCode = i2s_driver_install(I2S_NUM_0, &i2sConfig, 0, nullptr);
	isI2sDriverInstalled = (retCode == ESP_OK);

	//I2S.conf2レジスタを設定する
	//・I2Sドライバでサポートされない、特定機能向けのレジスタ。
	//・TRM - 12.5.2 Camera Slave Receiving Mode を参照。
	I2S0.conf2.val = 0;			//conf2レジスタをクリア	※以降、'0'をわざわざ設定する必要なし
	I2S0.conf2.camera_en = 1;	//カメラモード有効	※(VSYNC&&HREF&&H_EN)をトリガーにDMA転送するためのビット
	I2S0.conf2.lcd_en = 1;		//名称はLCDだが、I2Sのパラレルモードを有効化するためのビット
	/*	明示的な設定は不要
	I2S0.conf.rx_slave_mod = 1;	//i2s_config_t.mode により適切に設定される
	I2S0.sample_rate_conf.rx_bits_mod = 16;	//i2s_config_t.bits_per_sample により適切に設定される
	I2S0.conf_chan.rx_chan_mod = 1;	//i2s_config_t.channel_format により適切に設定される
	I2S0.fifo_conf.rx_fifo_mod = 1;	//i2s_config_t.bits_per_sample, 同.channel_format, 同.communication_format により適切に設定される
	I2S0.conf.rx_start = 1;	//i2s_driver_install()により'1'が設定される	※i2s_start()と同等
	*/
	/*	説明： カメラモードのレジスタ設定について
	TRM - 12.5.2 には設定すべきレジスタが説明されているが、i2s_driver_install()により同内容が設定されるので、
	明示的にレジスタ操作する必要はない。
	12.5.2 Camera Slave Receiving Mode
	In order to make I2S work in camera mode, 
	the I2S_LCD_EN bit and the I2S_CAMERA_EN bit of register I2S_CONF2_REG are set to 1, 
	the I2S_RX_SLAVE_MOD bit of register I2S_CONF_REG is set to 1, 
	the I2S_RX_MSB_RIGHT bit and the I2S_RX_RIGHT_FIRST bit of I2S_CONF_REG are set to 0. 
	Thus, I2S works in the LCD slave receiving mode. 
	At the same time, in order to use the correct mode to receive data, 
	both the I2S_RX_CHAN_MOD[2:0] bit of register I2S_CONF_CHAN_REG 
	and the I2S_RX_FIFO_MOD[2:0] bit of register I2S_FIFO_CONF_REG are set to 1.

	I2Sをカメラモードで動作させるには、
	レジスタI2S_CONF2_REGのI2S_LCD_ENビットとI2S_CAMERA_ENビットを1に設定し、
	レジスタI2S_CONF_REGのI2S_RX_SLAVE_MODビットを1に設定し、
	I2S_CONF_REGのI2S_RX_MSB_RIGHTビットとI2S_RX_RIGHT_FIRSTビットを0に設定します。
	これにより、I2SはLCDスレーブ受信モードで動作します。
	同時に、正しいモードでデータを受信するには、
	レジスタI2S_CONF_CHAN_REGのI2S_RX_CHAN_MOD[2:0]ビットと
	レジスタI2S_FIFO_CONF_REGのI2S_RX_FIFO_MOD[2:0]ビットの両方を1に設定します。
	*/
}

//キャプチャを停止する
void	I2sCamCapture::CaptureStop(void)
{
	WaitForFrameEnd();
	i2s_stop(I2S_NUM_0);

	//以下の説明は開発中には必要な検討事項だったが、上記コードで問題は解決したので、今となっては参考情報。
	/*	説明： DMAバッファをリセットする
	■i2s_driver_install()直後、
	transmission_start = (I2S_H_SYNC == 1) && (I2S_V_SYNC == 1) && (I2S_H_ENABLE == 1)
	の条件が成立した瞬間から、FIFOを通してDMAバッファに画素データが格納され始める。
	しかしその画素は何行目の何画素目かは分からないので（知る方法はない）、
	1ライン分のDMAバッファにはこの瞬間以降ずっと、n行目のm番目の画素からn+1行目のm-1番目の画素まで、
	という区切りでデータが格納されることとなる。
	※画面には縦にn行、横にm画素スライドした画像が表示される。

	■n行m画素分のずれを取り除くには、
	FIFOとDMAバッファの書き込みポインタ（インデックス）を先頭に戻したうえで、
	0行目の0番目の画素開始に同期して画素データを格納し始めればよい。
	VSYNCを監視することで画面の開始（0行目の0番目の画素開始）を検出することができる。

	■FIFO,DMAをリセットするには、
	下記のコードでFIFO,DMAをリセットすることができる。
	ビットは自動的にクリアされないので手動でクリアする必要がある。
		I2S0.conf.rx_fifo_reset = 1;
		while (I2S0.state.rx_fifo_reset_back) {}	//1: reset is not ready; 0: reset is ready.
		I2S0.conf.rx_fifo_reset = 0;
		I2S0.conf.rx_reset = 1;
		//rx_resetにはリセット完了通知はない
		I2S0.conf.rx_reset = 0;
	【ただし！】
	VSYNCに同期して停止すれば、FIFO,DMAをリセットする処理は必要ない。

	■参考
	開発中に下記のようにしていたことがあったが、この方法は停止タイミングによっては再開時に画像が乱れるのでボツとなった。
	→.dma_buf_count, .dma_buf_lenに起因すると思われる。
		while(gpio_get_level(CameraConfig::CamPin::HREF)) {}	//HREF=Hiはカメラが画素データを出力中
	その後、VSYNCに同期して停止するように変更したところ、画像の乱れはなくなり、
	さらにFIFO,DMAのリセット処理も不要となり、スッキリした。
	*/
}

//キャプチャを開始する
void	I2sCamCapture::CaptureStart(void)
{
	lineIndex = 0;
	WaitForFrameEnd();
	i2s_start(I2S_NUM_0);
}

//画素データ処理タスク
static	void	IRAM_ATTR	PixelDataProcessTask(void* pvParameters)
{
	while (1)
	{
		size_t bytes_read;
		i2s_read(I2S_NUM_0, i2sReadBuffer, i2sDmaBufferLength, &bytes_read, portMAX_DELAY);

		/*	サンプリングデータを画素データに変換する
		I2S-DMAバッファの内容は、AA 00 BB 00 CC 00 DD 00 ...
		16bit型の配列なので値としては、[0x00AA][0x00BB][0x00CC][0x00DD]...	※ESP32はリトルエンディアン
		これを8bit型へキャストすれば目的の配列（画素データの配列）が得られる。
		capBuf(uint16_t):[0x00AA][0x00BB]... --> pixBuf(uint8_t):[0xAA][0xBB]...
		*/
		auto capBuf = reinterpret_cast<uint16_t*>(i2sReadBuffer);
		auto pixBuf = reinterpret_cast<uint8_t*>(i2sReadBuffer);	//もともとuint8_t*型だが分かりやすさのため同書式でキャストした
		for (size_t i = 0; i < pixelDataLength; i++)
		{
			pixBuf[i] = static_cast<uint8_t>(capBuf[i]);
		}

		FuncCB(lineIndex, pixBuf, pixelDataLength);
		if (++lineIndex == camFrameHeight) { lineIndex = 0; }
	}
}
