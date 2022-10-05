#include "json_reader.h"

namespace json_reader {

using namespace std::string_literals;
using namespace std::string_view_literals;
using detail::RequestType;

static const std::string BASE_REQUESTS = "base_requests"s;
static const std::string RENDER_SETTINGS = "render_settings"s;
static const std::string ROUTING_SETTINGS = "routing_settings"s;
static const std::string STAT_REQUESTS = "stat_requests"s;

static const std::unordered_map<std::string_view, RequestType> REQUESTS_LIST{
	{"Stop"sv, RequestType::STOP_STAT},
	{"Bus"sv, RequestType::BUS_STAT},
	{"Map"sv, RequestType::MAP},
	{"Route"sv, RequestType::ROUTE}
};

using namespace json;
using transport_catalogue::TransportCatalogue;
using renderer::RenderSettings;
using transport_router::RoutingSettings;

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
	if ( requests.empty() ) { return; }
	std::vector<detail::AddStopRequest> add_stop_requests;
	std::vector<detail::AddBusRequest> add_bus_requests;
	for ( const Node& request : requests ) {
		if ( request.AsMap().at("type"s) == "Stop"s ) {
			detail::AddStopRequest add_request;
			add_request.name = request.AsMap().at("name"s).AsString();
			add_request.latitude = request.AsMap().at("latitude"s).AsDouble();
			add_request.longitude = request.AsMap().at("longitude"s).AsDouble();
			for ( const auto& [name, distance] : request.AsMap().at("road_distances"s).AsMap() ) {
				add_request.name_to_dist.insert({ name, distance.AsInt() });
			}
			add_stop_requests.push_back(std::move(add_request));
		} else if ( request.AsMap().at("type"s) == "Bus"s ) {
			detail::AddBusRequest add_request;
			add_request.name = request.AsMap().at("name"s).AsString();
			add_request.is_roundtrip = request.AsMap().at("is_roundtrip"s).AsBool();
			for ( const Node& bus : request.AsMap().at("stops"s).AsArray() ) {
				add_request.stops.push_back(bus.AsString());
			}
			add_bus_requests.push_back(std::move(add_request));
		} else {
			assert(false);
		}
	}
	for ( const auto& add_stop_query : add_stop_requests ) {
		t_catalogue.AddStop(add_stop_query.name,
			{ add_stop_query.latitude, add_stop_query.longitude });
	}
	for ( const auto& add_stop_query : add_stop_requests ) {
		t_catalogue.SetStopDistances(add_stop_query.name,
			add_stop_query.name_to_dist);
	}
	for ( const auto& add_bus_query : add_bus_requests ) {
		t_catalogue.AddBus(add_bus_query.name,
			add_bus_query.stops, add_bus_query.is_roundtrip);
	}
}

void ProcessStatRequests(const RequestHandler& req_handler,
	const Document& raw_requests, std::ostream& output) {
	using detail::RequestType;
	assert(raw_requests.GetRoot().IsMap());
	if ( !raw_requests.GetRoot().AsMap().count(STAT_REQUESTS) ) {
		return;
	}
	const auto& requests = raw_requests.GetRoot().AsMap().at(STAT_REQUESTS).AsArray();
	if ( requests.empty() ) { return; }
	std::vector<detail::StatRequest> stat_requests;
	for ( const Node& request : requests ) {
		detail::StatRequest stat_request;
		const std::string& request_name = request.AsMap().at("type"s).AsString();
		assert(REQUESTS_LIST.count(request_name) > 0);
		stat_request.type = REQUESTS_LIST.at(request_name);
		stat_request.id = request.AsMap().at("id"s).AsInt();
		if (stat_request.type == RequestType::STOP_STAT || stat_request.type == RequestType::BUS_STAT) {
			stat_request.request_data = request.AsMap().at("name"s).AsString();
		}
		if (stat_request.type == RequestType::ROUTE) {
			std::string_view from, to;
			from = request.AsMap().at("from"s).AsString();
			to = request.AsMap().at("to"s).AsString();
			stat_request.request_data = std::pair{ from, to };
		}
		stat_requests.push_back(std::move(stat_request));
	}
	Builder stats;
	stats.StartArray();
	for ( const auto& stat_request : stat_requests ) {
		switch ( stat_request.type ) {
		case RequestType::STOP_STAT:
			detail::ProcessStopStatRequest(req_handler, stats, stat_request);
			break;
		case RequestType::BUS_STAT:
			detail::ProcessBusStatRequest(req_handler, stats, stat_request);
			break;
		case RequestType::MAP:
			detail::ProcessMapRequest(req_handler, stats, stat_request);
			break;
		case RequestType::ROUTE:
			detail::ProcessRouteRequest(req_handler, stats, stat_request);
			break;
		default:
			assert(false);
			break;
		}
	}
	stats.EndArray();
	Print(Document{ Node{ stats.Build()} }, output);
}

RenderSettings ProcessRenderSettings(const Document& raw_requests) {
	RenderSettings result;
	assert(raw_requests.GetRoot().IsMap());
	if ( !raw_requests.GetRoot().AsMap().count(RENDER_SETTINGS) ) {
		return {};
	}
	const auto& settings = raw_requests.GetRoot().AsMap().at(RENDER_SETTINGS).AsMap();
	if ( settings.empty() ) { return {}; }
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
	for ( const auto& color : settings.at("color_palette"s).AsArray() ) {
		color_palette.push_back(detail::GetColor(color));
	}
	result.color_palette = std::move(color_palette);
	return result;
}

RoutingSettings ProcessRoutingSettings(const Document& raw_requests) {
	assert(raw_requests.GetRoot().IsMap());
	if ( !raw_requests.GetRoot().AsMap().count(ROUTING_SETTINGS) ) {
		return {};
	}
	const auto& settings = raw_requests.GetRoot().AsMap().at(ROUTING_SETTINGS).AsMap();
	return RoutingSettings{}
		.SetBusWaitTime(settings.at("bus_wait_time"s).AsDouble())
		.SetBusVelocity(settings.at("bus_velocity"s).AsDouble());
}


namespace detail {

svg::Color GetColor(const Node& color) {
	svg::Color result;
	if (color.IsString()) {
		result = color.AsString();
	} else if ( color.IsArray() ) {
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

//stat_request.type == RequestType::StopStat
void ProcessStopStatRequest(const RequestHandler& req_handler, Builder& stats,
	const detail::StatRequest& stat_request) {
	auto stop_stat = req_handler.GetStopStat(std::get<std::string_view>(stat_request.request_data));
	if ( stop_stat ) {
		Builder busses;
		busses.StartArray();
		for ( const auto& bus : stop_stat->busses ) {
			busses.Value(std::string{ bus });
		}
		busses.EndArray();
		stats.StartDict()
			.Key("buses"s).Value(busses.Build().AsArray())
			.Key("request_id"s).Value(stat_request.id)
			.EndDict();
	} else {
		stats.StartDict()
			.Key("request_id"s).Value(stat_request.id)
			.Key("error_message"s).Value("not found"s)
			.EndDict();
	}
}

//stat_request.type == RequestType::BusStat
void ProcessBusStatRequest(const RequestHandler& req_handler, Builder& stats,
	const detail::StatRequest& stat_request) {
	auto bus_stat = req_handler.GetBusStat(std::get<std::string_view>(stat_request.request_data));
	if ( bus_stat ) {
		stats.StartDict()
			.Key("curvature"s).Value(bus_stat->length_curv / bus_stat->length_geo)
			.Key("request_id"s).Value(stat_request.id)
			.Key("route_length"s).Value(bus_stat->length_curv)
			.Key("stop_count"s).Value(static_cast<int>(bus_stat->stops_count))
			.Key("unique_stop_count"s).Value(static_cast<int>(bus_stat->unique_stops_count))
			.EndDict();
	} else {
		stats.StartDict()
			.Key("request_id"s).Value(stat_request.id)
			.Key("error_message"s).Value("not found"s)
			.EndDict();
	}
}

//stat_request.type == RequestType::MAP
void ProcessMapRequest(const RequestHandler& req_handler, Builder& stats,
	const detail::StatRequest& stat_request) {
	svg::Document map;
	req_handler.RenderMap(map);
	std::ostringstream os;
	map.Render(os);
	stats.StartDict()
		.Key("map"s).Value(os.str())
		.Key("request_id"s).Value(stat_request.id)
		.EndDict();
}

//stat_request.type == RequestType::ROUTE
void ProcessRouteRequest(const RequestHandler& req_handler, Builder& stats,
	const detail::StatRequest& stat_request) {
	const auto [from, to] = std::get<std::pair<std::string_view, std::string_view>>(stat_request.request_data);
	stats.StartDict();
		
	if (from == to) {
		stats.Key("items"s).StartArray().EndArray()
			.Key("request_id").Value(stat_request.id)
			.Key("total_time").Value(0)
			.EndDict();
		return;
	}
	auto route = req_handler.BuildRoute(from, to);
	if (route.size() == 0) {
		stats.Key("error_message"s).Value("not found"s)
			.Key("request_id").Value(stat_request.id)
			.EndDict();
		return;
	}
	double total_time = 0;
	stats.Key("items"s).StartArray();
	for (const auto& route_part : route) {
		total_time += route_part.weight;
		if (route_part.type == transport_router::TransportRouter::EdgeInfo::EdgeType::WAIT) {
			stats.StartDict().
				Key("stop_name"s).Value(route_part.stop_ptr->name)
				.Key("time"s).Value(route_part.weight)
				.Key("type"s).Value("Wait"s)
				.EndDict();
		}
		if (route_part.type == transport_router::TransportRouter::EdgeInfo::EdgeType::BUS) {
			stats.StartDict().
				Key("bus"s).Value(route_part.bus_ptr->name)
				.Key("span_count"s).Value(route_part.span_count)
				.Key("time"s).Value(route_part.weight)
				.Key("type"s).Value("Bus"s)
				.EndDict();
		}
	}
	stats.EndArray();
	stats.Key("request_id").Value(stat_request.id)
		.Key("total_time").Value(total_time)
		.EndDict();
}

}//end namespace detail

}//end namespace json_reader