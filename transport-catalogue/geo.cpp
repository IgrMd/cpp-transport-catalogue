#include "geo.h"

namespace transport_catalogue {

namespace geo {
using namespace std;

double ComputeDistance(Coordinates from, Coordinates to) {
	if (from == to) {
		return 0;
	}
	static const double dr = M_PI / 180.;
	return acos(sin(from.lat * dr) * sin(to.lat * dr)
		+ cos(from.lat * dr) * cos(to.lat * dr) * cos(abs(from.lng - to.lng) * dr))
		* ERTH_RADIUS;
}

} //end namespace geo

} //end namespace transport_catalogue // namespace geo