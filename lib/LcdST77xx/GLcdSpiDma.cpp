//グラフィックLCD用SPI-DMA基本操作
//『昼夜逆転』工作室	@jsdiy	https://github.com/jsdiy
//	2025/09

#include	<Arduino.h>
#include	"GLcdSpiDma.hpp"

//初期化
void	GLcdSpiDma::Initialize(uint8_t spiMode, int32_t busSpeedHz, gpio_num_t pinSS, gpio_num_t pinDC)
{
	//ピンモードを設定する
	SetPinMode(pinSS, pinDC);
	gpio_set_level(pinSS, 1);	gpio_set_level(pinDC, 1);
	this->pinDC = pinDC;

	//SPIバスを初期化する
	spiDma.Initialize();

	//バスにデバイスを追加する
	spi_device_interface_config_t spiDevIfConfig;
	SetDeviceConfig(spiMode, busSpeedHz, pinSS, &spiDevIfConfig);
	spiDma.AddDeviceToBus(&spiDevIfConfig, &devHandle);
}

//ピンモードを設定する
void	GLcdSpiDma::SetPinMode(gpio_num_t pinSS, gpio_num_t pinDC)
{
	gpio_config_t pinConfig = { 0 };
	pinConfig.pin_bit_mask = (1ULL << pinSS) | (1ULL << pinDC);
	pinConfig.mode = GPIO_MODE_OUTPUT;
	gpio_config(&pinConfig);
}

//デバイス構造体を設定する
void	GLcdSpiDma::SetDeviceConfig(uint8_t spiMode, int32_t busSpeedHz, gpio_num_t pinSS, spi_device_interface_config_t* devConfig)
{
	memset(devConfig, 0, sizeof(spi_device_interface_config_t));

	devConfig->mode = spiMode;		//SPI mode 0-3
	devConfig->clock_speed_hz = busSpeedHz;	//80MHzを分周した値（SPI_MASTER_FREQ_xxMが使える）
	devConfig->spics_io_num = pinSS;	//SPIのSSピン
	devConfig->queue_size = 1;		//キューイング可能なトランザクション個数（1以上）
	devConfig->pre_cb = nullptr;	//DMA転送開始直前の処理
	devConfig->post_cb = nullptr;	//DMA転送完了直後の処理
}

//データを送信する
//引数	datas:	DMA対応メモリであること
//		length <= 4092byte が基本	※超える場合は自動的にTransmitOverSize()が実行される
void	GLcdSpiDma::SendData(const uint8_t* datas, size_t length)
{
	ChangeDataMode();
	if (SpiDma::MaxTxBufferSize < length)
		spiDma.TransmitOverSize(devHandle, datas, length);
	else
		spiDma.Transmit(devHandle, datas, length);
}

//LCDの描画領域を設定する
void	GLcdSpiDma::SetGRamRange(uint8_t cmd, int16_t startPos, int16_t endPos)
{
	SendCommand(cmd);
	ChangeDataMode();
	spiDma.Transmit(devHandle, highByte(startPos), lowByte(startPos), highByte(endPos), lowByte(endPos));
}

#if (0)	//非同期関係

/*
//トランザクション関連
#define	WRITE_SEQUENCE_SIZE	6	//描画シーケンスに必要なトランザクションの個数（固定値）
#define	MAX_SEQUENCE_SIZE	1	//キューに入れることができる描画シーケンスの最大数（アプリにより変更）
static	spi_transaction_t	trans[WRITE_SEQUENCE_SIZE * MAX_SEQUENCE_SIZE];	//描画シーケンス1つあたりのトランザクションのセット
static	spi_transaction_t*	sequenceArray[MAX_SEQUENCE_SIZE];	//trans[]を格納する配列

//コールバック関連
static	void	IRAM_ATTR	CbSpiPreTtransfer(spi_transaction_t *t);
*/

//初期化
void	GLcdSpiDma::Initialize(void)
{
	//トランザクション配列のメモリを確保する
	bool isOK = transactionPool.Initialize(transactionPerLine, maxTransfarLine);
	if (!isOK) { return; }
	for (int16_t i = 0; i < maxTransfarLine; i++)
	{
		spi_transaction_t *transArray = transactionPool.NextItem();
		InitializeTransaction(transArray);
	}

	//SPIホスト
	spiHostId = VSPI_HOST;	//VSPI_HOST=SPI3_HOST=VSPI

	//SPIバスの設定（VSPIを想定）
	InitializeSpiBusConfig();

	//Initialize the SPI bus
	spi_bus_initialize(spiHostId, &spiBusConfig, SPI_DMA_CH_AUTO);

	//SPIデバイスの設定（クロックスピード、SlaveSelectピン、転送前後の処理）
	InitializeSpiDeviceInterfaceConfig();

	//Attach the device to the SPI bus
	//・第3引数は戻り値として使用される（値が格納される）。
	spi_bus_add_device(spiHostId, &spiDevIfConfig, &spiDevice);

	pinDcHi = { pinDC, 1 };
	pinDcLo = { pinDC, 0 };
}

//ラインパケット（1ライン分のトランザクション）を初期化する
/*
trans[0]	コマンド:X方向領域指定(1byte)
trans[1]	データ:StartX_Hi/Lo,EndX_Hi/Lo(4byte)
trans[2]	コマンド:Y方向領域指定(1byte)
trans[3]	データ:StartY_Hi/Lo,EndY_Hi/Lo(4byte)
trans[4]	コマンド:データ送信開始(1byte)
trans[5]	データ:画素データ(LCD_Width * 2byte_RGB565)
trans[even/odd].user = 0:LCD_DC=Lo(コマンド送信), 1:LCD_DC=Hi(データ送信)
*/
void	GLcdSpiDma::InitializeTransaction(spi_transaction_t* trans)
{
	for (uint8_t i = 0; i < transactionPerLine; i++)
	{
		memset(&trans[i], 0, sizeof(spi_transaction_t));

		if ((i & 0x01) == 0)
		{
			//Even transfers are commands
			trans[i].length = BitLengthOfBytes(1);	//コマンドは1byteなので
			trans[i].user = &pinDcLo;
		}
		else
		{
			//Odd transfers are data
			trans[i].length = BitLengthOfBytes(4);	//データを4byte送るので
			trans[i].user = &pinDcHi;
		}

		trans[i].flags = SPI_TRANS_USE_TXDATA;
		//*tx_bufferではなくtx_data[4]を使うことのフラグ（4は4byteの意味）
		//ただしi=5のトランザクションは*tx_bufferを使うので、下記でフラグを解除している
	}

	trans[0].tx_data[0] = cmdCASET;	//Column address set
	trans[2].tx_data[0] = cmdRASET;	//Row address set
	trans[4].tx_data[0] = cmdRAMWR;	//Memory write
	trans[5].flags = 0;	//undo SPI_TRANS_USE_TXDATA flag
}

//コールバック：DMA転送直前の処理
//This function is called (in irq context!) just before a transmission starts.
static	void	IRAM_ATTR	CbSpiPreTtransfer(spi_transaction_t *t)
{
	auto pinCtrl = reinterpret_cast<TLcdDcCtrl*>(t->user);
	gpio_set_level(pinCtrl->pinDC, pinCtrl->val);	//digitalWrite()より高速
	//REG_WRITE(pinCtrl->val ? GPIO_OUT_W1TS_REG : GPIO_OUT_W1TC_REG, 1 << pinCtrl->pinDC);	//gpio_set_level()より高速らしいが差は見られなかった
}

//1ライン分の画素データをDMA転送する
//引数	pixelData:	画素データ。heap_caps_malloc(MALLOC_CAP_DMA)で確保したメモリであること
void	GLcdSpiDma::Transmit(int16_t x, int16_t y, int16_t w, int16_t h, const uint8_t *pixelData, size_t dataLength)
{
	//ラインパケットを構築する
	spi_transaction_t *linePacket = transactionPool.NextItem();
	BuildLinePacket(linePacket, x, y, w, h, pixelData, dataLength);

	//ラインパケットをDMA転送キューに入れる
	EnqueueLinePacket(linePacket);
}

//トランザクションに値をセットする
/*
trans[0],trans[2],trans[4]はコマンド送信のトランザクション
trans[1],trans[3],trans[5]はデータ送信のトランザクション　←こちらの値をセットする
*/
void	GLcdSpiDma::BuildLinePacket(spi_transaction_t *trans, int16_t x, int16_t y, int16_t w, int16_t h, const uint8_t *imgData, size_t dataLength)
{
	int16_t endX = x + w - 1, endY = y + h - 1;
	trans[1].tx_data[0] = HiByte(x);
	trans[1].tx_data[1] = LoByte(x);
	trans[1].tx_data[2] = HiByte(endX);
	trans[1].tx_data[3] = LoByte(endX);

	trans[3].tx_data[0] = HiByte(y);
	trans[3].tx_data[1] = LoByte(y);
	trans[3].tx_data[2] = HiByte(endY);
	trans[3].tx_data[3] = LoByte(endY);

	trans[5].tx_buffer = imgData;
	trans[5].length = BitLengthOfBytes(dataLength);	//Data length, in bits
}

//1ライン分の描画トランザクションをキューに入れる
void	GLcdSpiDma::EnqueueLinePacket(const spi_transaction_t* linePacket)
{
	auto transArray = const_cast<spi_transaction_t*>(linePacket);
	for (uint8_t i = 0; i < transactionPerLine; i++)
	{
		spi_device_queue_trans(spiDevice, &transArray[i], portMAX_DELAY);
	}
}

#endif
