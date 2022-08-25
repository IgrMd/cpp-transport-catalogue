#include "request_handler.h"

/*
 * Здесь можно было бы разместить код обработчика запросов к базе, содержащего логику, которую не
 * хотелось бы помещать ни в transport_catalogue, ни в json reader.
 *
 * Если вы затрудняетесь выбрать, что можно было бы поместить в этот файл,
 * можете оставить его пустым.
 */

using namespace transport_catalogue;

RequestHandler::RequestHandler(const TransportCatalogue& db, const renderer::MapRenderer& renderer)
	:db_(db), renderer_(renderer) {}

// Возвращает информацию о маршруте (запрос Bus)
std::optional<BusStat> RequestHandler::GetBusStat(const std::string_view& bus_name) const {
	return db_.GetBusStat(bus_name);
}

// Возвращает маршруты, проходящие через остановку
std::optional<StopStat> RequestHandler::GetBusesByStop(const std::string_view& stop_name) const {
	return db_.GetStopStat(stop_name);
}

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
