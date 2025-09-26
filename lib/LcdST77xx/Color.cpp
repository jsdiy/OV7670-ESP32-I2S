//色データクラス
//『昼夜逆転』工作室	@jsdiy	https://github.com/jsdiy
//	2024/04 - 2025/08

#include <Arduino.h>
#include "Color.hpp"

//色を作る
Color	Color::CreateRGB565(uint8_t red, uint8_t green, uint8_t blue)
{
	red		= red   * 0x1F / 0xFF;
	green	= green * 0x3F / 0xFF;
	blue	= blue  * 0x1F / 0xFF;
	
	//[R5:G3][G3:B5]
	Color	color;
	color.bytes[0] = (red   << 3) | (green >> 3);
	color.bytes[1] = (green << 5) | blue;
	return color;
}
