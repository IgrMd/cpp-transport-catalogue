#include "transport_catalogue.h"

namespace transportCatalogue{
using namespace std::literals;

void Catalogue::AddStop(const std::string_view name, double latitude, double longitude) {
	AddStop(std::string{ name }, latitude, longitude);
}

void Catalogue::AddStop(std::string&& name, double latitude, double longitude) {
	auto [it, is_inserted] = stop_names_.insert(std::move( name ));
	assert(is_inserted);
	stops_.push_back({ *it, {latitude, longitude} });
	name_to_stop_[*it] = &stops_.back();
	stop_to_buses_[*it];
}

void Catalogue::AddStopDistances(std::string_view name_from,
	const std::unordered_map<std::string_view, int>& name_to_dist) {
	auto stop_from = name_to_stop_.find(name_from);
	assert(stop_from != name_to_stop_.end());
	for (const auto [name_to, distance] : name_to_dist) {
		auto stop_to = name_to_stop_.find(name_to);
		assert(stop_to != name_to_stop_.end());
		stop_pair_to_dist_[{stop_from->second, stop_to->second}] = distance;
	}
}

void Catalogue::CalculateLength(Bus& bus) {
	double length_geo = 0;
	int length_curv = 0;
	auto from = bus.stops.begin();
	auto to = std::next(from);
	while (to != bus.stops.end()) {
		length_geo += geo::ComputeDistance((*from)->coordinates, (*to)->coordinates);
		int distance_curv = 0;
		if (stop_pair_to_dist_.count({ *from, *to })) {
			distance_curv = stop_pair_to_dist_.at({ *from, *to });
		} else if (stop_pair_to_dist_.count({ *to, *from })) {
			distance_curv = stop_pair_to_dist_.at({ *to, *from });
		} else {
			assert(false);
		}
		length_curv += distance_curv;
		from = to;
		to = std::next(to);
	}
	if (!bus.is_circle) {
		size_t middle = bus.stops.size()/2;
		auto reverse = bus.stops[middle];
		if (stop_pair_to_dist_.count({ reverse, reverse })) {
			length_curv += stop_pair_to_dist_.at({ reverse, reverse });
		}
	}
	bus.length_geo = length_geo;
	bus.length_curv = length_curv;
}

detail::BusInfo Catalogue::GetBusInfo(const std::string_view name) const {
	const auto result = name_to_bus_.find(name);
	if (result == name_to_bus_.end()) {
		return { name };
	}
	size_t stops_count = result->second->stops.size();
	size_t unique_stops_count = result->second->unique_stop_names_.size();
	double length_geo = result->second->length_geo;
	int length_curv = result->second->length_curv;

	return { result->first, true, stops_count, unique_stops_count, length_geo , length_curv };
}

detail::StopInfo Catalogue::GetStopInfo(const std::string_view name) const {
	const auto result = stop_to_buses_.find(name);
	if (result == stop_to_buses_.end()) {
		static const std::set<std::string_view> empty_result;
		return { name, false, empty_result};
	}
	return { result->first, true, result->second };
}

bool Catalogue::StopPair::operator==(const StopPair& other) const {
	return this->from == other.from && this->to == other.to;
}

size_t Catalogue::StopPairHasher::operator()(const StopPair& stop_pair) const {
	return std::hash<std::string_view>{}(stop_pair.from->name) + std::hash<std::string_view>{}(stop_pair.to->name)*43;
}

namespace detail{

BusInfo::BusInfo(const std::string_view name)
	: name(name) {}
BusInfo::BusInfo(const std::string_view name, bool is_found, size_t stops_count,
	size_t unique_stops_count, double length_geo, int length_curv)
	: name(name),
	is_found(is_found),
	stops_count(stops_count),
	unique_stops_count(unique_stops_count),
	length_geo(length_geo),
	length_curv(length_curv) {}

StopInfo::StopInfo(const std::string_view name, bool is_found, const std::set<std::string_view>& buses)
	: name(name), is_found(is_found), buses(buses) {}

} //end namespace detail

} //end namespace transportCatalogue