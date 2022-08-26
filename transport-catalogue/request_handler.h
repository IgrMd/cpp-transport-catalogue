#pragma once

#include "geo.h"
#include "map_renderer.h"
#include "svg.h"
#include "transport_catalogue.h"

#include <algorithm>
#include <iostream>
#include <optional>
#include <vector>

using transport_catalogue::TransportCatalogue;
using transport_catalogue::domain::BusStat;
using transport_catalogue::domain::StopStat;

class RequestHandler {
public:
	RequestHandler(const TransportCatalogue& db, const renderer::MapRenderer& renderer);

	// Возвращает информацию о маршруте (запрос Bus)
	std::optional<BusStat> GetBusStat(const std::string_view& bus_name) const;

	// Возвращает маршруты, проходящие через остановку (запрос Stop)
	std::optional<StopStat> GetStopStat(const std::string_view& stop_name) const;

	//Рисует карту (запрос Map)
	void RenderMap(svg::Document& map) const;

private:
	const TransportCatalogue& db_;
	const renderer::MapRenderer& renderer_;
};
