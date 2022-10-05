#include "request_handler.h"

using namespace transport_catalogue;

RequestHandler::RequestHandler(const TransportCatalogue& db,
	const renderer::MapRenderer& renderer, const transport_router::TransportRouter& router)
	: db_(db)
	, renderer_(renderer)
	, router_(router) {}

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
	std::vector<domain::Bus> busses{ db_.GetBusses().begin(), db_.GetBusses().end() };
	std::sort(busses.begin(),
		busses.end(),
		[](const auto& lhs, const auto& rhs) {return lhs.name < rhs.name; }
	);
	std::vector<const domain::Stop*> stops = db_.GetStopsUsed();
	std::sort(stops.begin(),
		stops.end(),
		[](const auto& lhs, const auto& rhs) {return lhs->name < rhs->name; }
	);
	renderer_.RenderMap(map, busses, stops);
}

//Строит мршрут (запрос Route)
std::vector<transport_router::TransportRouter::EdgeInfo> RequestHandler::BuildRoute(
	const std::string_view from, const std::string_view to) const {
	return router_.BuildRoute(from, to);
}
