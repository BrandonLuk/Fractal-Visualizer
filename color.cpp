#include "color.h"
#include "thread_pool.h"

#include <algorithm>
#include <stdint.h>

#include <iostream>

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
	mode = Generators::HISTOGRAM;
	weak = Color{ 50, 100, 25 };
	strong = Color{ 255, 255, 255 };
}

// To pack chars into a 32-bit int we use bit shifting.
// OpenGL expects the last 8-bits to be the alpha value of the color, but since we dont use the
// alpha channel we just ignore it.
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

void ColorGenerator::histogram(int* matrix, int matrix_width, int matrix_height, int n)
{
	int* num_iter_per_pixel = new int[n];
	std::fill(num_iter_per_pixel, num_iter_per_pixel + n, 0);

	for (int j = 0; j < matrix_height; ++j)
	{
		for (int i = 0; i < matrix_width; ++i)
		{
			num_iter_per_pixel[matrix[j * matrix_width + i]]++;
		}
	}

	unsigned int total = 0;
	for (int i = 0; i < n; ++i)
	{
		total += num_iter_per_pixel[i];
	}

	for (int j = 0; j < matrix_height; ++j)
	{
		for (int i = 0; i < matrix_width; ++i)
		{
			int iter = matrix[j * matrix_width + i];
			double hue = 0.0;
			for (int k = 0; k < iter; ++k)
			{
				hue += float(num_iter_per_pixel[k]) / total;
			}
			matrix[j * matrix_width + i] = int(weak + ((strong - weak) * hue));
		}
	}
}

void ColorGenerator::generate(int* matrix, int matrix_width, int matrix_height, int n)
{
	if (mode == Generators::HISTOGRAM)
	{
		histogram(matrix, matrix_width, matrix_height, n);
	}
}