#include "fractal.h"

#include "thread_pool.h"

#include <cmath>
#include <complex>
#include <functional>
#include <thread>
#include <vector>

#include <immintrin.h>

#include <iostream>

// For timing
#ifdef PRINT_INFO
std::chrono::steady_clock::time_point start;
std::chrono::nanoseconds dur;

#define START_TIMER start = std::chrono::steady_clock::now();
#define END_TIMER   dur = std::chrono::steady_clock::now() - start; \
                    std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(dur).count() << "ms" << std::endl;
#endif

Fractal::Fractal()
{
	fractal_mode = FractalSets::MANDELBROT;
	instruction_mode = InstructionModes::AVX;

	mandelbrot_x_min				= mandelbrot_x_min_DEFAULT;
	mandelbrot_x_max				= mandelbrot_x_max_DEFAULT;
	mandelbrot_y_min				= mandelbrot_y_min_DEFAULT;
	mandelbrot_y_max				= mandelbrot_y_max_DEFAULT;
	mandelbrot_radius				= mandelbrot_radius_DEFAULT;
	mandelbrot_zoom					= mandelbrot_zoom_DEFAULT;
	mandelbrot_zoom_multiplier		= mandelbrot_zoom_multiplier_DEFAULT;
	mandelbrot_x_offset				= mandelbrot_x_offset_DEFAULT;
	mandelbrot_y_offset				= mandelbrot_y_offset_DEFAULT;
	mandelbrot_pan_increment		= mandelbrot_pan_increment_DEFAULT;
	mandelbrot_max_iter				= mandelbrot_max_iter_DEFAULT;
	mandelbrot_max_iter_multiplier	= mandelbrot_max_iter_multiplier_DEFAULT;
	

	julia_x_offset					= julia_x_offset_DEFAULT;
	julia_y_offset					= julia_y_offset_DEFAULT;
	julia_pan_increment				= julia_pan_increment_DEFAULT;
	julia_zoom						= julia_zoom_DEFAULT;
	julia_zoom_multiplier			= julia_zoom_multiplier_DEFAULT;
	julia_n							= julia_n_DEFAULT;
	julia_max_iter					= julia_max_iter_DEFAULT;
	julia_max_iter_multiplier		= julia_max_iter_multiplier_DEFAULT;
	julia_escape_radius				= julia_escape_radius_DEFAULT;
	julia_complex_param				= julia_complex_param_DEFAULT;

	t_pool = &ThreadPool::getInstance();
}


/*
* Zoom in or out while mantaining current view of the fractal.
* 
* @param int direction : The direction of the zoom. direction > 0 is zoom in, else zoom out
*/
void Fractal::stationaryZoom(int direction, int max_x, int max_y)
{
	if (fractal_mode == FractalSets::MANDELBROT)
	{
		/*
		* Soley altering the mandelbrot_zoom parameter would cause the current view of the fractal to shift.
		* To avoid this, we track the change of the center pixel before and after the zoom and change our offsets to match.
		*/

		// 0.5 since we are interested in the center pixel value.
		long double common_x = mandelbrot_x_min + ((mandelbrot_x_max - mandelbrot_x_min) * 0.5) + mandelbrot_x_offset;
		long double common_y = mandelbrot_y_min + ((mandelbrot_y_max - mandelbrot_y_min) * 0.5) + mandelbrot_y_offset;
		long double old_zoom = mandelbrot_zoom;

		if (direction > 0)
		{
			mandelbrot_zoom *= (1.0 + mandelbrot_zoom_multiplier);
		}
		else
		{
			mandelbrot_zoom *= (1.0 - mandelbrot_zoom_multiplier);
		}

		mandelbrot_x_offset += common_x * (mandelbrot_zoom / old_zoom - 1.0);
		mandelbrot_y_offset += common_y * (mandelbrot_zoom / old_zoom - 1.0);
	}
	else if (fractal_mode == FractalSets::JULIA)
	{
		long double common_x = -julia_escape_radius + (2.0 * julia_escape_radius * 0.5) + julia_x_offset;
		long double common_y = -julia_escape_radius + (2.0 * julia_escape_radius * 0.5) + julia_y_offset;
		long double old_zoom = julia_zoom;

		if (direction > 0)
		{
			julia_zoom *= (1.0 + julia_zoom_multiplier);
		}
		else
		{
			julia_zoom *= (1.0 - julia_zoom_multiplier);
		}

		julia_x_offset += common_x * (julia_zoom / old_zoom - 1.0);
		julia_y_offset += common_y * (julia_zoom / old_zoom - 1.0);
	}
}

/*
* Similar to the stationary zoom, we zoom but also follow the mouse cursor.
*/
void Fractal::followingZoom(int direction, int x_pos, int y_pos, int max_x, int max_y)
{
	if (fractal_mode == FractalSets::MANDELBROT)
	{
		long double prezoom_cursor_x;
		long double prezoom_cursor_y;
		mandelbrotScale(prezoom_cursor_x, prezoom_cursor_y, x_pos, y_pos, max_x, max_y);

		stationaryZoom(direction, max_x, max_y);

		long double postzoom_cursor_x;
		long double postzoom_cursor_y;
		mandelbrotScale(postzoom_cursor_x, postzoom_cursor_y, x_pos, y_pos, max_x, max_y);

		mandelbrot_x_offset += (prezoom_cursor_x - postzoom_cursor_x) * mandelbrot_zoom;
		mandelbrot_y_offset -= (prezoom_cursor_y - postzoom_cursor_y) * mandelbrot_zoom;
	}
	else if (fractal_mode == FractalSets::JULIA)
	{
		long double prezoom_cursor_x;
		long double prezoom_cursor_y;
		juliaScale(prezoom_cursor_x, prezoom_cursor_y, x_pos, y_pos, max_x, max_y);

		stationaryZoom(direction, max_x, max_y);

		long double postzoom_cursor_x;
		long double postzoom_cursor_y;
		juliaScale(postzoom_cursor_x, postzoom_cursor_y, x_pos, y_pos, max_x, max_y);

		julia_x_offset += (prezoom_cursor_x - postzoom_cursor_x) * julia_zoom;
		julia_y_offset -= (prezoom_cursor_y - postzoom_cursor_y) * julia_zoom;
	}
}

void Fractal::panUp(long double delta)
{
	if (fractal_mode == FractalSets::MANDELBROT)
	{
		mandelbrot_y_offset += mandelbrot_pan_increment * delta;
	}
	else if (fractal_mode == FractalSets::JULIA)
	{
		julia_y_offset += julia_pan_increment * delta;
	}
}
void Fractal::panDown(long double delta)
{
	if (fractal_mode == FractalSets::MANDELBROT)
	{
		mandelbrot_y_offset -= mandelbrot_pan_increment * delta;
	}
	else if (fractal_mode == FractalSets::JULIA)
	{
		julia_y_offset -= julia_pan_increment * delta;
	}
}
void Fractal::panLeft(long double delta)
{
	if (fractal_mode == FractalSets::MANDELBROT)
	{
		mandelbrot_x_offset -= mandelbrot_pan_increment * delta;
	}
	else if (fractal_mode == FractalSets::JULIA)
	{
		julia_x_offset -= julia_pan_increment * delta;
	}
}
void Fractal::panRight(long double delta)
{
	if (fractal_mode == FractalSets::MANDELBROT)
	{
		mandelbrot_x_offset += mandelbrot_pan_increment * delta;
	}
	else if (fractal_mode == FractalSets::JULIA)
	{
		julia_x_offset += julia_pan_increment * delta;
	}
}

void Fractal::increaseIterations()
{
	if (fractal_mode == FractalSets::MANDELBROT)
	{
		mandelbrot_max_iter = static_cast<int>(mandelbrot_max_iter * mandelbrot_max_iter_multiplier);

#ifdef PRINT_INFO
		std::cout << "Mandelbrot iterations: " << mandelbrot_max_iter << std::endl;
#endif
	}
	else if (fractal_mode == FractalSets::JULIA)
	{
		julia_max_iter = static_cast<int>(julia_max_iter * julia_max_iter_multiplier);

#ifdef PRINT_INFO
		std::cout << "Julia iterations: " << julia_max_iter << std::endl;
#endif
	}
}

void Fractal::decreaseIterations()
{
	if (fractal_mode == FractalSets::MANDELBROT)
	{
		mandelbrot_max_iter = static_cast<int>(mandelbrot_max_iter / mandelbrot_max_iter_multiplier);

#ifdef PRINT_INFO
		std::cout << "Mandelbrot iterations: " << mandelbrot_max_iter << std::endl;
#endif
	}
	else if (fractal_mode == FractalSets::JULIA)
	{
		julia_max_iter = static_cast<int>(julia_max_iter / julia_max_iter_multiplier);

#ifdef PRINT_INFO
		std::cout << "Julia iterations: " << julia_max_iter << std::endl;
#endif
	}
}

void Fractal::reset()
{
	if (fractal_mode == FractalSets::MANDELBROT)
	{
		mandelbrot_zoom = mandelbrot_zoom_DEFAULT;
		mandelbrot_x_offset = mandelbrot_x_offset_DEFAULT;
		mandelbrot_y_offset = mandelbrot_y_offset_DEFAULT;
		mandelbrot_max_iter = mandelbrot_max_iter_DEFAULT;
	}
	else if (fractal_mode == FractalSets::JULIA)
	{
		julia_x_offset = julia_x_offset_DEFAULT;
		julia_y_offset = julia_y_offset_DEFAULT;
		julia_zoom = julia_zoom_DEFAULT;
		julia_max_iter = julia_max_iter_DEFAULT;
	}
}

// Mandlebrot set
void Fractal::mandelbrotScale(long double& scaled_x, long double& scaled_y, int x, int y, int max_x, int max_y)
{
	scaled_x = mandelbrot_x_min + ((mandelbrot_x_max - mandelbrot_x_min) * (static_cast<long double>(x) / max_x));
	scaled_y = mandelbrot_y_min + ((mandelbrot_y_max - mandelbrot_y_min) * (static_cast<long double>(y) / max_y));
	scaled_x = (scaled_x + mandelbrot_x_offset) / mandelbrot_zoom;
	scaled_y = (scaled_y + mandelbrot_y_offset) / mandelbrot_zoom;
}

void Fractal::mandelbrotMatrix(int* matrix, int matrix_width, int matrix_height)
{
	for (int index = 0; index < t_pool->size; ++index)
	{
		if(instruction_mode == InstructionModes::STANDARD)
			t_pool->addJob([=]() {mandelbrotThread(index, matrix, matrix_width, matrix_height, t_pool->size); });
		else
			t_pool->addJob([=]() {mandelbrotAVXThread(index*4, matrix, matrix_width, matrix_height, t_pool->size); });
	}
	t_pool->synchronize();
}

void Fractal::mandelbrotThread(int index, int* matrix, int matrix_width, int matrix_height, int stride)
{
	for (int i = index; i < (matrix_width * matrix_height); i += stride)
	{
		matrix[i] = mandelbrotSetAtPoint(i % matrix_width, i / matrix_width, matrix_width, matrix_height);
	}
}

bool Fractal::mandelbrotBulbCheck(long double x_0, long double y_0)
{
	// Period-2 bulb check
	long double period_2 = ((x_0 + 1.0L) * (x_0 + 1.0L)) + (y_0 * y_0);
	bool within_period_2 = period_2 <= 1.0L / 16.0L;

	return within_period_2;
}

bool Fractal::mandelbrotCardioidCheck(long double x_0, long double y_0)
{
	// Cardioid check
	long double q = ((x_0 - 0.25L) * (x_0 - 0.25L)) + (y_0 * y_0);
	bool within_cardioid = q * (q + (x_0 - 0.25L)) <= 0.25L * (y_0 * y_0);

	return within_cardioid;
}

bool Fractal::mandelbrotPrune(long double x_0, long double y_0)
{
	return	mandelbrotCardioidCheck(x_0, y_0) ||
			mandelbrotBulbCheck(x_0, y_0);
}

int Fractal::mandelbrotSetAtPoint(int x, int y, int max_x, int max_y)
{
	long double x_0;
	long double y_0;
	mandelbrotScale(x_0, y_0, x, y, max_x, max_y);


	if (mandelbrotPrune(x_0, y_0))
		return 0;

	long double x_1 = 0;
	long double y_1 = 0;
	long double x_2 = 0;
	long double y_2 = 0;

	int iter = 0;
	while (x_2 + y_2 <= mandelbrot_radius && iter < mandelbrot_max_iter)
	{
		y_1 = (x_1 + x_1) * y_1 + y_0;
		x_1 = x_2 - y_2 + x_0;
		x_2 = x_1 * x_1;
		y_2 = y_1 * y_1;
		iter += 1;
	}
	if (iter == mandelbrot_max_iter)
		return 0;
	return iter;
}

/*
* Calculate mandelbrot iterations using AVX2 instructions. AVX2 uses 256-bit registers and we do calculations with 64-bit floating point numbers, so we are able
* to calculate 4 points per pass.
*/
void Fractal::mandelbrotAVXThread(int index, int* matrix, int matrix_width, int matrix_height, int stride)
{
	__m256d _index, _radius, _max_x, _max_y, _m_x_min, _m_y_min, _m_x_subbed, _m_y_subbed, _m_x_offset, _m_y_offset, _m_zoom, _x_0, _y_0, _x_1, _y_1, _x_2, _y_2, _mask1, _index_add_mask;
	__m256i _iter, _max_iter, _increment, _one, _mask2;


	_radius = _mm256_set1_pd(mandelbrot_radius);
	_max_x = _mm256_set1_pd(static_cast<long double>(matrix_width));
	_max_y = _mm256_set1_pd(static_cast<long double>(matrix_height));
	_m_x_min = _mm256_set1_pd(mandelbrot_x_min);
	_m_y_min = _mm256_set1_pd(mandelbrot_y_min);
	_m_x_subbed = _mm256_set1_pd(mandelbrot_x_max - mandelbrot_x_min);
	_m_y_subbed = _mm256_set1_pd(mandelbrot_y_max - mandelbrot_y_min);
	_m_x_offset = _mm256_set1_pd(mandelbrot_x_offset);
	_m_y_offset = _mm256_set1_pd(mandelbrot_y_offset);
	_m_zoom = _mm256_set1_pd(mandelbrot_zoom);
	_index_add_mask = _mm256_set_pd(0.0, 1.0, 2.0, 3.0);

	_one = _mm256_set1_epi64x(1);
	_max_iter = _mm256_set1_epi64x(mandelbrot_max_iter);

	for (int i = index; i < (matrix_width * matrix_height); i += (stride * 4))
	{
		_index = _mm256_set1_pd(static_cast<long double>(i));
		_index = _mm256_add_pd(_index, _index_add_mask);

		// Convert from flat index to 2-D x and y
		_x_0 = _mm256_div_pd(_index, _max_x);
		_x_0 = _mm256_floor_pd(_x_0);
		_x_0 = _mm256_mul_pd(_max_x, _x_0);
		_x_0 = _mm256_sub_pd(_index, _x_0);

		_y_0 = _mm256_div_pd(_index, _max_x);
		_y_0 = _mm256_floor_pd(_y_0);

		// Scaling from window pixels to Cartesian X and Y
		// x_0 = mandelbrot_x_min + ((mandelbrot_x_max - mandelbrot_x_min) * long double(x) / max_x);
		_x_0 = _mm256_div_pd(_x_0, _max_x);
		_x_0 = _mm256_mul_pd(_x_0, _m_x_subbed);
		_x_0 = _mm256_add_pd(_x_0, _m_x_min);
		// y_0 = mandelbrot_y_min + ((mandelbrot_y_max - mandelbrot_y_min) * long double(y) / max_y);
		_y_0 = _mm256_div_pd(_y_0, _max_y);
		_y_0 = _mm256_mul_pd(_y_0, _m_y_subbed);
		_y_0 = _mm256_add_pd(_y_0, _m_y_min);

		// x_0 = (x_0 + mandelbrot_x_offset) / mandelbrot_zoom;
		_x_0 = _mm256_add_pd(_x_0, _m_x_offset);
		_x_0 = _mm256_div_pd(_x_0, _m_zoom);
		// y_0 = (y_0 + mandelbrot_y_offset) / mandelbrot_zoom;
		_y_0 = _mm256_add_pd(_y_0, _m_y_offset);
		_y_0 = _mm256_div_pd(_y_0, _m_zoom);


		_x_1 = _mm256_set1_pd(0.0);
		_y_1 = _mm256_set1_pd(0.0);
		_x_2 = _mm256_set1_pd(0.0);
		_y_2 = _mm256_set1_pd(0.0);
		_iter = _mm256_set1_epi64x(1);

	loop:

		_y_1 = _mm256_fmadd_pd(_mm256_add_pd(_x_1, _x_1), _y_1, _y_0);  // y_1 = (x_1 + x_1) * y_1 + y_0;
		_x_1 = _mm256_add_pd(_mm256_sub_pd(_x_2, _y_2), _x_0);			// x_1 = x_2 - y_2 + x_0;
		_x_2 = _mm256_mul_pd(_x_1, _x_1);								// x_2 = x_1 * x_1;
		_y_2 = _mm256_mul_pd(_y_1, _y_1);								// y_2 = y_1 * y_1;

		_mask1 = _mm256_cmp_pd(_mm256_add_pd(_x_2, _y_2), _radius, _CMP_LE_OQ);  // is x_2 + x_2 <= mandelbrot_radius?
		_mask2 = _mm256_cmpgt_epi64(_max_iter, _iter);							 // is iter < max_iter?
		_mask2 = _mm256_and_si256(_mask2, _mm256_castpd_si256(_mask1));			 // AND the two masks together, since we dont want to increment if either of the two conditions above are false
		_increment = _mm256_and_si256(_one, _mask2);
		_iter = _mm256_add_epi64(_iter, _increment);
		if (_mm256_movemask_pd(_mm256_castsi256_pd(_mask2)) > 0)		// Loop if any of the 4 values in the vector dont violate either mask condition
			goto loop;

		// If any of the iteration values = max_iter, set them to 0
		_mask2 = _mm256_cmpeq_epi64(_iter, _max_iter);
		_iter = _mm256_andnot_si256(_mask2, _iter);

		// Extract vector values
		// These iter values should always be positive and should never get too high, so casting from 64-bit int to 32-bit int should not be a problem
		matrix[i]	  = static_cast<int>(_iter.m256i_i64[3]);
		matrix[i + 1] = static_cast<int>(_iter.m256i_i64[2]);
		matrix[i + 2] = static_cast<int>(_iter.m256i_i64[1]);
		matrix[i + 3] = static_cast<int>(_iter.m256i_i64[0]);
	}
}

// Julia set
void Fractal::juliaScale(long double& scaled_x, long double& scaled_y, int x, int y, int max_x, int max_y)
{
	scaled_x = -julia_escape_radius + (2.0 * julia_escape_radius * (static_cast<long double>(x) / max_x));
	scaled_y = -julia_escape_radius + (2.0 * julia_escape_radius * (static_cast<long double>(y) / max_y));
	scaled_x = (scaled_x + julia_x_offset) / julia_zoom;
	scaled_y = (scaled_y + julia_y_offset) / julia_zoom / (static_cast<long double>(max_x) / max_y); // We want to stretch out the y-axis since the window frame most likely has a larger width (e.g. 1280x720)
}

void Fractal::juliaMatrix(int* matrix, int matrix_width, int matrix_height)
{
	for (int index = 0; index < t_pool->size; ++index)
	{
		t_pool->addJob([=]() {juliaThread(index, matrix, matrix_width, matrix_height, t_pool->size); });
	}
	t_pool->synchronize();
}

void Fractal::juliaThread(int index, int* matrix, int matrix_width, int matrix_height, int stride)
{
	for (int i = index; i < (matrix_width * matrix_height); i += stride)
	{
		matrix[i] = juliaSetAtPoint(i % matrix_width, i / matrix_width, matrix_width, matrix_height);
	}
}

int Fractal::juliaSetAtPoint(int x, int y, int max_x, int max_y)
{
	long double zx;
	long double zy;
	juliaScale(zx, zy, x, y, max_x, max_y);

	int iter = 0;

	long double temp;
	while (zx * zx + zy * zy < (julia_escape_radius * julia_escape_radius) && iter < julia_max_iter)
	{
		temp = zx * zx - zy * zy;
		zy = 2 * zx * zy + julia_complex_param.imag();
		zx = temp + julia_complex_param.real();

		iter += 1;
	}
	
	if (iter == julia_max_iter)
		return 0;
	else
		return iter;
}

void Fractal::switchFractal()
{
	fractal_mode = (FractalSets)((static_cast<int>(fractal_mode) + 1) % static_cast<int>(FractalSets::LAST));
}

void Fractal::switchInstruction()
{
	instruction_mode = (InstructionModes)((static_cast<int>(instruction_mode) + 1) % static_cast<int>(InstructionModes::LAST));

#ifdef PRINT_INFO
	if (static_cast<int>(instruction_mode) == 0)
		std::cout << "Using standard instructions:" << std::endl;
	else
		std::cout << "Using AVX:" << std::endl;
#endif
}

void Fractal::generate(int* matrix, int matrix_width, int matrix_height, ColorGenerator& cg)
{
#ifdef PRINT_INFO
	std::cout << "Fractal generation: ";
	START_TIMER
#endif

	int n = 0;
	if (fractal_mode == FractalSets::MANDELBROT)
	{
		n = mandelbrot_max_iter;
		mandelbrotMatrix(matrix, matrix_width, matrix_height);
	}
	else if (fractal_mode == FractalSets::JULIA)
	{
		n = julia_max_iter;
		juliaMatrix(matrix, matrix_width, matrix_height);
	}

#ifdef PRINT_INFO
	END_TIMER
	std::cout << "Color generation: ";
	START_TIMER
#endif

	cg.generate(matrix, matrix_width, matrix_height, n);

#ifdef PRINT_INFO
	END_TIMER
	std::cout << "----------------------------------------" << std::endl;
#endif
}