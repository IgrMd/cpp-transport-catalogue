#include "request_handler.h"

using namespace transport_catalogue;

RequestHandler::RequestHandler(const TransportCatalogue& db, const renderer::MapRenderer& renderer)
	:db_(db), renderer_(renderer) {}

// Возвращает информацию о маршруте (запрос Bus)
std::optional<BusStat> RequestHandler::GetBusStat(const std::string_view& bus_name) const {
	return db_.GetBusStat(bus_name);
}

// Возвращает маршруты, проходящие через остановку (запрос Stop)
std::optional<StopStat> RequestHandler::GetStopStat(const std::string_view& stop_name) const {
	return db_.GetStopStat(stop_name);
}

//Рисует карту (запрос Map)
void RequestHandler::RenderMap(svg::Document& map) const {
	std::vector<domain::Bus> buses{ db_.GetBuses().begin(), db_.GetBuses().end() };
	std::sort(buses.begin(),
		buses.end(),
		[](const auto& lhs, const auto& rhs) {return lhs.name < rhs.name; }
	);
	std::vector<const domain::Stop*> stops = db_.GetStopsUsed();
	std::sort(stops.begin(),
		stops.end(),
		[](const auto& lhs, const auto& rhs) {return lhs->name < rhs->name; }
	);
	renderer_.RenderMap(map, buses, stops);
}
