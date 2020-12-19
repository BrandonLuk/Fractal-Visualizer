#include "color.h"
#include "thread_pool.h"

#include <stdint.h>

uint8_t sat_add_u8b(uint8_t l, uint8_t r)
{
	uint8_t res = l + r;
	res |= -(res < l);
	return res;
}

uint8_t sat_sub_u8b(uint8_t l, uint8_t r)
{
	uint8_t res = l - r;
	res &= -(res <= l);
	return res;
}

ColorGenerator::ColorGenerator()
{
	weak = Color{ 0, 100, 0 };
	strong = Color{ 0, 0, 255 };
}

ColorGenerator::Color::operator int() const 
{
	return int(b << 16 | g << 8 | r);
}

ColorGenerator::Color ColorGenerator::Color::operator+(const Color& other)
{
	return Color{	sat_add_u8b(r, other.r),
					sat_add_u8b(g, other.g),
					sat_add_u8b(b, other.b) };
}

ColorGenerator::Color ColorGenerator::Color::operator-(const Color& other)
{

	return Color{	sat_sub_u8b(r, other.r),
					sat_sub_u8b(g, other.g),
					sat_sub_u8b(b, other.b) };
}

ColorGenerator::Color ColorGenerator::Color::operator*(double multiplier)
{
	return Color{	unsigned char(r * multiplier),
					unsigned char(g * multiplier),
					unsigned char(b * multiplier) };
}

int ColorGenerator::colorClamp(int val, int val_max)
{
	double ratio = double(val) / val_max;
	return int(weak + ((strong - weak) * ratio));
}