#pragma once

#include "domain.h"
#include "geo.h"

#include <algorithm>
#include <cassert>
#include <deque>
#include <functional>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>

namespace transport_catalogue {

class TransportCatalogue {

using Bus = domain::Bus;
using Stop = domain::Stop;
using StopPair = domain::StopPair;
using StopPairHasher = domain::StopPairHasher;

public:
	TransportCatalogue() = default;

	template<typename StringType>
	void AddBus(const std::string_view name, const std::vector<StringType>& stop,
		const bool is_roundtrip);
	template<typename StringType>
	void AddBus(std::string&& name, const std::vector<StringType>& stops,
		const bool is_roundtrip);

	void AddStop(const std::string_view name, geo::Coordinates coordinates);
	void AddStop(std::string&& name, geo::Coordinates coordinates);

	//Добавляет информацию о расстояниях между остановками
	void SetStopDistances(std::string_view name,
		const std::unordered_map<std::string_view, int>& name_to_dist);

	std::optional<domain::BusStat> GetBusStat(const std::string_view name) const;

	std::optional<domain::StopStat> GetStopStat(const std::string_view name) const;

	const std::deque<Bus>& GetBuses() const;

	std::vector<const Stop*> GetStopsUsed() const;


private:

	std::pair<double, int> CalculateLength(const Bus& bus) const;

	//Контейнер автобусов (маршрутов)
	std::deque <Bus> buses_;

	//Контейнер для быстрого доступа к автобусам (маршрутам) по имени
	std::unordered_map<std::string_view, const Bus*> name_to_bus_;

	//Контейнер остановок
	std::deque <Stop> stops_;

	//Контейнер для быстрого доступа к остановкам по имени
	std::unordered_map<std::string_view, const Stop*> name_to_stop_;

	//Контенер для хранения информации об автобусах, проходящих через остановку
	std::unordered_map<std::string_view, std::set<std::string_view>> stop_to_buses_;

	std::unordered_map<StopPair, int, StopPairHasher> stop_pair_to_dist_;
};

template<typename StringType>
void TransportCatalogue::AddBus(const std::string_view name,
	const std::vector<StringType>& stops, const bool is_roundtrip) {
	AddBus(std::string{ name }, stops, is_roundtrip);
}
template<typename StringType>
void TransportCatalogue::AddBus(std::string&& name,
	const std::vector<StringType>& stops, const bool is_roundtrip) {
	Bus bus;
	bus.name = std::move(name);
	bus.is_roundtrip = is_roundtrip;
	bus.stops.reserve(stops.size() + stops.size() * (!is_roundtrip));
	std::unordered_set<std::string_view> unique_stops;
	for (const auto& stop : stops) {
		assert(name_to_stop_.count(stop));
		bus.stops.push_back(name_to_stop_.at(stop));
		unique_stops.insert(stop);
	}
	bus.unique_stops_count = unique_stops.size();
	if (!is_roundtrip) {
		bus.stops.resize(stops.size() * 2 - 1);
		std::reverse_copy(bus.stops.begin(),
			std::next(bus.stops.begin(), stops.size() - 1),
			std::next(bus.stops.begin(), stops.size())
		);
	}
	buses_.push_back(std::move(bus));
	name_to_bus_[buses_.back().name] = &buses_.back();
	for (const auto& stop : stops) {
		stop_to_buses_.at(stop).insert(buses_.back().name);
	}
}

} //end namespace transport_catalogue