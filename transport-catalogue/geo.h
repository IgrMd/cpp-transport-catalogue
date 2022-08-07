#pragma once

#define _USE_MATH_DEFINES
#include <cmath>

namespace transport_catalogue {

namespace geo {

constexpr double TRESHOLD = 1e-6;
constexpr int ERTH_RADIUS = 6371000;

struct Coordinates {
	double lat;
	double lng;
	bool operator==(const Coordinates& other) const {
		return std::abs(lat - other.lat) < TRESHOLD && std::abs(lng - other.lng) < TRESHOLD;
	}
	bool operator!=(const Coordinates& other) const {
		return !(*this == other);
	}
};

inline double ComputeDistance(Coordinates from, Coordinates to) {
	using namespace std;
	if (from == to) {
		return 0;
	}
	static const double dr = M_PI / 180.;
	return acos(sin(from.lat * dr) * sin(to.lat * dr)
				+ cos(from.lat * dr) * cos(to.lat * dr) * cos(abs(from.lng - to.lng) * dr))
		* ERTH_RADIUS;
}

} //end namespace geo

} //end namespace transport_catalogue