
#include "Filter.h"

namespace roadcheck {
	Filter::Filter(float *b, int length) {
		this->b = b;
		this->length = length;
		this->d = new float[length];
		for (int i = 0; i < length; i++) {
			this->d[i] = 0;
		}
		this->i = 0;
	}

	Filter::~Filter() {
		delete [] d;
	}

	float Filter::filtering(float input) {
		for (int i = 0; i < length - 1; i++) {
			d[length - 1 - i] = d[length - 2 - i];
		}
		d[0] = input;

		float out = 0;
		for (int i = 0; i < length; i++) {
			out += d[i] * b[i];
		}
		return out;
	}
}
