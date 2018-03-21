#pragma once

#define _USE_MATH_DEFINES
#include <cmath>
#include <random>
#include <ctime>
#include <complex>

#include "glm/vec3.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include "FFT_CODE\complex.h"
#include "FFT_CODE\fft.h"

class Ocean {
public:

	Ocean(double lx, double ly, int nx, int ny, double wind_speed, double min_wave_size, double A);
	float* generateMesh(int *size); //generate Ocean mesh without height, returns number of generated vertices in size var
	float* generateNorm(float *mesh); //generate normals for Ocean mesh

	void setMeshHeight(float *mesh, double t); //set mesh height in particular time
	~Ocean();

private:

	void phillipsSpectrum(); //calculate Phillips spectrum and save it in h0
	void compute_h(double t); //calculate values of h(k,t) function and save it in h
	void compute_H(); //calculate wave heights with h(k,t) function and FFT and save it in H

	complex **h0, //Phillps spectrum data
			**h, //h(k,t) function values data used in FFT
			**H; //wave heights data

	const double lx; //real ocean width
	const double ly; //real ocean lenght
	const int    nx; //ocean samples for width, must be power of 2
	const int    ny; //ocean samples for length, must be power of 2

	const double wind_speed;
	const double min_wave_size;
	const double A; //constant to regulate wave height
};