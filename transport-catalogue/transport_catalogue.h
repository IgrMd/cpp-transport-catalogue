#pragma once

#include "geo.h"

#include <algorithm>
#include <cassert>
#include <deque>
#include <functional>
#include <set>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>

namespace transportCatalogue {

namespace detail{
struct BusInfo {
	BusInfo(const std::string_view name);
	BusInfo(const std::string_view name, bool is_found, size_t stops_count,
		size_t unique_stops_count, double length_geo, int length_curv);
	const std::string_view name;
	bool is_found = false;
	size_t stops_count = 0;
	size_t unique_stops_count = 0;
	double length_geo = 0;
	int length_curv = 0;
};

struct StopInfo {
	StopInfo(const std::string_view name, bool is_found, const std::set<std::string_view>& buses);
	const std::string_view name;
	const bool is_found = false;
	const std::set<std::string_view>& buses;
};

} //end namespace detail

class Catalogue {
public:
	Catalogue() = default;

	template<typename StringType>
	void AddBus(const std::string_view name, const std::vector<StringType>& stop, const bool is_circles);
	template<typename StringType>
	void AddBus(std::string&& name, const std::vector<StringType>& stops, const bool is_circle);

	void AddStop(const std::string_view name, double latitude, double longitude);
	void AddStop(std::string&& name, double latitude, double longitude);

	//Добавляет информацию о расстояниях между остановками
	void AddStopDistances(std::string_view name, const std::unordered_map<std::string_view, int>& name_to_dist);

	detail::BusInfo GetBusInfo(const std::string_view name) const;
	
	detail::StopInfo GetStopInfo(const std::string_view name) const;

private:
	//Остановка
	struct Stop {
		std::string_view name;
		geo::Coordinates coordinates;
	};
	//Автобус (маршрут)
	struct Bus {
		std::string_view name;
		std::vector<const Stop*> stops;
		std::unordered_set<std::string_view> unique_stop_names_;
		bool is_circle = false;
		double length_geo = 0;
		int length_curv = 0;
	};
	//Пара остановок для использования в ассоциативном контейнере
	struct StopPair {
		const Stop* from;
		const Stop* to;
		bool operator==(const StopPair& other) const;
	};
	//Хэшер для пары остановок
	struct StopPairHasher {
		size_t operator()(const StopPair& stop_pair) const;
	};
	//Контейнер названий автобусов (маршрутов)
	std::unordered_set<std::string> bus_names_;
	//Контейнер названий остановок
	std::unordered_set<std::string> stop_names_;
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

	void CalculateLength(Bus& bus);
};

template<typename StringType>
void Catalogue::AddBus(const std::string_view name, const std::vector<StringType>& stops, const bool is_circle) {
	AddBus(std::string{ name }, stops, is_circle);
}
template<typename StringType>
void Catalogue::AddBus(std::string&& name, const std::vector<StringType>& stops, const bool is_circle) {
	auto [it, is_inserted] = bus_names_.insert(std::move(name));
	if (is_inserted) {
		Bus bus;
		bus.name = *it;
		bus.is_circle = is_circle;
		bus.stops.reserve(stops.size() + stops.size()* (!is_circle));
		for (const auto& stop : stops) {
			assert(name_to_stop_.count(stop));
			bus.stops.push_back(name_to_stop_.at(stop));
			bus.unique_stop_names_.insert(name_to_stop_.at(stop)->name);
			stop_to_buses_.at(stop).insert(*it);
		}
		if (!is_circle) {
			bus.stops.resize(stops.size()*2 - 1);
			std::reverse_copy(bus.stops.begin(),
				std::next(bus.stops.begin(), stops.size() - 1),
				std::next(bus.stops.begin(), stops.size())
			);
		}
		CalculateLength(bus);
		buses_.push_back(bus);
		name_to_bus_[*it] = &buses_.back();
	}
}

} //end namespace transportCatalogue