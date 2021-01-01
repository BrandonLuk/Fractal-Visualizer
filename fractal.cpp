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
#define START_TIMER auto start = std::chrono::steady_clock::now();
#define END_TIMER   auto dur = std::chrono::steady_clock::now() - start; \
                    std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(dur).count() << std::endl;

Fractal::Fractal()
{
	mode = FractalSets::MANDELBROT;

	x_offset = 0.0;
	y_offset = 0.0;
	zoom_degree = 1.0;

	mandelbrot_x_min			  = mandelbrot_x_min_DEFAULT;
	mandelbrot_x_max			  = mandelbrot_x_max_DEFAULT;
	mandelbrot_y_min			  = mandelbrot_y_min_DEFAULT;
	mandelbrot_y_max			  = mandelbrot_y_max_DEFAULT;
	mandelbrot_radius			  = mandelbrot_radius_DEFAULT;
	mandelbrot_zoom				  = mandelbrot_zoom_DEFAULT;
	mandelbrot_zoom_multiplier	  = mandelbrot_zoom_multiplier_DEFAULT;
	mandelbrot_x_offset			  = mandelbrot_x_offset_DEFAULT;
	mandelbrot_y_offset			  = mandelbrot_y_offset_DEFAULT;
	mandelbrot_pan_increment	  = mandelbrot_pan_increment_DEFAULT;
	mandelbrot_max_iter			  = mandelbrot_max_iter_DEFAULT;
	mandelbrot_max_iter_multiplier= mandelbrot_max_iter_multiplier_DEFAULT;
	

	julia_n = 2;
	julia_max_iter = 200;
	julia_escape_radius = 2;
	julia_complex_param = std::complex<long double>(1.0 - ((1.0 + std::sqrt(5)) / 2.0), 0.0);

	t_pool = &ThreadPool::getInstance();
}

void Fractal::stationaryZoom(int direction)
{
	if (mode == FractalSets::MANDELBROT)
	{
		long double prezoom_center_x = mandelbrot_x_min + ((mandelbrot_x_max - mandelbrot_x_min) * 0.5);
		prezoom_center_x = (prezoom_center_x + mandelbrot_x_offset) / mandelbrot_zoom;
		long double prezoom_center_y = mandelbrot_y_min + ((mandelbrot_y_max - mandelbrot_y_min) * 0.5);
		prezoom_center_y = (prezoom_center_y + mandelbrot_y_offset) / mandelbrot_zoom;

		if (direction > 0)
		{
			mandelbrot_zoom *= (1.0 + mandelbrot_zoom_multiplier);
		}
		else
		{
			mandelbrot_zoom *= (1.0 - mandelbrot_zoom_multiplier);
		}

		long double postzoom_center_x = mandelbrot_x_min + ((mandelbrot_x_max - mandelbrot_x_min) * 0.5);
		postzoom_center_x = (postzoom_center_x + mandelbrot_x_offset) / mandelbrot_zoom;
		long double postzoom_center_y = mandelbrot_y_min + ((mandelbrot_y_max - mandelbrot_y_min) * 0.5);
		postzoom_center_y = (postzoom_center_y + mandelbrot_y_offset) / mandelbrot_zoom;

		mandelbrot_x_offset += (prezoom_center_x - postzoom_center_x) * mandelbrot_zoom;
		mandelbrot_y_offset += (prezoom_center_y - postzoom_center_y) * mandelbrot_zoom;
	}
	else if (mode == FractalSets::JULIA)
	{
		zoom_degree *= 0.95;
	}
}

void Fractal::followingZoom(double zoom_amount, int x_pos, int y_pos, int max_x, int max_y)
{
	if (mode == FractalSets::MANDELBROT)
	{
		long double prezoom_cursor_x = mandelbrot_x_min + ((mandelbrot_x_max - mandelbrot_x_min) * (long double(x_pos) / max_x));
		prezoom_cursor_x = (prezoom_cursor_x + mandelbrot_x_offset) / mandelbrot_zoom;
		long double prezoom_cursor_y = mandelbrot_y_min + ((mandelbrot_y_max - mandelbrot_y_min) * (long double(y_pos) / max_y));
		prezoom_cursor_y = (prezoom_cursor_y + mandelbrot_y_offset) / mandelbrot_zoom;

		stationaryZoom(zoom_amount);

		long double postzoom_cursor_x = mandelbrot_x_min + ((mandelbrot_x_max - mandelbrot_x_min) * (long double(x_pos) / max_x));
		postzoom_cursor_x = (postzoom_cursor_x + mandelbrot_x_offset) / mandelbrot_zoom;
		long double postzoom_cursor_y = mandelbrot_y_min + ((mandelbrot_y_max - mandelbrot_y_min) * (long double(y_pos) / max_y));
		postzoom_cursor_y = (postzoom_cursor_y + mandelbrot_y_offset) / mandelbrot_zoom;

		mandelbrot_x_offset += (prezoom_cursor_x - postzoom_cursor_x) * mandelbrot_zoom;
		mandelbrot_y_offset += (prezoom_cursor_y - postzoom_cursor_y) * mandelbrot_zoom;
	}
	else if (mode == FractalSets::JULIA)
	{
		zoom_degree *= 0.95;
	}
}

void Fractal::panUp(long double delta)
{
	if (mode == FractalSets::MANDELBROT)
	{
		mandelbrot_y_offset -= mandelbrot_pan_increment * delta;
	}
	else if (mode == FractalSets::JULIA)
	{
		y_offset += 0.01;
	}
}
void Fractal::panDown(long double delta)
{
	if (mode == FractalSets::MANDELBROT)
	{
		mandelbrot_y_offset += mandelbrot_pan_increment * delta;
	}
	else if (mode == FractalSets::JULIA)
	{
		y_offset -= 0.01;
	}
}
void Fractal::panLeft(long double delta)
{
	if (mode == FractalSets::MANDELBROT)
	{
		mandelbrot_x_offset -= mandelbrot_pan_increment * delta;
	}
	else if (mode == FractalSets::JULIA)
	{
		x_offset -= 0.01;
	}
}
void Fractal::panRight(long double delta)
{
	if (mode == FractalSets::MANDELBROT)
	{
		mandelbrot_x_offset += mandelbrot_pan_increment * delta;
	}
	else if (mode == FractalSets::JULIA)
	{
		x_offset += 0.01;
	}
}

void Fractal::increaseIterations()
{
	if (mode == FractalSets::MANDELBROT)
	{
		mandelbrot_max_iter *= mandelbrot_max_iter_multiplier;
		std::cout << "Iterations: " << mandelbrot_max_iter << std::endl;
	}
	else if (mode == FractalSets::JULIA)
	{
		
	}
}

void Fractal::decreaseIterations()
{
	if (mode == FractalSets::MANDELBROT)
	{
		mandelbrot_max_iter /= mandelbrot_max_iter_multiplier;
		std::cout << "Iterations: " << mandelbrot_max_iter << std::endl;
	}
	else if (mode == FractalSets::JULIA)
	{

	}
}

void Fractal::reset()
{
	if (mode == FractalSets::MANDELBROT)
	{
		mandelbrot_x_min = mandelbrot_x_min_DEFAULT;
		mandelbrot_x_max = mandelbrot_x_max_DEFAULT;
		mandelbrot_y_min = mandelbrot_y_min_DEFAULT;
		mandelbrot_y_max = mandelbrot_y_max_DEFAULT;
		mandelbrot_radius = mandelbrot_radius_DEFAULT;
		mandelbrot_zoom = mandelbrot_zoom_DEFAULT;
		mandelbrot_zoom_multiplier = mandelbrot_zoom_multiplier_DEFAULT;
		mandelbrot_x_offset = mandelbrot_x_offset_DEFAULT;
		mandelbrot_y_offset = mandelbrot_y_offset_DEFAULT;
		mandelbrot_pan_increment = mandelbrot_pan_increment_DEFAULT;
		mandelbrot_max_iter = mandelbrot_max_iter_DEFAULT;
		mandelbrot_max_iter_multiplier = mandelbrot_max_iter_multiplier_DEFAULT;
	}
	else if (mode == FractalSets::JULIA)
	{

	}
}

// Mandlebrot set
void Fractal::mandelbrotMatrix(int* matrix, int matrix_width, int matrix_height)
{
	for (int index = 0; index < t_pool->size; ++index)
	{
		t_pool->addJob([=]() {mandelbrotThread(index, matrix, matrix_width, matrix_height, t_pool->size); });
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

bool Fractal::mandelbrot_bulb_check(long double x_0, long double y_0)
{
	// Period-2 bulb check
	long double period_2 = ((x_0 + 1) * (x_0 + 1)) + (y_0 * y_0);
	bool within_period_2 = period_2 <= 1.0L / 16.0L;

	return within_period_2;
}

bool Fractal::mandelbrot_cardioid_check(long double x_0, long double y_0)
{
	// Cardioid check
	long double q = ((x_0 - 0.25L) * (x_0 - 0.25L)) + (y_0 * y_0);
	bool within_cardioid = q * (q + (x_0 - 0.25L)) <= 0.25L * (y_0 * y_0);

	return within_cardioid;
}

bool Fractal::mandelbrot_prune(long double x_0, long double y_0)
{
	return	mandelbrot_cardioid_check(x_0, y_0) ||
			mandelbrot_bulb_check(x_0, y_0);
}

int Fractal::mandelbrotSetAtPoint(int x, int y, int max_x, int max_y)
{
	long double x_0 = mandelbrot_x_min + ((mandelbrot_x_max - mandelbrot_x_min) * long double(x) / max_x);
	long double y_0 = mandelbrot_y_min + ((mandelbrot_y_max - mandelbrot_y_min) * long double(y) / max_y);
	x_0 = (x_0 + mandelbrot_x_offset) / mandelbrot_zoom;
	y_0 = (y_0 - mandelbrot_y_offset) / mandelbrot_zoom;

	if (mandelbrot_prune(x_0, y_0))
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

// Julia set
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
	long double zx = -julia_escape_radius + (2.0 * zoom_degree * julia_escape_radius * x / max_x) + x_offset;
	long double zy = -julia_escape_radius + (2.0 * zoom_degree * julia_escape_radius * y / max_y) + y_offset;

	int iter = 0;

	long double temp;
	while (zx * zx + zy * zy < std::powl(julia_escape_radius, 2.0) && iter < julia_max_iter)
	{
		temp = std::powl(zx * zx + zy * zy, long double(julia_n) / 2.0) *
			std::cosl(julia_n * std::atan2l(zy, zx)) + julia_complex_param.real();

		zy = std::powl(zx * zx + zy * zy, long double(julia_n) / 2.0) * std::sinl(julia_n * std::atan2l(zy, zx)) + julia_complex_param.imag();
		zx = temp;

		iter += 1;
	}
	
	if (iter == julia_max_iter)
		return 0;
	else
		return iter;
}

void Fractal::generate(int* matrix, int matrix_width, int matrix_height, ColorGenerator& cg)
{
	START_TIMER
	int n = 0;
	if (mode == FractalSets::MANDELBROT)
	{
		n = mandelbrot_max_iter;
		mandelbrotMatrix(matrix, matrix_width, matrix_height);
	}
	else if (mode == FractalSets::JULIA)
	{
		n = julia_max_iter;
		juliaMatrix(matrix, matrix_width, matrix_height);
	}
	END_TIMER
	//START_TIMER
	cg.generate(matrix, matrix_width, matrix_height, n);
	//END_TIMER
}