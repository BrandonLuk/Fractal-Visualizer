#include "fractal.h"

#include "thread_pool.h"

#include <cmath>
#include <complex>
#include <functional>
#include <thread>
#include <vector>

Fractal::Fractal()
{
	mode = FractalSets::JULIA;

	x_offset = 0.0;
	y_offset = 0.0;
	zoom_degree = 1.0;

	mandelbrot_x_min = -2.5;
	mandelbrot_x_max = 1.0;
	mandelbrot_y_min = -1.0;
	mandelbrot_y_max = 1.0;
	mandelbrot_radius = 4.0;
	mandelbrot_zoom = 1.0;
	mandelbrot_zoom_increment = 0.95;
	mandelbrot_pan_increment = 0.05;
	mandelbrot_max_iter = 200;
	

	julia_n = 2;
	julia_max_iter = 200;
	julia_escape_radius = 2;
	julia_complex_param = std::complex<long double>(1.0 - ((1.0 + std::sqrt(5)) / 2.0), 0.0);

	t_pool = &ThreadPool::getInstance();
}

void Fractal::zoom(double zoom_amount)
{
	if (mode == FractalSets::MANDELBROT)
	{
		mandelbrot_zoom *= mandelbrot_zoom_increment;
	}
	else if (mode == FractalSets::JULIA)
	{
		zoom_degree *= 0.95;
	}
}

void Fractal::panUp()
{
	if (mode == FractalSets::MANDELBROT)
	{
		mandelbrot_y_min += mandelbrot_pan_increment;
		mandelbrot_y_max += mandelbrot_pan_increment;
	}
	else if (mode == FractalSets::JULIA)
	{
		y_offset += 0.01;
	}
}
void Fractal::panDown()
{
	if (mode == FractalSets::MANDELBROT)
	{
		mandelbrot_y_min -= mandelbrot_pan_increment;
		mandelbrot_y_max -= mandelbrot_pan_increment;
	}
	else if (mode == FractalSets::JULIA)
	{
		y_offset -= 0.01;
	}
}
void Fractal::panLeft()
{
	if (mode == FractalSets::MANDELBROT)
	{
		mandelbrot_x_min -= mandelbrot_pan_increment;
		mandelbrot_x_max -= mandelbrot_pan_increment;
	}
	else if (mode == FractalSets::JULIA)
	{
		x_offset -= 0.01;
	}
}
void Fractal::panRight()
{
	if (mode == FractalSets::MANDELBROT)
	{
		mandelbrot_x_min += mandelbrot_pan_increment;
		mandelbrot_x_max += mandelbrot_pan_increment;
	}
	else if (mode == FractalSets::JULIA)
	{
		x_offset += 0.01;
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

int Fractal::mandelbrotSetAtPoint(int x, int y, int max_x, int max_y)
{
	long double x_0 = mandelbrot_x_min + ((mandelbrot_x_max - mandelbrot_x_min) * long double(x) / max_x);
	long double y_0 = mandelbrot_y_min + ((mandelbrot_y_max - mandelbrot_y_min) * long double(y) / max_y);
	x_0 *= mandelbrot_zoom;
	y_0 *= mandelbrot_zoom;

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
	cg.generate(matrix, matrix_width, matrix_height, n);
}