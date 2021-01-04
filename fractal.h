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

// Golden ratio
#define PHI (1.0 + std::sqrt(5.0) / 2.0)

////////////////////////////////////////////////////////////
/// Default fractal parameters
////////////////////////////////////////////////////////////

/* Mandelbrot Set */
constexpr double mandelbrot_x_min_DEFAULT				= -2.5;
constexpr double mandelbrot_x_max_DEFAULT				=  1.0;
constexpr double mandelbrot_y_min_DEFAULT				= -1.0;
constexpr double mandelbrot_y_max_DEFAULT				=  1.0;
constexpr double mandelbrot_radius_DEFAULT				=  4.0;
constexpr double mandelbrot_zoom_DEFAULT				=  1.0;
constexpr double mandelbrot_zoom_multiplier_DEFAULT	=  0.1;
constexpr double mandelbrot_x_offset_DEFAULT			=  0.0;
constexpr double mandelbrot_y_offset_DEFAULT			=  0.0;
constexpr double mandelbrot_pan_increment_DEFAULT		=  0.8;
constexpr int mandelbrot_max_iter_DEFAULT					=  200;
constexpr float mandelbrot_max_iter_multiplier_DEFAULT		=  1.5;

/* Julia Set */
constexpr double julia_x_offset_DEFAULT				=  0.0;
constexpr double julia_y_offset_DEFAULT				=  0.0;
constexpr double julia_pan_increment_DEFAULT			=  0.8;
constexpr double julia_zoom_DEFAULT					=  1.0;
constexpr double julia_zoom_multiplier_DEFAULT			=  0.1;
constexpr double  julia_n_DEFAULT						=  2;
constexpr int julia_max_iter_DEFAULT						=  200;
constexpr float julia_max_iter_multiplier_DEFAULT			=  1.5;
constexpr double julia_escape_radius_DEFAULT			=  2;

// Some nice parameters for the julia set (taken from Wikipedia)
//static std::complex<double> julia_complex_param_DEFAULT = std::complex<double>(1.0 - PHI, 0.0); // (1 - phi, 0) where phi is the golden ratio
//static std::complex<double> julia_complex_param_DEFAULT = std::complex<double>(PHI - 2.0, PHI - 1.0); // (phi - 2, phi - 1)
//static std::complex<double> julia_complex_param_DEFAULT = std::complex<double>(0.285, 0.01);
//static std::complex<double> julia_complex_param_DEFAULT = std::complex<double>(-0.70176, -0.3842);
//static std::complex<double> julia_complex_param_DEFAULT = std::complex<double>(-0.835, -0.2321);
//static std::complex<double> julia_complex_param_DEFAULT = std::complex<double>(-0.8, 0.156);
//static std::complex<double> julia_complex_param_DEFAULT = std::complex<double>(-0.7269, 0.1889);
static std::complex<double> julia_complex_param_DEFAULT = std::complex<double>(0.0, -0.8);


/////////////////////////////////////////////////////////////

struct thread_info;

class Fractal
{
	ThreadPool* t_pool;

	void mandelbrotScale(double& scaled_x, double& scaled_y, int x, int y, int max_x, int max_y);
	void mandelbrotAVXThread(int index, int* matrix, int matrix_width, int matrix_height, int stride);
	void mandelbrotThread(int index, int* matrix, int matrix_width, int matrix_height, int stride);
	bool mandelbrot_bulb_check(double x_0, double y_0);
	bool mandelbrot_cardioid_check(double x_0, double y_0);
	bool mandelbrot_prune(double x_0, double y_0);
	int mandelbrotSetAtPoint(int x, int y, int max_x, int max_y);

	void juliaScale(double& scaled_x, double& scaled_y, int x, int y, int max_x, int max_y);
	void juliaThread(int index, int* matrix, int matrix_width, int matrix_height, int stride);
	int juliaSetAtPoint(int x, int y, int max_x, int max_y);

public:

	enum class FractalSets { MANDELBROT = 0, JULIA, LAST} fractal_mode;
	enum class InstructionModes {STANDARD = 0, AVX, LAST} instruction_mode;

	double mandelbrot_x_min;
	double mandelbrot_x_max;
	double mandelbrot_y_min;
	double mandelbrot_y_max;
	double mandelbrot_radius;
	double mandelbrot_zoom;
	double mandelbrot_zoom_multiplier;
	double mandelbrot_x_offset;
	double mandelbrot_y_offset;
	double mandelbrot_pan_increment;
	int mandelbrot_max_iter;
	float mandelbrot_max_iter_multiplier;

	double julia_x_offset;
	double julia_y_offset;
	double julia_pan_increment;
	double julia_zoom;
	double julia_zoom_multiplier;
	double julia_n;
	int julia_max_iter;
	float julia_max_iter_multiplier;
	double julia_escape_radius;
	std::complex<double> julia_complex_param;

	Fractal();

	void stationaryZoom(int direction, int max_x, int max_y);
	void followingZoom(int direction, int x_pos, int y_pos, int max_x, int max_y);
	void panUp(double delta);
	void panDown(double delta);
	void panLeft(double delta);
	void panRight(double delta);
	void increaseIterations();
	void decreaseIterations();
	void reset();

	void mandelbrotMatrix(int* matrix, int matrix_width, int matrix_height);
	void juliaMatrix(int* matrix, int matrix_width, int matrix_height);

	void switchFractal();
	void switchInstruction();
	void generate(int* matrix, int matrix_width, int matrix_height, ColorGenerator& cg);
};