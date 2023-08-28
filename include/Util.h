#pragma once

#include <iostream>

namespace util {
	
	template<typename T>
	struct Dim3 {
		Dim3() {}
		Dim3(T x, T y, T z) : x(x), y(y), z(z) {}

		T x;
		T y;
		T z;
	};

	template <typename T>
	std::ostream & operator<<(std::ostream & os, const Dim3<T> dim3) {
			std::cout << "(" << dim3.x << ", " << dim3.y << ", " << dim3.z << ")";
			return os;	
	}
};

