#include "color.h"
#include "thread_pool.h"

#include <algorithm>
#include <atomic>
#include <memory>
#include <stdint.h>
#include <vector>

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
	mode = Generators::SIMPLE;
	weak = Color{ 50, 100, 25 };
	strong = Color{ 255, 255, 255 };

	t_pool = &ThreadPool::getInstance();
}

void ColorGenerator::switchMode()
{
	mode = static_cast<Generators>((mode + 1) % (Generators::LAST));
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

void ColorGenerator::simpleThread(int index, int stride, int* matrix, int matrix_width, int matrix_height)
{
	double a;
	unsigned char r, g, b;
	for (int i = index; i < (matrix_width * matrix_height); i += stride)
	{
		a = (double)matrix[i];
		r = 255 * (0.5f * std::sin(a * 0.1f + 1.246f) + 0.5f);
		g = 255 * (0.5f * std::sin(a * 0.1f + 0.396f) + 0.5f);
		b = 255 * (0.5f * std::sin(a * 0.1f + 3.188f) + 0.5f);
		matrix[i] = int(b << 16 | g << 8 | r);
	}
}

void ColorGenerator::simple(int* matrix, int matrix_width, int matrix_height)
{
	for (int index = 0; index < t_pool->size; ++index)
	{
		t_pool->addJob([=]() {simpleThread(index, t_pool->size, matrix, matrix_width, matrix_height); });
	}
	t_pool->synchronize();
}

void histogramCountThread(int index, int stride, std::vector<std::unique_ptr<std::atomic<int>>>& num_iters_per_pixel, int* matrix, int matrix_width, int matrix_height)
{
	for (int i = index; i < matrix_width * matrix_height; i += stride)
	{
		(*num_iters_per_pixel[matrix[i]]) += 1;
	}
}

void histogramSumThread(int index, int stride, int* result, std::vector<std::unique_ptr<std::atomic<int>>>& num_iters_per_pixel, int n)
{
	for (int i = index; i < n; i += stride)
	{
		(*result) += num_iters_per_pixel[i]->load();
	}
}

void ColorGenerator::histogramHueThead(int index, int stride, unsigned int total, std::vector<std::unique_ptr<std::atomic<int>>>& num_iters_per_pixel, int* matrix, int matrix_width, int matrix_height)
{
	int iter;
	double hue;
	for (int i = index; i < matrix_width * matrix_height; i += stride)
	{
		iter = matrix[i];
		hue = 0.0;
		for (int k = 0; k < iter; ++k)
		{
			hue += double(num_iters_per_pixel[k]->load()) / total;
		}
		matrix[i] = int(weak + ((strong - weak) * hue));
	}
}

void ColorGenerator::histogram(int* matrix, int matrix_width, int matrix_height, int n)
{
	std::vector<std::unique_ptr<std::atomic<int>>> num_iters_per_pixel(n);
	for (auto& p : num_iters_per_pixel)
		p = std::make_unique<std::atomic<int>>(0);

	for (int index = 0; index < t_pool->size; ++index)
	{
		t_pool->addJob([=, &num_iters_per_pixel]() {histogramCountThread(index, t_pool->size, num_iters_per_pixel, matrix, matrix_width, matrix_height); });
	}

	std::vector<int> thread_sums(t_pool->size, 0);
	t_pool->synchronize();

	for (int index = 0; index < t_pool->size; ++index)
	{
		t_pool->addJob([=, &num_iters_per_pixel, &thread_sums]() {histogramSumThread(index, t_pool->size, &thread_sums[index], num_iters_per_pixel, n); });
	}
	t_pool->synchronize();

	unsigned int total = 0;
	for (int sum : thread_sums)
		total += sum;

	for (int index = 0; index < t_pool->size; ++index)
	{
		t_pool->addJob([=, &num_iters_per_pixel]() {histogramHueThead(index, t_pool->size, total, num_iters_per_pixel, matrix, matrix_width, matrix_height); });
	}
	t_pool->synchronize();

}

void ColorGenerator::generate(int* matrix, int matrix_width, int matrix_height, int n)
{
	if (mode == Generators::HISTOGRAM)
	{
		histogram(matrix, matrix_width, matrix_height, n);
	}
	else
	{
		simple(matrix, matrix_width, matrix_height);
	}
}