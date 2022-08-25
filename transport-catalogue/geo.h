#pragma once

#define _USE_MATH_DEFINES
#include <cmath>

namespace transport_catalogue {

namespace geo {

static const double TRESHOLD = 1e-6;
static const int ERTH_RADIUS = 6371000;

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

double ComputeDistance(Coordinates from, Coordinates to);

} //end namespace geo

} //end namespace transport_catalogue