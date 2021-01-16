/*
* Takes a matrix and generates fractal iteration values within it.
*/

#pragma once

#define PRINT_INFO

#include "color.h"
#include "thread_pool.h"

#include <cmath>
#include <complex>
#include <thread>
#include <vector>

#include <immintrin.h> // AVX intrinsics

// Golden ratio
#define PHI (1.0 + std::sqrt(5.0) / 2.0)

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

/* Julia Set */
constexpr long double julia_x_offset_DEFAULT				=  0.0;
constexpr long double julia_y_offset_DEFAULT				=  0.0;
constexpr long double julia_pan_increment_DEFAULT			=  0.8;
constexpr long double julia_zoom_DEFAULT					=  1.0;
constexpr long double julia_zoom_multiplier_DEFAULT			=  0.1;
constexpr long double  julia_n_DEFAULT						=  2;
constexpr int julia_max_iter_DEFAULT						=  200;
constexpr float julia_max_iter_multiplier_DEFAULT			=  1.5;
constexpr long double julia_escape_radius_DEFAULT			=  2;

// Some nice parameters for the julia set (taken from Wikipedia)
//static std::complex<long double> julia_complex_param_DEFAULT = std::complex<long double>(1.0 - PHI, 0.0); // (1 - phi, 0) where phi is the golden ratio
//static std::complex<long double> julia_complex_param_DEFAULT = std::complex<long double>(PHI - 2.0, PHI - 1.0); // (phi - 2, phi - 1)
//static std::complex<long double> julia_complex_param_DEFAULT = std::complex<long double>(0.285, 0.01);
//static std::complex<long double> julia_complex_param_DEFAULT = std::complex<long double>(-0.70176, -0.3842);
//static std::complex<long double> julia_complex_param_DEFAULT = std::complex<long double>(-0.835, -0.2321);
//static std::complex<long double> julia_complex_param_DEFAULT = std::complex<long double>(-0.8, 0.156);
//static std::complex<long double> julia_complex_param_DEFAULT = std::complex<long double>(-0.7269, 0.1889);
static std::complex<long double> julia_complex_param_DEFAULT = std::complex<long double>(0.0, -0.8);


/////////////////////////////////////////////////////////////

struct thread_info;

class Fractal
{
	ThreadPool* t_pool;

	void mandelbrotScale(long double& scaled_x, long double& scaled_y, int x, int y, int max_x, int max_y);
	bool mandelbrotBulbCheck(long double x_0, long double y_0);
	bool mandelbrotCardioidCheck(long double x_0, long double y_0);
	bool mandelbrotPrune(long double x_0, long double y_0);
	int mandelbrotSetAtPoint(int x, int y, int max_x, int max_y);
	void mandelbrotThread(int index, int* matrix, int matrix_width, int matrix_height, int stride);

	__m256d mandelbrotBulbCheckAVX(const __m256d& _x, const __m256d& _y);
	__m256d mandelbrotCardioidCheckAVX(const __m256d& _x, const __m256d& _y);
	bool mandelbrotPruneAVX(const __m256d& _x, const __m256d& _y);
	void mandelbrotAVXThread(int index, int* matrix, int matrix_width, int matrix_height, int stride);

	void juliaScale(long double& scaled_x, long double& scaled_y, int x, int y, int max_x, int max_y);
	void juliaThread(int index, int* matrix, int matrix_width, int matrix_height, int stride);
	int juliaSetAtPoint(int x, int y, int max_x, int max_y);

public:

	enum class FractalSets { MANDELBROT = 0, JULIA, LAST} fractal_mode;

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

	long double julia_x_offset;
	long double julia_y_offset;
	long double julia_pan_increment;
	long double julia_zoom;
	long double julia_zoom_multiplier;
	long double julia_n;
	int julia_max_iter;
	float julia_max_iter_multiplier;
	long double julia_escape_radius;
	std::complex<long double> julia_complex_param;

	Fractal();

	void stationaryZoom(int direction, int max_x, int max_y);
	void followingZoom(int direction, int x_pos, int y_pos, int max_x, int max_y);
	void panUp(long double delta);
	void panDown(long double delta);
	void panLeft(long double delta);
	void panRight(long double delta);
	void increaseIterations();
	void decreaseIterations();
	void reset();

	void mandelbrotMatrix(int* matrix, int matrix_width, int matrix_height);
	void mandelbrotMatrixAVX(int* matrix, int matrix_width, int matrix_height);
	void juliaMatrix(int* matrix, int matrix_width, int matrix_height);

	void switchFractal();
	void generate(int* matrix, int matrix_width, int matrix_height, ColorGenerator& cg, bool AVX);
};