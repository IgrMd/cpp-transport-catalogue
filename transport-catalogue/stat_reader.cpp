#include"stat_reader.h"

namespace transportCatalogue {

namespace io {
using namespace std::literals;

void PrintBusInfo(const detail::BusInfo& bus, std::ostream& out) {
	if (!bus.is_found) {
		out << "Bus "s << bus.name << ": not found"s << std::endl;
	} else {
		out << std::setprecision(6)
			<< "Bus "s << bus.name << ": "s
			<< bus.stops_count << " stops on route, "s
			<< bus.unique_stops_count << " unique stops, "s
			<< bus.length_curv << " route length, "s
			<< bus.length_curv/bus.length_geo <<" curvature"s
			<< std::endl;
	}
}

void PrintStopInfo(const detail::StopInfo& stop, std::ostream& out) {
	if (!stop.is_found) {
		out << "Stop "s << stop.name << ": not found"s << std::endl;
	} else if (stop.buses.size() == 0) {
		out << "Stop "s << stop.name << ": no buses"s << std::endl;
	} else {
		out << "Stop "s << stop.name << ": buses"s;
		for (std::string_view bus : stop.buses) {
			out << ' ' << bus;
		}
		out << std::endl;
	}
}

} //end namespace io

} //end namespace transportCatalogue