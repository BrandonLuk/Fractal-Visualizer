/*
* Declares ColorGenerator, a class which handles the conversion of iteration values produced from a fractal to color values
* which can be passed to an OpenGL buffer for display.
*/

#pragma once

#include "thread_pool.h"

#include <stdint.h>

// Some saturated arithmetic functions for 8-bit data types
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

	enum class Generators { HISTOGRAM } mode;

	Color strong;
	Color weak;

	void histogram(int* matrix, int matrix_width, int matrix_height, int n);

public:
	ColorGenerator();
	void generate(int* matrix, int matrix_width, int matrix_height, int n);
};