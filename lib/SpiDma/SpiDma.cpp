//SPI-DMA制御
//『昼夜逆転』工作室	@jsdiy	https://github.com/jsdiy
//	2024/06 - 2025/09

#include	<Arduino.h>
#include	"SpiDma.hpp"

//インスタンス
SpiDma	spiDma;

//SPIバスを初期化する
void	SpiDma::Initialize(spi_host_device_t hostId, gpio_num_t mosi, gpio_num_t miso, gpio_num_t sck)
{
	if (isInitialized) { return; }

	//SPIホストの設定
	spiHostId = hostId;

	//SPIバスの設定
	spi_bus_config_t spiBusConfig;
	InitializeSpiBusConfig(&spiBusConfig, mosi, miso, sck);
	spi_bus_initialize(spiHostId, &spiBusConfig, SPI_DMA_CH_AUTO);

	isInitialized = true;
}

//SPIバスの設定（ユーザー設定値）
void	SpiDma::InitializeSpiBusConfig(spi_bus_config_t* spiBusConfig, gpio_num_t mosi, gpio_num_t miso, gpio_num_t sck)
{
	spiBusConfig->mosi_io_num = mosi;
	spiBusConfig->miso_io_num = miso;
	spiBusConfig->sclk_io_num = sck;
	spiBusConfig->quadwp_io_num = -1;
	spiBusConfig->quadhd_io_num = -1;
	spiBusConfig->data4_io_num = -1;
	spiBusConfig->data5_io_num = -1;
	spiBusConfig->data6_io_num = -1;
	spiBusConfig->data7_io_num = -1;
	spiBusConfig->max_transfer_sz = 0;	//'=0'で4092byte（最大値）
	spiBusConfig->flags = SPICOMMON_BUSFLAG_MASTER;
	spiBusConfig->intr_flags = ESP_INTR_FLAG_LEVEL1;
}

//SPIデバイスをバスに加える（3個まで可）
//引数	devConfig:	デバイスの設定
//		devHandle:	戻り値としてデバイスハンドル
void	SpiDma::AddDeviceToBus(const spi_device_interface_config_t* devConfig, spi_device_handle_t* devHandle)
{
	spi_bus_add_device(spiHostId, devConfig, devHandle);
}

//SPIデバイスをバスから外す
//・バスに接続したいデバイスが3個を越える場合はどれかを外す必要がある。
void	SpiDma::RemoveDeviceFromBus(spi_device_handle_t devHandle)
{
	spi_bus_remove_device(devHandle);
}

//データを送信する（転送完了までブロックする関数）
//・DMAではなくCPUによる転送。
void	SpiDma::Transmit(spi_device_handle_t devHandle, uint8_t data)
{
	trans.flags = SPI_TRANS_USE_TXDATA;	// *tx_bufferではなくtx_data[]を使うことのフラグ
	trans.length = GetBitLength(1);
	trans.tx_data[0] = data;

	spi_device_transmit(devHandle, &trans);
}

//データを送信する（転送完了までブロックする関数）
//・DMAではなくCPUによる転送。
void	SpiDma::Transmit(spi_device_handle_t devHandle, uint8_t data1, uint8_t data2)
{
	trans.flags = SPI_TRANS_USE_TXDATA;	// *tx_bufferではなくtx_data[]を使うことのフラグ
	trans.length = GetBitLength(2);
	trans.tx_data[0] = data1;
	trans.tx_data[1] = data2;

	spi_device_transmit(devHandle, &trans);
}

//データを送信する（転送完了までブロックする関数）
//・DMAではなくCPUによる転送。
void	SpiDma::Transmit(spi_device_handle_t devHandle, uint8_t data1, uint8_t data2, uint8_t data3)
{
	trans.flags = SPI_TRANS_USE_TXDATA;	// *tx_bufferではなくtx_data[]を使うことのフラグ
	trans.length = GetBitLength(3);
	trans.tx_data[0] = data1;
	trans.tx_data[1] = data2;
	trans.tx_data[2] = data3;

	spi_device_transmit(devHandle, &trans);
}

//データを送信する（転送完了までブロックする関数）
//・DMAではなくCPUによる転送。
void	SpiDma::Transmit(spi_device_handle_t devHandle, uint8_t data1, uint8_t data2, uint8_t data3, uint8_t data4)
{
	trans.flags = SPI_TRANS_USE_TXDATA;	// *tx_bufferではなくtx_data[]を使うことのフラグ
	trans.length = GetBitLength(4);
	trans.tx_data[0] = data1;
	trans.tx_data[1] = data2;
	trans.tx_data[2] = data3;
	trans.tx_data[3] = data4;

	spi_device_transmit(devHandle, &trans);
}

//データを送信する（転送完了までブロックする関数）
//引数	datas:	DMA対応メモリであること
//		length <= 4092byte
void	SpiDma::Transmit(spi_device_handle_t devHandle, const uint8_t* datas, size_t length)
{
	trans.flags = 0;	//tx_data[]ではなく *tx_bufferを使う場合はフラグをリセットする
	trans.length = GetBitLength(length);
	trans.tx_buffer = datas;

	spi_device_transmit(devHandle, &trans);
}

//データを送信する（転送完了までブロックする関数）
//引数	datas:	DMA対応メモリであること
//		length > 4092byte
void	SpiDma::TransmitOverSize(spi_device_handle_t devHandle, const uint8_t* datas, size_t length)
{
	trans.flags = 0;	//tx_data[]ではなく *tx_bufferを使う場合はフラグをリセットする
	size_t idx = 0;
	while (0 < length)
	{
		size_t txLength = (MaxTxBufferSize < length) ? MaxTxBufferSize : length;
		trans.length = GetBitLength(txLength);
		trans.tx_buffer = &datas[idx];
		spi_device_transmit(devHandle, &trans);
		length -= txLength;
		idx += txLength;
	}
}

/*	非同期関係
//非同期でデータを送信する（転送完了までブロックしない関数）
//・ローカルのトランザクションデスクリプタを使用する。
bool	SpiDma::TransmitAsync(uint8_t* datas, size_t length)
{
	if (isTransmitAsync) { return false; }	//前回のTransmitAsync()がまだ完了していない（transが使用中）
	isTransmitAsync = true;
	TransmitAsync(datas, length, trans);
	return true;
}

//非同期でデータを送信する（転送完了までブロックしない関数）
//・ユーザーが用意したトランザクションデスクリプタを使用する。
void	SpiDma::TransmitAsync(uint8_t* datas, size_t length, spi_transaction_t* transaction)
{
	transaction->flags = 0;	//tx_data[]ではなく *tx_bufferを使う場合はフラグをリセットする
	transaction->length = GetBitLength(length);
	transaction->tx_buffer = datas;

	spi_device_queue_trans(spiDevice, transaction, portMAX_DELAY);
}

//TransmitAsync()の完了を待つ（ブロックする関数）
//戻り値:	実行完了したトランザクションデスクリプタ。またはnullptr
//・キューに入っているトランザクションの転送が完了するまで待つ。キューが空なら無限に待ち続ける。
//・キューの深さはspi_device_interface_config_t.queue_sizeで決めておく。
spi_transaction_t*	SpiDma::WaitForTransmitComplete(void)
{
	spi_transaction_t* finishedTrans = nullptr;
	spi_device_get_trans_result(spiDevice, &finishedTrans, portMAX_DELAY);

	//finishedTransがローカルのトランザクションデスクリプタだったら、戻り値はnullptrとする
	//ユーザーが用意したトランザクションデスクリプタだったら、それを戻り値とする
	return (finishedTrans == trans) ? nullptr : finishedTrans;
}
*/
