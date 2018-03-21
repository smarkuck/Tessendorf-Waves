#include "ocean.h"

Ocean::Ocean(double lx, double ly, int nx, int ny, double wind_speed, double min_wave_size, double A) :
	lx(lx), ly(ly), nx(nx), ny(ny), wind_speed(wind_speed), min_wave_size(min_wave_size), A(A) {

	h0 = new complex*[ny]; //prepare 2D array to storage Phillips spectrum data
	h = new complex*[ny]; //function h(k,t) data
	H = new complex*[nx]; //and real height data

	for (int i = 0; i < ny; i++) {
		h0[i] = new complex[nx];
		h[i] = new complex[nx];
	}

	for (int i = 0; i < nx; i++) {
		H[i] = new complex[ny];
	}

	phillipsSpectrum(); //calculate Phillips spectrum
}

void Ocean::phillipsSpectrum() {

	//calculate Phillips spectrum for every point nx, ny

	std::default_random_engine generator;
	generator.seed(time(0));
	std::normal_distribution<double> distribution(0.0, 1.0); //normal distribution

	double g = 9.81; //gravitational acceleration
	double L_sq = pow((wind_speed*wind_speed) / g, 2); //L^2

	// Ph(k) = A * exp(-1 / (kL)^2) * |^k|^2 / k^4

	// h0(k) = 1/sqrt(2) * (dist + i*dist) * sqrt(Ph(k))

	for (int i = 0; i < ny; i++) {
		for (int j = 0; j < nx; j++) {

			double kx = (2 * M_PI*(j-nx/2)) / lx; //factor x of vector k (wave direction)
			double ky = (2 * M_PI*(i-ny/2)) / ly; //factor y of vector k (wave direction)
			double k_sq = kx*kx + ky*ky; //k^2

			if (k_sq == 0) {
				h0[i][j] = 0;
			}
			else {
				double P;
				P = A*exp((-1 / (k_sq*L_sq)));
				P *= exp(-k_sq*pow(min_wave_size, 2)); //remove waves smaller than min_wave_size
				P *= pow((kx*kx) / k_sq, 2);
				P /= k_sq*k_sq;
				P /= 2.;
				P = sqrt(P);
				h0[i][j] = complex(P * distribution(generator), P*distribution(generator));
			}
		}
	}
}

void Ocean::compute_h(double t) {

	//calculate h(k,t) function for time t

	for (int i = 0; i < ny; i++) {
		for (int j = 0; j < nx; j++) {
			double   A; //waves frequency
			double   L = 0.1; //surface tension
			double k_sq = pow((2 * M_PI*j) / lx, 2) + pow((2 * M_PI*i) / ly, 2); //k^2, k - wave direction

			// A = gk(1 + k^2 * L^2) - wave frequency
			A = t*sqrt(9.81*sqrt(k_sq) * (1 + k_sq*pow(L, 2)));

			// h(k,t) = h0(k) * exp(iAt) + h0*(-k) * exp(-iAt)
			h[i][j] = h0[i][j] * (cos(A) + complex::i*sin(A)) + h0[ny - i - 1][nx - j - 1] * (cos(-A) + complex::i*sin(-A));
		}
	}
}

void Ocean::compute_H() {

	//calculate FFT for h(k,t) function

	for (int i = 0; i < ny; i++) {
		CFFT::Forward(h[i], nx);
	}

	for (int i = 0; i < nx; i++) {
		for (int j = 0; j < ny; j++) {
			H[i][j] = h[j][i];
		}
		CFFT::Forward(H[i], ny);
	}
}

float* Ocean::generateMesh(int *size) {

	//generate Ocean mesh as ny TRIANGLE_STRIPs

	float *mesh = new float[3 * 2 * (nx+1) * ny];

	for (int i = 0; i < ny; i++) {
		for (int j = 0; j < nx+1; j++) {
			int pos = (i*(nx+1) + j) * 6;

			mesh[pos] = (lx / nx)*j;
			mesh[pos + 1] = 0;
			mesh[pos + 2] = (ly / ny)*i;

			mesh[pos + 3] = (lx / nx)*j;
			mesh[pos + 4] = 0;
			mesh[pos + 5] = (ly / ny)*(i+1);
		}
	}

	*size = 2 * (nx+1) * ny;
	return mesh;
}

float* Ocean::generateNorm(float* mesh) {

	//generate normal vectors for ocean Mesh

	int size = 2 * (nx+1) * ny;
	float* norm = new float[3 * size];
	int triangles = nx*2;

	for (int i = 0; i < ny; i++) {
		for (int j = 0; j < triangles; j++) {

			int pos = ((triangles + 2)*i + j) * 3;
			//for last strip copy normal vector of first edge of first strip
			//it allows for continuity if we create more than 1 tile

			if ((i == ny - 1) && (j % 2)) {
				norm[pos] = norm[(j - 1) * 3];
				norm[pos + 1] = norm[(j - 1) * 3 + 1];
				norm[pos + 2] = norm[(j - 1) * 3 + 2];

				//for two last vertices of strip copy normal vector of two first vertices to keep continuity

				if (j == triangles - 1) {
					int pos2 = pos - (triangles - 1)*3;
					norm[pos + 3] = norm[pos2];
					norm[pos + 4] = norm[pos2+1];
					norm[pos + 5] = norm[pos2+2];

					norm[pos + 6] = norm[pos2+3];
					norm[pos + 7] = norm[pos2 + 4];
					norm[pos + 8] = norm[pos2 + 5];
				}
				continue;
			}
			//if strip is not first, normal vector of first edge is the same as previous strip
			//we need to copy it
			if (!(j % 2) && i > 0) {
				norm[pos] = norm[pos - (triangles + 1) * 3];
				norm[pos + 1] = norm[pos - (triangles + 1) * 3 + 1];
				norm[pos + 2] = norm[pos - (triangles + 1) * 3 + 2];
				continue;
			}
			
			//get xyz coordinates of triangle vertices
			glm::vec3 v1 = glm::vec3(mesh[pos], mesh[pos + 1], mesh[pos + 2]);
			glm::vec3 v2 = glm::vec3(mesh[pos + 3], mesh[pos + 4], mesh[pos + 5]);
			glm::vec3 v3 = glm::vec3(mesh[pos + 6], mesh[pos + 7], mesh[pos + 8]);

			//calculate 2 edges of triangle
			glm::vec3 e1 = v2 - v1;
			glm::vec3 e2 = v3 - v1;

			//calculate normal vector from this edges
			glm::vec3 normal = glm::normalize(glm::cross(e1, e2));
			//every second triangle has normal vector directed down, we need to revert it
			if (normal.y < 0) normal = -normal;

			norm[pos] = normal.x;
			norm[pos + 1] = normal.y;
			norm[pos + 2] = normal.z;

			//for two last vertices of strip copy normal vector of two first vertices to keep continuity
			if (j == triangles - 1) {
				int pos2 = pos - (triangles - 1) * 3;
				norm[pos + 3] = norm[pos2];
				norm[pos + 4] = norm[pos2 + 1];
				norm[pos + 5] = norm[pos2 + 2];

				norm[pos + 6] = norm[pos2 + 3];
				norm[pos + 7] = norm[pos2 + 4];
				norm[pos + 8] = norm[pos2 + 5];
			}
		}
	}
	return norm;
}

void Ocean::setMeshHeight(float *mesh, double t) {
	compute_h(t);
	compute_H();

	for (int i = 0; i < ny; i++) {
		for (int j = 0; j < nx+1; j++) {
			int pos = (i*(nx+1) + j) * 6;

			//if we set height for nx, ny edge or nx,ny vertex
			//we need to copy height from opposite edge to keep continuity of tiles


			//because of H calculation every second value is negative, we need to revert it
			if (j == nx && i == ny - 1)
			{
				mesh[pos + 1] = H[0][i].re()*((i + j) % 2 ? 1 : -1);
				mesh[pos + 4] = H[0][0].re()*((i + j + 1) % 2 ? 1 : -1);
				continue;
			}

			if (j == nx) {
				mesh[pos + 1] = H[0][i].re()*((i + j) % 2 ? 1 : -1);
				mesh[pos + 4] = H[0][i + 1].re()*((i + j + 1) % 2 ? 1 : -1);
				continue;
			}

			if (i == ny - 1) {
				mesh[pos + 1] = H[j][i].re()*((i + j) % 2 ? 1 : -1);
				mesh[pos + 4] = H[j][0].re()*((i + j + 1) % 2 ? 1 : -1);
				continue;
			}

			mesh[pos + 1] = H[j][i].re()*((i + j) % 2 ? 1 : -1);
			mesh[pos + 4] = H[j][i + 1].re()*((i + j + 1) % 2 ? 1 : -1);
		}
	}
}

Ocean::~Ocean() {
	for (int i = 0; i < ny; i++) {
		delete[] h0[i];
		delete[] h[i];
	}

	for (int i = 0; i < nx; i++) {
		delete[] H[i];
	}

	delete[] h0;
	delete[] h;
	delete[] H;
}