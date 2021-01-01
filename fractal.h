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


////////////////////////////////////////////////////////////
/// Default fractal parameters
////////////////////////////////////////////////////////////

/* Mandelbrot Set */
constexpr long double mandelbrot_x_min_DEFAULT				= -2.5;
constexpr long double mandelbrot_x_max_DEFAULT				=  1.0;
constexpr long double mandelbrot_y_min_DEFAULT				= -1.0;
constexpr long double mandelbrot_y_max_DEFAULT				=  1.0;
constexpr long double mandelbrot_radius_DEFAULT				=  4.0;
constexpr long double mandelbrot_zoom_DEFAULT				=  1.0;
constexpr long double mandelbrot_zoom_multiplier_DEFAULT	=  0.1;
constexpr long double mandelbrot_x_offset_DEFAULT			=  0.0;
constexpr long double mandelbrot_y_offset_DEFAULT			=  0.0;
constexpr long double mandelbrot_pan_increment_DEFAULT		=  0.8;
constexpr int mandelbrot_max_iter_DEFAULT					=  200;
constexpr float mandelbrot_max_iter_multiplier_DEFAULT		=  1.5;

/////////////////////////////////////////////////////////////

struct thread_info;

class Fractal
{
	ThreadPool* t_pool;

	void mandelbrotThread(int index, int* matrix, int matrix_width, int matrix_height, int stride);
	bool mandelbrot_bulb_check(long double x_0, long double y_0);
	bool mandelbrot_cardioid_check(long double x_0, long double y_0);
	bool mandelbrot_prune(long double x_0, long double y_0);
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
	long double mandelbrot_zoom_multiplier;
	long double mandelbrot_x_offset;
	long double mandelbrot_y_offset;
	long double mandelbrot_pan_increment;
	int mandelbrot_max_iter;
	float mandelbrot_max_iter_multiplier;

	long double julia_n;
	int julia_max_iter;
	long double julia_escape_radius;
	std::complex<long double> julia_complex_param;

	Fractal();

	void stationaryZoom(int direction);
	void followingZoom(double zoom_amount, int x_pos, int y_pos, int max_x, int max_y);
	void panUp(long double delta);
	void panDown(long double delta);
	void panLeft(long double delta);
	void panRight(long double delta);
	void increaseIterations();
	void decreaseIterations();
	void reset();

	void mandelbrotMatrix(int* matrix, int matrix_width, int matrix_height);
	void juliaMatrix(int* matrix, int matrix_width, int matrix_height);

	void generate(int* matrix, int matrix_width, int matrix_height, ColorGenerator& cg);
};