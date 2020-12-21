/*
* Takes a matrix and generates fractal iteration values within it.
*/

#pragma once

#include "color.h"
#include "thread_pool.h"

#include <cmath>
#include <complex>
#include <thread>
#include <vector>


struct thread_info;

class Fractal
{
	ThreadPool* t_pool;

	void mandelbrotThread(int index, int* matrix, int matrix_width, int matrix_height, int stride);
	int mandelbrotSetAtPoint(int x, int y, int max_x, int max_y);

	void juliaThread(int index, int* matrix, int matrix_width, int matrix_height, int stride);
	int juliaSetAtPoint(int x, int y, int max_x, int max_y);

public:

	enum class FractalSets { MANDELBROT, JULIA } mode;

	long double x_offset;
	long double y_offset;
	long double zoom_degree;

	long double mandelbrot_x_min;
	long double mandelbrot_x_max;
	long double mandelbrot_y_min;
	long double mandelbrot_y_max;
	long double mandelbrot_radius;
	long double mandelbrot_zoom;
	long double mandelbrot_zoom_increment;
	long double mandelbrot_pan_increment;
	int mandelbrot_max_iter;

	long double julia_n;
	int julia_max_iter;
	long double julia_escape_radius;
	std::complex<long double> julia_complex_param;

	Fractal();

	void zoom(double zoom_amount);
	void panUp();
	void panDown();
	void panLeft();
	void panRight();

	void mandelbrotMatrix(int* matrix, int matrix_width, int matrix_height);
	void juliaMatrix(int* matrix, int matrix_width, int matrix_height);

	void generate(int* matrix, int matrix_width, int matrix_height, ColorGenerator& cg);
};