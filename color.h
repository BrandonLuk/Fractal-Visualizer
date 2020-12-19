#pragma once

#include "thread_pool.h"

#include <stdint.h>

uint8_t sat_add_u8b(uint8_t l, uint8_t r);
uint8_t sat_sub_u8b(uint8_t l, uint8_t r);

class ColorGenerator
{
	struct Color {
		unsigned char r, g, b;
		explicit operator int() const;
		Color operator+(const Color& other);
		Color operator-(const Color& other);
		Color operator*(double multiplier);
	};

	Color strong;
	Color weak;

public:
	ColorGenerator();
	int colorClamp(int val, int val_max);
};