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
constexpr long double mandelbrot_pan_increment_DEFAULT		=  0.08;
constexpr int mandelbrot_max_iter_DEFAULT					=  200;
constexpr float mandelbrot_max_iter_multiplier_DEFAULT		=  1.5;

/* Julia Set */
constexpr long double julia_x_offset_DEFAULT				=  0.0;
constexpr long double julia_y_offset_DEFAULT				=  0.0;
constexpr long double julia_pan_increment_DEFAULT			=  0.08;
constexpr long double julia_zoom_DEFAULT					=  1.0;
constexpr long double julia_zoom_multiplier_DEFAULT			=  0.1;
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

/* Burning ship */
constexpr long double bship_x_offset_DEFAULT				= 0.0;
constexpr long double bship_y_offset_DEFAULT				= 0.0;
constexpr long double bship_pan_increment_DEFAULT			= 0.08;
constexpr long double bship_zoom_DEFAULT					= 1.0;
constexpr long double bship_zoom_multiplier_DEFAULT			= 0.1;
constexpr int bship_max_iter_DEFAULT						= 200;
constexpr float bship_max_iter_multiplier_DEFAULT			= 1.5;
constexpr long double bship_radius_DEFAULT					= 2;


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
	void juliaAVXThread(int index, int* matrix, int matrix_width, int matrix_height, int stride);

	void bshipScale(long double& scaled_x, long double& scaled_y, int x, int y, int max_x, int max_y);
	void bshipThread(int index, int* matrix, int matrix_width, int matrix_height, int stride);
	int bshipAtPoint(int x, int y, int max_x, int max_y);
	void bshipAVXThread(int index, int* matrix, int matrix_width, int matrix_height, int stride);

public:

	enum class FractalSets { MANDELBROT = 0, JULIA, BSHIP, LAST} fractal_mode;

	// Mandelbrot
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
	unsigned int mandelbrot_max_iter;
	float mandelbrot_max_iter_multiplier;

	// Julia
	long double julia_x_offset;
	long double julia_y_offset;
	long double julia_pan_increment;
	long double julia_zoom;
	long double julia_zoom_multiplier;
	unsigned int julia_max_iter;
	float julia_max_iter_multiplier;
	long double julia_radius;
	std::complex<long double> julia_complex_param;

	// Burning ship
	long double bship_x_offset;
	long double bship_y_offset;
	long double bship_pan_increment;
	long double bship_zoom;
	long double bship_zoom_multiplier;
	unsigned int bship_max_iter;
	float bship_max_iter_multiplier;
	long double bship_radius;


	Fractal();

	void stationaryZoom(int direction, int max_x, int max_y);
	void followingZoom(int direction, int x_pos, int y_pos, int max_x, int max_y);
	void panUp();
	void panDown();
	void panLeft();
	void panRight();
	void increaseIterations();
	void decreaseIterations();
	void reset();

	void mandelbrotMatrix(int* matrix, int matrix_width, int matrix_height);
	void mandelbrotMatrixAVX(int* matrix, int matrix_width, int matrix_height);
	void juliaMatrix(int* matrix, int matrix_width, int matrix_height);
	void juliaMatrixAVX(int* matrix, int matrix_width, int matrix_height);
	void bshipMatrix(int* matrix, int matrix_width, int matrix_height);
	void bshipMatrixAVX(int* matrix, int matrix_width, int matrix_height);

	void selectNextFractal();
	void selectFractal(int fractal);
	void generate(int* matrix, int matrix_width, int matrix_height, ColorGenerator& cg, bool AVX);
};