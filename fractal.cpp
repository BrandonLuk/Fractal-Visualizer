#include "fractal.h"

#include "thread_pool.h"

#include <cmath>
#include <complex>
#include <functional>
#include <thread>
#include <vector>

#include <immintrin.h> // AVX intrinsics

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

////////////////////////////////////////////////////////////
/// Mandelbrot Set Functions
////////////////////////////////////////////////////////////

//Mandelbrot set functions for standard instruction set

void Fractal::mandelbrotScale(long double& scaled_x, long double& scaled_y, int x, int y, int max_x, int max_y)
{
	scaled_x = mandelbrot_x_min + ((mandelbrot_x_max - mandelbrot_x_min) * (static_cast<long double>(x) / max_x));
	scaled_y = mandelbrot_y_min + ((mandelbrot_y_max - mandelbrot_y_min) * (static_cast<long double>(y) / max_y));
	scaled_x = (scaled_x + mandelbrot_x_offset) / mandelbrot_zoom;
	scaled_y = (scaled_y + mandelbrot_y_offset) / mandelbrot_zoom;
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

	unsigned int period_check = 10;
	long double check_x = x_0;
	long double check_y = y_0;

	int iter = 0;


	while (iter < mandelbrot_max_iter)
	{
		for (; iter < period_check; ++iter)
		{
			y_1 = (x_1 + x_1) * y_1 + y_0;
			x_1 = x_2 - y_2 + x_0;
			x_2 = x_1 * x_1;
			y_2 = y_1 * y_1;

			if (x_2 + y_2 > mandelbrot_radius)
				return iter;
			if (x_2 == check_x && y_2 == check_y)
				return 0;
		}
		
		check_x = x_2;
		check_y = y_2;
		period_check += period_check;
		if (period_check > mandelbrot_max_iter)
			period_check = mandelbrot_max_iter;
	}
	return 0;

	// Loop without period checking
	/*while (x_2 + y_2 <= mandelbrot_radius && iter < mandelbrot_max_iter)
	{
		y_1 = (x_1 + x_1) * y_1 + y_0;
		x_1 = x_2 - y_2 + x_0;
		x_2 = x_1 * x_1;
		y_2 = y_1 * y_1;
		iter += 1;
	}
	if (iter == mandelbrot_max_iter)
		return 0;
	return iter;*/
}

void Fractal::mandelbrotThread(int index, int* matrix, int matrix_width, int matrix_height, int stride)
{
	for (int i = index; i < (matrix_width * matrix_height); i += stride)
	{
		matrix[i] = mandelbrotSetAtPoint(i % matrix_width, i / matrix_width, matrix_width, matrix_height);
	}
}

/*
*	Adds jobs to the ThreadPool job queue that will fill the supplied matrix with Mandelbrot set iteration values.
*/
void Fractal::mandelbrotMatrix(int* matrix, int matrix_width, int matrix_height)
{
	for (int index = 0; index < t_pool->size; ++index)
	{
		t_pool->addJob([=]() {mandelbrotThread(index, matrix, matrix_width, matrix_height, t_pool->size); });
	}
	t_pool->synchronize();
}


//	Mandelbrot set functions for AVX instruction set

__m256d Fractal::mandelbrotBulbCheckAVX(const __m256d& _x, const __m256d& _y)
{
	__m256d _period_2, _one, _sixteen;

	_one = _mm256_set1_pd(1.0);
	_sixteen = _mm256_set1_pd(16.0);

	_period_2 = _mm256_add_pd(_x, _one);
	_period_2 = _mm256_mul_pd(_period_2, _period_2);
	_period_2 = _mm256_fmadd_pd(_y, _y, _period_2);

	return _mm256_cmp_pd(_period_2, _mm256_div_pd(_one, _sixteen), _CMP_LE_OQ);
}
__m256d Fractal::mandelbrotCardioidCheckAVX(const __m256d& _x, const __m256d& _y)
{
	__m256d _q, _l, _r, _x_sub_quarter, _quarter;

	_quarter = _mm256_set1_pd(0.25);
	_x_sub_quarter = _mm256_sub_pd(_x, _quarter); // (x_0 - 0.25) is used several times here, so we will do the computation once and store it

	_q = _mm256_mul_pd(_x_sub_quarter, _x_sub_quarter);
	_q = _mm256_fmadd_pd(_y, _y, _q);

	// Left side of the inequality
	_l = _mm256_add_pd(_q, _x_sub_quarter);
	_l = _mm256_mul_pd(_q, _l);

	// Right side of the inequality
	_r = _mm256_mul_pd(_y, _y);
	_r = _mm256_mul_pd(_r, _quarter);

	return _mm256_cmp_pd(_l, _r, _CMP_LE_OQ);
}

bool Fractal::mandelbrotPruneAVX(const __m256d& _x, const __m256d& _y)
{
	__m256d _mask1, _mask2;

	_mask1 = mandelbrotBulbCheckAVX(_x, _y);
	_mask2 = mandelbrotCardioidCheckAVX(_x, _y);

	_mask1 = _mm256_or_pd(_mask1, _mask2);

	/* "_mm256_movemask_pd" will return an int whose individiual bits are set to 1 in correlation to the 1's in the given mask.
	*  Therefore, if every value in the mask is true, movemask will return 1111 which is 0xF.
	*  We only want to return true if each of the 4 points can be pruned. Even if there is only one point that cannot be pruned, we still will return false
	*  so that that one point can be iterated over.
	*/
	if (_mm256_movemask_pd(_mask1) == 0xF)
		return true;

	return false;
}

/*
* Calculate mandelbrot iterations using AVX2 instructions. AVX2 uses 256-bit registers and we do calculations with 64-bit floating point numbers, so we are able
* to calculate 4 points per pass.
*/
void Fractal::mandelbrotAVXThread(int index, int* matrix, int matrix_width, int matrix_height, int stride)
{
	__m256d _index, _radius, _max_x, _max_y, _m_x_min, _m_y_min, _m_x_subbed, _m_y_subbed, _m_x_offset, _m_y_offset, _m_zoom, _x_0, _y_0, _x_1, _y_1, _x_2, _y_2, _mask1, _index_add_mask, _check_x, _check_y;
	__m256i _iter, _max_iter, _active, _one, _mask2;
	int period, period_check;


	_radius		= _mm256_set1_pd(mandelbrot_radius);
	_max_x		= _mm256_set1_pd(static_cast<long double>(matrix_width));
	_max_y		= _mm256_set1_pd(static_cast<long double>(matrix_height));
	_m_x_min	= _mm256_set1_pd(mandelbrot_x_min);
	_m_y_min	= _mm256_set1_pd(mandelbrot_y_min);
	_m_x_subbed = _mm256_set1_pd(mandelbrot_x_max - mandelbrot_x_min);
	_m_y_subbed = _mm256_set1_pd(mandelbrot_y_max - mandelbrot_y_min);
	_m_x_offset = _mm256_set1_pd(mandelbrot_x_offset);
	_m_y_offset = _mm256_set1_pd(mandelbrot_y_offset);
	_m_zoom		= _mm256_set1_pd(mandelbrot_zoom);

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

		// Check to see if each of these 4 points are guaranteed to be in the cardiod or the bulb. If so, set them all to 0 and assign.
		if (mandelbrotPruneAVX(_x_0, _y_0))
		{
			_iter = _mm256_set1_epi64x(0);
			goto assign;
		}

		_x_1 = _mm256_set1_pd(0.0);
		_y_1 = _mm256_set1_pd(0.0);
		_x_2 = _mm256_set1_pd(0.0);
		_y_2 = _mm256_set1_pd(0.0);
		_iter = _mm256_set1_epi64x(0);
		_active = _mm256_set1_epi64x(-1);
		period = 10;
		period_check = 0;

		_check_x = _x_0;
		_check_y = _y_0;

	loop:
		for (; period_check < period; ++period_check)
		{
			_y_1 = _mm256_fmadd_pd(_mm256_add_pd(_x_1, _x_1), _y_1, _y_0);  // y_1 = (x_1 + x_1) * y_1 + y_0;
			_x_1 = _mm256_add_pd(_mm256_sub_pd(_x_2, _y_2), _x_0);			// x_1 = x_2 - y_2 + x_0;
			_x_2 = _mm256_mul_pd(_x_1, _x_1);								// x_2 = x_1 * x_1;
			_y_2 = _mm256_mul_pd(_y_1, _y_1);								// y_2 = y_1 * y_1;


			//if (x_2 + y_2 <= mandelbrot_radius)
			_mask1 = _mm256_cmp_pd(_mm256_add_pd(_x_2, _y_2), _radius, _CMP_LE_OQ);
			// Each point that violates the above is marked as inactive so that its iteration count it not incremented anymore
			_active = _mm256_and_si256(_active, _mm256_castpd_si256(_mask1));

			//if (x_2 != check_x || y_2 != check_y)
			_mask1 = _mm256_or_pd(_mm256_cmp_pd(_x_2, _check_x, _CMP_NEQ_OQ), _mm256_cmp_pd(_y_2, _check_y, _CMP_NEQ_OQ));
			// Each point that violates the above should have its iteration count set to 0, but only if that point is still active
			_iter = _mm256_and_si256(_iter, _mm256_or_si256(_mm256_castpd_si256(_mask1), _mm256_xor_si256(_active, _mm256_set1_epi64x(-1))));
			// Set the points that violate the above as inactive
			_active = _mm256_and_si256(_active, _mm256_castpd_si256(_mask1));
			

			// Check to see if all of the points are inactive. If they are we are done and will jump to assign
			if (_mm256_movemask_pd(_mm256_castsi256_pd(_active)) == 0)
				goto assign;
			// At least one point is still active, so we increment
			_iter = _mm256_add_epi64(_iter, _mm256_and_si256(_one, _active)); // one AND active
		}

		// If any points iteration count has reached the max we are done
		_mask2 = _mm256_cmpeq_epi64(_iter, _max_iter);
		if (_mm256_movemask_pd(_mm256_castsi256_pd(_mask2)) > 0)
			goto assign;

		_check_x = _x_2;
		_check_y = _y_2;
		period += period;
		if (period > mandelbrot_max_iter)
			period = mandelbrot_max_iter;
		goto loop;

	// Loop without period checking
	//loop:

	//	_y_1 = _mm256_fmadd_pd(_mm256_add_pd(_x_1, _x_1), _y_1, _y_0);  // y_1 = (x_1 + x_1) * y_1 + y_0;
	//	_x_1 = _mm256_add_pd(_mm256_sub_pd(_x_2, _y_2), _x_0);			// x_1 = x_2 - y_2 + x_0;
	//	_x_2 = _mm256_mul_pd(_x_1, _x_1);								// x_2 = x_1 * x_1;
	//	_y_2 = _mm256_mul_pd(_y_1, _y_1);								// y_2 = y_1 * y_1;

	//	_mask1 = _mm256_cmp_pd(_mm256_add_pd(_x_2, _y_2), _radius, _CMP_LE_OQ);  // is x_2 + x_2 <= mandelbrot_radius?
	//	_mask2 = _mm256_cmpgt_epi64(_max_iter, _iter);							 // is iter < max_iter?
	//	_mask2 = _mm256_and_si256(_mask2, _mm256_castpd_si256(_mask1));			 // AND the two masks together, since we dont want to increment if either of the two conditions above are false
	//	_increment = _mm256_and_si256(_one, _mask2);
	//	_iter = _mm256_add_epi64(_iter, _increment);
	//	if (_mm256_movemask_pd(_mm256_castsi256_pd(_mask2)) > 0)		// Loop if any of the 4 values in the vector dont violate either mask condition
	//		goto loop;



	assign:

		// If any of the iteration values = max_iter, set them to 0
		_mask2 = _mm256_cmpeq_epi64(_iter, _max_iter);
		_iter = _mm256_andnot_si256(_mask2, _iter);

		// Extract vector values
		// These iter values should never get too high, so casting from 64-bit int to 32-bit int should not be a problem
		matrix[i]	  = static_cast<int>(_iter.m256i_i64[3]);
		matrix[i + 1] = static_cast<int>(_iter.m256i_i64[2]);
		matrix[i + 2] = static_cast<int>(_iter.m256i_i64[1]);
		matrix[i + 3] = static_cast<int>(_iter.m256i_i64[0]);
	}
}

void Fractal::mandelbrotMatrixAVX(int* matrix, int matrix_width, int matrix_height)
{
	for (int index = 0; index < t_pool->size; ++index)
	{
		t_pool->addJob([=]() {mandelbrotAVXThread(index * 4, matrix, matrix_width, matrix_height, t_pool->size); });
	}
	t_pool->synchronize();
}

////////////////////////////////////////////////////////////
/// Julia Set Functions
////////////////////////////////////////////////////////////
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


////////////////////////////////////////////////////////////
/// Fractal Parameter Adjustment Functions
////////////////////////////////////////////////////////////

void Fractal::switchFractal()
{
	fractal_mode = (FractalSets)((static_cast<int>(fractal_mode) + 1) % static_cast<int>(FractalSets::LAST));
}

void Fractal::generate(int* matrix, int matrix_width, int matrix_height, ColorGenerator& cg, bool AVX)
{
#ifdef PRINT_INFO
	if (AVX)
		std::cout << "Using AVX instructions..." << std::endl;
	else
		std::cout << "Using standard instructions..." << std::endl;
	std::cout << "Fractal generation: ";
	START_TIMER
#endif

	int n = 0;
	if (fractal_mode == FractalSets::MANDELBROT)
	{
		n = mandelbrot_max_iter;
		if (AVX)
			mandelbrotMatrixAVX(matrix, matrix_width, matrix_height);
		else
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

	if (AVX)
		cg.generateAVX(matrix, matrix_width, matrix_height, n);
	else
		cg.generate(matrix, matrix_width, matrix_height, n);

#ifdef PRINT_INFO
	END_TIMER
	std::cout << "----------------------------------------" << std::endl;
#endif
}