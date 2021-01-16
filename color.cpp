#include "color.h"
#include "thread_pool.h"

#include <algorithm>
#include <atomic>
#include <memory>
#include <stdint.h>
#include <vector>

#include <immintrin.h> // AVX intrinsics

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
	mode = (Generators)((static_cast<int>(mode) + 1) % static_cast<int>(Generators::LAST));
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

////////////////////////////////////////////////////////////
/// Simple color generator
////////////////////////////////////////////////////////////
/*
* To produce some nice, simple colors, simply plug the number of iterations into the sine function. Arbitrary modifiers can be applied
* to change the color palette.
*/

void ColorGenerator::simpleThread(int index, int stride, int* matrix, int matrix_width, int matrix_height)
{
	double a;
	unsigned char r, g, b;
	for (int i = index; i < (matrix_width * matrix_height); i += stride)
	{
		a = static_cast<double>(matrix[i]);
		// 255 -> max value of an unsigned char
		// The sine function can return negative values, so clamp its result to [0.0, 1.0] by
		// multiplying by 0.5 then adding 0.5.
		// The float literals inside the sine function are arbitrary and can be changed to alter the color palette.
		r = static_cast<unsigned char>(255 * (0.5f * std::sin(a * 0.1f + 1.246f) + 0.5f));
		g = static_cast<unsigned char>(255 * (0.5f * std::sin(a * 0.1f + 0.396f) + 0.5f));
		b = static_cast<unsigned char>(255 * (0.5f * std::sin(a * 0.1f + 3.188f) + 0.5f));

		// Pack the unsigned chars into an int for OpenGL
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

/*
* Perform simple iteratin value to color conversion using AVX instructions.
*/
void ColorGenerator::simpleAVXThread(int index, int stride, int* matrix, int end_index)
{
	__m256 _iter, _r, _g, _b, _uchar_max, _half, _tenth, _r_mod, _g_mod, _b_mod;
	__m256i _res;

	_uchar_max	= _mm256_set1_ps(255.0f);
	_half		= _mm256_set1_ps(0.5f);
	_tenth		= _mm256_set1_ps(0.1f);
	_r_mod		= _mm256_set1_ps(1.246f);
	_g_mod		= _mm256_set1_ps(0.396f);
	_b_mod		= _mm256_set1_ps(3.188f);

	for (int i = index; i < end_index; i += stride)
	{
		// Load the iteration values, convert them to floats, and store them in _iter
		_iter = _mm256_cvtepi32_ps(_mm256_load_si256((__m256i*)&matrix[i]));

		// r
		_r = _mm256_sin_ps(_mm256_fmadd_ps(_iter, _tenth, _r_mod));
		_r = _mm256_fmadd_ps(_half, _r, _half);
		_r = _mm256_mul_ps(_uchar_max, _r);

		// g
		_g = _mm256_sin_ps(_mm256_fmadd_ps(_iter, _tenth, _g_mod));
		_g = _mm256_fmadd_ps(_half, _g, _half);
		_g = _mm256_mul_ps(_uchar_max, _g);

		// b
		_b = _mm256_sin_ps(_mm256_fmadd_ps(_iter, _tenth, _b_mod));
		_b = _mm256_fmadd_ps(_half, _b, _half);
		_b = _mm256_mul_ps(_uchar_max, _b);

		/*
		* We wish to shift and pack the R, G, and B values into an int, as is done in the standard isntruction version of this function:
		* matrix[i] = int(b << 16 | g << 8 | r);
		*/
		_res = _mm256_slli_epi32(_mm256_cvtps_epi32(_b), 16);						// Convert _b from float to 32-bit int and shift left 16
		_res = _mm256_or_si256(_res, _mm256_slli_epi32(_mm256_cvtps_epi32(_g), 8)); // Convert _g from float to 32-bit int, shift left 8, and or with _res
		_res = _mm256_or_si256(_res, _mm256_cvtps_epi32(_r));						// Convert _r from float to 32-bit int and or with _res

		// Write _res directly into the matrix, overwriting the iteration values we used
		_mm256_store_si256((__m256i*) & matrix[i], _res);
	}
}

void ColorGenerator::simpleAVX(int* matrix, int end_index)
{
	for (int index = 0; index < t_pool->size; ++index)
	{
		t_pool->addJob([=]() {simpleAVXThread(index * 8, (t_pool->size) * 8, matrix, end_index); });
	}
	t_pool->synchronize();
}

////////////////////////////////////////////////////////////
/// Histogram color generator
////////////////////////////////////////////////////////////
/*
* Produces a more regular color pattern, but is slower.
*/
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

void ColorGenerator::generateAVX(int* matrix, int matrix_width, int matrix_height, int n)
{
	if (mode == Generators::HISTOGRAM)
	{
		histogram(matrix, matrix_width, matrix_height, n);
	}
	else
	{
		simpleAVX(matrix, matrix_width * matrix_height);
	}
}