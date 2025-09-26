//色データクラス
//『昼夜逆転』工作室	@jsdiy	https://github.com/jsdiy
//	2024/04 - 2025/08

#pragma	once

#include <Arduino.h>

//色データ
class	Color
{
public:
	static	constexpr	uint8_t	Length = 2;
	static	Color	CreateRGB565(uint8_t red, uint8_t green, uint8_t blue);

	Color(void) {}
	uint8_t	bytes[Length];
};
