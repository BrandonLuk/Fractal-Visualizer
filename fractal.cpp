#include "fractal.h"

#include "thread_pool.h"

#include <cmath>
#include <complex>
#include <functional>
#include <thread>
#include <vector>

Fractal::Fractal()
{
	x_offset = 0.0;
	y_offset = 0.0;
	zoom_degree = 1.0;

	mandelbrot_x_min = -2.5;
	mandelbrot_x_max = 1.0;
	mandelbrot_y_min = -1.0;
	mandelbrot_y_max = 1.0;
	mandelbrot_max_iter = 100;
	mandelbrot_radius = 4.0;

	julia_n = 2;
	julia_max_iter = 1000;
	julia_escape_radius = 2;
	julia_complex_param = std::complex<long double>(1.0 - ((1.0 + std::sqrt(5)) / 2.0), 0.0);

	t_pool = &ThreadPool::getInstance();
}

void Fractal::zoom(double zoom_amount)
{
	zoom_degree -= 0.05 * zoom_amount;
}

void Fractal::panUp()
{
	y_offset += 0.01;
}
void Fractal::panDown()
{
	y_offset -= 0.01;
}
void Fractal::panLeft()
{
	x_offset -= 0.01;
}
void Fractal::panRight()
{
	x_offset += 0.01;
}


// Mandlebrot set
void Fractal::coloredMandelbrotMatrix(int* matrix, int matrix_width, int matrix_height, ColorGenerator& cg)
{
	for (int index = 0; index < t_pool->size; ++index)
	{
		t_pool->addJob([=, &cg]() {coloredMandelbrotThread(index, matrix, matrix_width, matrix_height, t_pool->size, cg); });
	}
	t_pool->synchronize();
}

void Fractal::coloredMandelbrotThread(int index, int* matrix, int matrix_width, int matrix_height, int stride, ColorGenerator& cg)
{
	for (int i = index; i < (matrix_width * matrix_height); i += stride)
	{
		matrix[i] = cg.colorClamp(mandelbrotSetAtPoint(i % matrix_width, i / matrix_width, matrix_width, matrix_height), mandelbrot_max_iter);
	}
}

int Fractal::mandelbrotSetAtPoint(int x, int y, int max_x, int max_y)
{
	long double x_0 = mandelbrot_x_min + ((mandelbrot_x_max - mandelbrot_x_min) * long double(x) / max_x);
	long double y_0 = mandelbrot_y_min + ((mandelbrot_y_max - mandelbrot_y_min) * long double(y) / max_y);
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
	return iter;
}

// Julia set
void Fractal::coloredJuliaMatrix(int* matrix, int matrix_width, int matrix_height, ColorGenerator& cg)
{
	for (int index = 0; index < t_pool->size; ++index)
	{
		t_pool->addJob([=, &cg]() {coloredJuliaThread(index, matrix, matrix_width, matrix_height, t_pool->size, cg); });
	}
	t_pool->synchronize();
}

void Fractal::coloredJuliaThread(int index, int* matrix, int matrix_width, int matrix_height, int stride, ColorGenerator& cg)
{
	for (int i = index; i < (matrix_width * matrix_height); i += stride)
	{
		matrix[i] = cg.colorClamp(juliaSetAtPoint(i % matrix_width, i / matrix_width, matrix_width, matrix_height), julia_max_iter);
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
		return -1;
	else
		return iter;
}