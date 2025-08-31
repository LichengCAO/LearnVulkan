#include "gaussian_blur.h"
#define _USE_MATH_DEFINES
#include <math.h>
#include <cmath>
std::vector<float> Get1DGaussian(int radius, float sigma)
{
	int ksize = 2 * radius + 1;
	int offset = radius;
	std::vector<float> kernel(ksize);
	std::vector<float> ret(radius + 1);

	float sum = 0.0;

	for (int i = -radius; i <= radius; ++i) {
		double value = std::exp(-(i * i) / (2 * sigma * sigma)) / (std::sqrt(2 * M_PI) * sigma);
		kernel[i + radius] = value;
		sum += value;
	}

	// Normalize the kernel
	for (float& value : kernel) {
		value /= sum;
	}

	for (int i = 0; i < ret.size(); ++i)
	{
		ret[i] = kernel[i + offset];
	}

	return ret;
}
