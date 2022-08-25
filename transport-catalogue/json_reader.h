#pragma once
#include "json.h"
#include "request_handler.h"
#include "map_renderer.h"
#include "transport_catalogue.h"

#include <cassert>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>

namespace json_reader {


using transport_catalogue::TransportCatalogue;

json::Document ReadFromJSON(std::istream& input);

//Обрабатывает запросы на добавление в базу
void ProcessBaseRequests(TransportCatalogue& t_catalogue,
	const json::Document& raw_requests);

//Обрабатывает запросы чтения и выводит результат в поток
void ProcessStatRequests(TransportCatalogue& t_catalogue,
	const json::Document& raw_requests, std::ostream& output);

//обрабатывает настройти рендера
renderer::RenderSettings ProcessRenderSettings(const json::Document& raw_requests);

namespace detail {

enum class RequestType {
	INCORRECT,
	StopStat,
	BusStat
};
struct AddStopRequest {
	std::string_view name;
	double latitude = 0;
	double longitude = 0;
	std::unordered_map<std::string_view, int> name_to_dist;
};
struct AddBusRequest {
	std::string_view name;
	std::vector<std::string_view> stops;
	bool is_roundtrip = false;
};
struct StatRequest {
	int id = 0;
	RequestType type;
	std::string_view name;
};

//Возвращает цвет в формате svg::Color
svg::Color GetColor(const json::Node& color);

}//end namespace detail


} //end namespace json_reader