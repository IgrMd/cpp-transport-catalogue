#include "json_reader.h"

namespace json_reader {

using namespace std::string_literals;

static const std::string BASE_REQUESTS = "base_requests"s;
static const std::string RENDER_SETTINGS = "render_settings"s;
static const std::string STAT_REQUESTS = "stat_requests"s;

using namespace json;
using transport_catalogue::TransportCatalogue;
using renderer::RenderSettings;

Document ReadFromJSON(std::istream& input) {
	return { Load(input) };
}

void ProcessBaseRequests(TransportCatalogue& t_catalogue,
	const Document& raw_requests) {
	assert(raw_requests.GetRoot().IsMap());
	if ( !raw_requests.GetRoot().AsMap().count(BASE_REQUESTS) ) {
		return;
	}
	const auto& requests = raw_requests.GetRoot().AsMap().at(BASE_REQUESTS).AsArray();
	if (requests.empty()) { return; }
	std::vector<detail::AddStopRequest> add_stop_requests;
	std::vector<detail::AddBusRequest> add_bus_requests;
	for (const Node& request : requests) {
		if (request.AsMap().at("type"s) == "Stop"s) {
			detail::AddStopRequest add_request;
			add_request.name = request.AsMap().at("name"s).AsString();
			add_request.latitude = request.AsMap().at("latitude"s).AsDouble();
			add_request.longitude = request.AsMap().at("longitude"s).AsDouble();
			for (const auto& [name, distance] : request.AsMap().at("road_distances"s).AsMap()) {
				add_request.name_to_dist.insert({ name, distance.AsInt() });
			}
			add_stop_requests.push_back(std::move(add_request));
		} else if (request.AsMap().at("type"s) == "Bus"s) {
			detail::AddBusRequest add_request;
			add_request.name = request.AsMap().at("name"s).AsString();
			add_request.is_roundtrip = request.AsMap().at("is_roundtrip"s).AsBool();
			for (const Node& bus : request.AsMap().at("stops"s).AsArray()) {
				add_request.stops.push_back(bus.AsString());
			}
			add_bus_requests.push_back(std::move(add_request));
		} else {
			assert(false);
		}
	}
	for (const auto& add_stop_query : add_stop_requests) {
		t_catalogue.AddStop(add_stop_query.name, add_stop_query.latitude, add_stop_query.longitude);
	}
	for (const auto& add_stop_query : add_stop_requests) {
		t_catalogue.SetStopDistances(add_stop_query.name, add_stop_query.name_to_dist);
	}
	for (const auto& add_bus_query : add_bus_requests) {
		t_catalogue.AddBus(add_bus_query.name, add_bus_query.stops, add_bus_query.is_roundtrip);
	}
}

void ProcessStatRequests(TransportCatalogue& t_catalogue,
	const Document& raw_requests, std::ostream& output) {
	using detail::RequestType;
	assert(raw_requests.GetRoot().IsMap());
	if ( !raw_requests.GetRoot().AsMap().count(STAT_REQUESTS) ) {
		return;
	}
	const auto& requests = raw_requests.GetRoot().AsMap().at(STAT_REQUESTS).AsArray();
	if (requests.empty()) { return; }
	std::vector<detail::StatRequest> stat_requests;
	for (const Node& request : requests) {
		detail::StatRequest stat_request;
		if (request.AsMap().at("type"s) == "Stop"s) {
			stat_request.type = RequestType::StopStat;
		} else if (request.AsMap().at("type"s) == "Bus"s) {
			stat_request.type = RequestType::BusStat;
		} else {
			stat_request.type = RequestType::INCORRECT;
			assert(false);
		}
		stat_request.id = request.AsMap().at("id"s).AsInt();
		stat_request.name = request.AsMap().at("name"s).AsString();
		stat_requests.push_back(std::move(stat_request));
	}
	Array stats;
	for (const auto& stat_request : stat_requests) {
		if (stat_request.type == RequestType::StopStat) {
			auto stop_stat = t_catalogue.GetStopStat(stat_request.name);
			if (stop_stat) {
				Array buses;
				for (const auto& bus : stop_stat->buses) {
					buses.push_back(std::string{ bus });
				}
				Dict stat{ {"buses"s, buses}, {"request_id"s, stat_request.id } };
				stats.push_back(std::move(stat));
			} else {
				Dict stat{ {"request_id"s, stat_request.id }, {"error_message"s, "not found"s} };
				stats.push_back(std::move(stat));
			}
		} if (stat_request.type == RequestType::BusStat) {
			auto bus_stat = t_catalogue.GetBusStat(stat_request.name);
			if (bus_stat) {
				Dict stat;
				stat.insert({ "curvature"s, (bus_stat->length_curv / bus_stat->length_geo) });
				stat.insert({ "request_id"s, stat_request.id });
				stat.insert({ "route_length"s, bus_stat->length_curv });
				stat.insert({ "stop_count"s, static_cast<int>(bus_stat->stops_count) });
				stat.insert({ "unique_stop_count"s, static_cast<int>(bus_stat->unique_stops_count) });
				stats.push_back(std::move(stat));
			} else {
				Dict stat{ {"request_id"s, stat_request.id }, {"error_message"s, "not found"s} };
				stats.push_back(std::move(stat));
			}
		}
	}
	Print(Document{ Node{ std::move(stats) } }, output);
}

RenderSettings ProcessRenderSettings(const Document& raw_requests) {
	RenderSettings result;
	assert(raw_requests.GetRoot().IsMap());
	if ( !raw_requests.GetRoot().AsMap().count(RENDER_SETTINGS) ) {
		return {};
	}
	const auto& settings = raw_requests.GetRoot().AsMap().at(RENDER_SETTINGS).AsMap();
	if (settings.empty()) { return {}; }
	result.width = settings.at("width"s).AsDouble();
	result.height = settings.at("height"s).AsDouble();

	result.padding = settings.at("padding"s).AsDouble();

	result.line_width = settings.at("line_width"s).AsDouble();
	result.stop_radius = settings.at("stop_radius"s).AsDouble();

	result.bus_label_font_size = settings.at("bus_label_font_size"s).AsInt();
	const auto& bus_label_offset = settings.at("bus_label_offset"s).AsArray();
	result.bus_label_offset = { bus_label_offset[0].AsDouble(),
		bus_label_offset[1].AsDouble()
	};

	result.stop_label_font_size = settings.at("stop_label_font_size"s).AsInt();
	const auto& stop_label_offset = settings.at("stop_label_offset"s).AsArray();
	result.stop_label_offset = { stop_label_offset[0].AsDouble(),
		stop_label_offset[1].AsDouble() };

	result.underlayer_color = detail::GetColor(settings.at("underlayer_color"s));
	result.underlayer_width = settings.at("underlayer_width"s).AsDouble();

	result.width = settings.at("width"s).AsDouble();

	std::vector<svg::Color> color_palette;
	for (const auto& color : settings.at("color_palette"s).AsArray()) {
		color_palette.push_back(detail::GetColor(color));
	}
	result.color_palette = std::move(color_palette);
	return result;
}

namespace detail {

svg::Color GetColor(const Node& color) {
	svg::Color result;
	if (color.IsString()) {
		result = color.AsString();
	} else if (color.IsArray()) {
		const int r = color.AsArray()[0].AsInt();
		const int g = color.AsArray()[1].AsInt();
		const int b = color.AsArray()[2].AsInt();
		if (color.AsArray().size() == 4) {
			const double opacity = color.AsArray()[3].AsDouble();
			result = svg::Color{ svg::Rgba{ r, g, b, opacity } };
		} else if (color.AsArray().size() == 3) {
			result = svg::Color{ svg::Rgb{ r, g, b } };
		} else {
			assert(false);
		}
	}
	return result;
}

}//end namespace detail

}//end namespace json_reader