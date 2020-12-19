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

	void coloredMandelbrotThread(int index, int* matrix, int matrix_width, int matrix_height, int stride, ColorGenerator& cg);
	int mandelbrotSetAtPoint(int x, int y, int max_x, int max_y);

	void coloredJuliaThread(int index, int* matrix, int matrix_width, int matrix_height, int stride, ColorGenerator& cg);
	int juliaSetAtPoint(int x, int y, int max_x, int max_y);

public:

	long double x_offset;
	long double y_offset;
	long double zoom_degree;

	long double mandelbrot_x_min;
	long double mandelbrot_x_max;
	long double mandelbrot_y_min;
	long double mandelbrot_y_max;
	int mandelbrot_max_iter;
	long double mandelbrot_radius;

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

	void coloredMandelbrotMatrix(int* matrix, int matrix_width, int matrix_height, ColorGenerator& cg);
	void coloredJuliaMatrix(int* matrix, int matrix_width, int matrix_height, ColorGenerator& cg);

};