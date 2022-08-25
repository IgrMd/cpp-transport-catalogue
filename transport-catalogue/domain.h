#pragma once
#include "geo.h"

#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace transport_catalogue {

namespace domain {

struct BusStat {
	const std::string_view name;
	size_t stops_count = 0;
	size_t unique_stops_count = 0;
	double length_geo = 0;
	int length_curv = 0;
};

struct StopStat {
	const std::string_view name;
	const std::set<std::string_view>& buses;
};

//Остановка
struct Stop {
	std::string name;
	geo::Coordinates coordinates;
};

//Автобус (маршрут)
struct Bus {
	std::string name;
	std::vector<const Stop*> stops;
	bool is_roundtrip = false;
	size_t unique_stops_count = 0;
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

}//end namespace domain

}//end namespace transport_catalogue
