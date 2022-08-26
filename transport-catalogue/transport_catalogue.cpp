#include "transport_catalogue.h"

namespace transport_catalogue {
using namespace std::literals;

void TransportCatalogue::AddStop(const std::string_view name, geo::Coordinates coordinates) {
	AddStop(std::string{ name }, coordinates);
}

void TransportCatalogue::AddStop(std::string&& name, geo::Coordinates coordinates) {
	Stop stop{ std::move(name), coordinates };
	assert(!name_to_stop_.count(stop.name));
	stops_.push_back(std::move(stop));
	name_to_stop_[stops_.back().name] = &stops_.back();
	stop_to_buses_[stops_.back().name];
}

void TransportCatalogue::SetStopDistances(std::string_view name_from,
	const std::unordered_map<std::string_view, int>& name_to_dist) {
	auto stop_from = name_to_stop_.find(name_from);
	assert(stop_from != name_to_stop_.end());
	for (const auto& [name_to, distance] : name_to_dist) {
		auto stop_to = name_to_stop_.find(name_to);
		assert(stop_to != name_to_stop_.end());
		stop_pair_to_dist_[{stop_from->second, stop_to->second}] = distance;
	}
}

std::pair<double, int> TransportCatalogue::CalculateLength(const Bus& bus) const {
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
	if (!bus.is_roundtrip) {
		size_t middle = bus.stops.size() / 2;
		auto reverse = bus.stops[middle];
		if (stop_pair_to_dist_.count({ reverse, reverse })) {
			length_curv += stop_pair_to_dist_.at({ reverse, reverse });
		}
	}
	return { length_geo, length_curv };
}

std::optional<domain::BusStat> TransportCatalogue::GetBusStat(const std::string_view name) const {
	const auto result = name_to_bus_.find(name);
	if (result == name_to_bus_.end()) {
		return std::optional<domain::BusStat>();
	}
	size_t stops_count = result->second->stops.size();
	size_t unique_stops_count = result->second->unique_stops_count;
	const auto [length_geo, length_curv] = CalculateLength(*(result->second));
	return std::optional<domain::BusStat>(
		{ result->first, stops_count, unique_stops_count, length_geo, length_curv });
}

std::optional<domain::StopStat> TransportCatalogue::GetStopStat(const std::string_view name) const {
	const auto result = stop_to_buses_.find(name);
	if (result == stop_to_buses_.end()) {
		static const std::set<std::string_view> empty_result;
		return std::optional<domain::StopStat>();
	}
	return std::optional<domain::StopStat>({ result->first, result->second });
}

const std::deque <domain::Bus>& TransportCatalogue::GetBuses() const{
	return buses_;
}

std::vector<const domain::Stop*> TransportCatalogue::GetStopsUsed() const {
	std::vector<const domain::Stop*> stops;
	stops.reserve(stop_to_buses_.size());
	for (const auto& [stop, buses] : stop_to_buses_) {
		if (!buses.empty()) {
			stops.push_back(name_to_stop_.at(stop));
		}
	}
	return stops;
}

namespace detail {

} //end namespace detail

} //end namespace transport_catalogue