/**
 * Utilities for RFC-sim model
 * author@Qiran Qian, qiranq@kth.se
 **/

#pragma once
#include <iostream>

namespace utils {

	// Dim3 for specifying 3-dimentional TB in GPU, 
	// But can be used more generally
	template<typename T>
	struct Dim3 {
		Dim3() {}
		Dim3(T x, T y, T z) : x(x), y(y), z(z) {}

		// Values
		T x;
		T y;
		T z;
	};

	// Pretty Print
	template <typename T>
	std::ostream & operator<<(std::ostream & os, const Dim3<T> dim3) {
			std::cout << "(" << dim3.x << ", " << dim3.y << ", " << dim3.z << ")";
			return os;	
	}
};

