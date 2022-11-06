#include "serialization.h"
#include "transport_catalogue.pb.h"

#include <algorithm>
#include <cassert>
#include <fstream>
#include <filesystem>
#include <vector>
#include <unordered_map>

namespace transport_catalogue_serialize {

using namespace std;
using transport_catalogue::TransportCatalogue;

static vector<Stop> CreateStopMessages(const TransportCatalogue& db) {
	const auto& db_stops = db.GetStops();
	vector<Stop> stop_message_list;
	stop_message_list.reserve(db_stops.size());
	for (const auto& db_stop : db_stops) {
		Stop stop_message;
		stop_message.set_name(db_stop.name);
		stop_message.mutable_coordinates()->set_lat(db_stop.coordinates.lat);
		stop_message.mutable_coordinates()->set_lng(db_stop.coordinates.lng);
		stop_message_list.push_back(std::move(stop_message));
	}
	return stop_message_list;
}

static unordered_map<string_view, uint32_t> CreateStopAssociativity(const vector<Stop>& stops) {
	unordered_map<string_view, uint32_t> stop_to_index;
	for (size_t i = 0; i < stops.size(); ++i) {
		const auto& stop_message = stops[i];
		stop_to_index[stop_message.name()] = i;
	}
	return stop_to_index;
}

static vector<Bus> CreateBusMessages(const TransportCatalogue& db,
	const unordered_map<string_view, uint32_t>& stop_to_index) {
	const auto& db_buses = db.GetBuses();
	vector<Bus> bus_message_list;
	bus_message_list.reserve(db_buses.size());
	for (const auto& db_bus : db_buses) {
		Bus bus_message;
		bus_message.set_name(db_bus.name);
		bus_message.set_is_round_trip(db_bus.is_roundtrip);
		size_t stop_count = db_bus.is_roundtrip ? db_bus.stops.size() : (db_bus.stops.size() + 1) / 2;
		for (size_t i = 0; i < stop_count; ++i) {
			const auto db_stop = db_bus.stops[i];
			bus_message.add_stop(stop_to_index.at(db_stop->name));
		}
		bus_message_list.push_back(std::move(bus_message));
	}
	return bus_message_list;
}

static void CreateStopDistanceMessages([[out]] vector<Stop>& stops,
	const TransportCatalogue& db, const unordered_map<string_view, uint32_t>& stop_to_index) {
	const auto& db_distances = db.GetDistances();
	for (const auto& [stop_pair, distance] : db_distances) {
		uint32_t index_from = stop_to_index.at(stop_pair.from->name);
		uint32_t index_to = stop_to_index.at(stop_pair.to->name);
		RoadDistance rd_message;
		rd_message.set_destanation(index_to);
		rd_message.set_distance(distance);
		*stops[index_from].add_road_distance() = rd_message;
	}
}

static void CreateTransportCatalogueMessages(
	const TransportCatalogue& db, DataBase& serialized_db)
{
	vector<Stop> stop_message_list = CreateStopMessages(db);
	unordered_map<string_view, uint32_t> stop_to_index = CreateStopAssociativity(stop_message_list);
	CreateStopDistanceMessages(stop_message_list, db, stop_to_index);
	vector<Bus> bus_message_list = CreateBusMessages(db, stop_to_index);
	for (auto& stop_message : stop_message_list) {
		*serialized_db.add_stop() = std::move(stop_message);
	}
	for (auto& bus_message : bus_message_list) {
		*serialized_db.add_bus() = std::move(bus_message);
	}
}

static void SetColorMessage(Color* color_message, const svg::Color& color) {
	if (holds_alternative<string>(color)) {
		color_message->set_str_color(get<string>(color));
		return;
	}
	if (holds_alternative<svg::Rgba>(color)) {
		auto rgba_color = get<svg::Rgba>(color);
		color_message->mutable_rgba()->set_r(rgba_color.red);
		color_message->mutable_rgba()->set_g(rgba_color.green);
		color_message->mutable_rgba()->set_b(rgba_color.blue);
		color_message->mutable_rgba()->set_opacity(rgba_color.opacity);
		return;
	}
	if (holds_alternative<svg::Rgb>(color)) {
		auto rgb_color = get<svg::Rgb>(color);
		color_message->mutable_rgb()->set_r(rgb_color.red);
		color_message->mutable_rgb()->set_g(rgb_color.green);
		color_message->mutable_rgb()->set_b(rgb_color.blue);
		return;
	}
}

static void CreateRenderSettingsMessages(
	const renderer::RenderSettings& render_settings, DataBase& serialized_db)
{
	auto& render_settings_msg = *serialized_db.mutable_render_settings();
	render_settings_msg.set_width(render_settings.width);
	render_settings_msg.set_height(render_settings.height);
	render_settings_msg.set_padding(render_settings.padding);
	render_settings_msg.set_stop_radius(render_settings.stop_radius);
	render_settings_msg.set_line_width(render_settings.line_width);
	render_settings_msg.set_bus_label_font_size(render_settings.bus_label_font_size);
	render_settings_msg.mutable_bus_label_offset()->set_x(render_settings.bus_label_offset.x);
	render_settings_msg.mutable_bus_label_offset()->set_y(render_settings.bus_label_offset.y);
	render_settings_msg.set_stop_label_font_size(render_settings.stop_label_font_size);
	render_settings_msg.mutable_stop_label_offset()->set_x(render_settings.stop_label_offset.x);
	render_settings_msg.mutable_stop_label_offset()->set_y(render_settings.stop_label_offset.y);
	SetColorMessage(render_settings_msg.mutable_underlayer_color(), render_settings.underlayer_color);
	render_settings_msg.set_underlayer_width(render_settings.underlayer_width);
	for (const auto& color : render_settings.color_palette) {
		Color color_message;
		SetColorMessage(&color_message, color);
		*render_settings_msg.add_color_palette() = color_message;
	}
}

static void CreateRoutingSettingsMessages(
	const transport_router::RoutingSettings routing_settings, DataBase& serialized_db) {
	auto& routing_settings_msg = *serialized_db.mutable_routing_settings();
	routing_settings_msg.set_bus_velocity(routing_settings.bus_velocity);
	routing_settings_msg.set_bus_wait_time(routing_settings.bus_wait_time);
}

void Serialize(
	const TransportCatalogue&					db,
	const renderer::RenderSettings&				render_settings,
	const transport_router::RoutingSettings&	routing_settings,
	const std::filesystem::path&				path) {
	
	DataBase serialized_db;
	CreateTransportCatalogueMessages(db, serialized_db);
	CreateRenderSettingsMessages(render_settings, serialized_db);
	CreateRoutingSettingsMessages(routing_settings, serialized_db);

	std::ofstream out(path, std::ios::binary);
	if (out.is_open()) {
		serialized_db.SerializeToOstream(&out);
	}
}

static unordered_map<int, string_view> AddStops(TransportCatalogue* db, const DataBase& serialized_db) {
	unordered_map<int, string_view> index_to_stop_name;
	const int stop_count = serialized_db.stop_size();
	for (int i = 0; i < stop_count; ++i) {
		const auto& stop_message = serialized_db.stop(i);
		db->AddStop(stop_message.name(),
			{ stop_message.coordinates().lat(), stop_message.coordinates().lng() });
		index_to_stop_name[i] = stop_message.name();
	}
	return index_to_stop_name;
}

static void SetStopDistances(TransportCatalogue* db,
	const DataBase& serialized_db, const unordered_map<int, string_view>& index_to_stop_name) {
	const int stop_count = serialized_db.stop_size();
	for (const auto& stop_message : serialized_db.stop()) {
		string_view stop_from = stop_message.name();
		for (const auto& road_distance_message : stop_message.road_distance()) {
			string_view stop_to = index_to_stop_name.at(road_distance_message.destanation());
			int distance = road_distance_message.distance();
			db->SetStopDistance(stop_from, stop_to, distance);
		}
	}
}

static void AddBuses(TransportCatalogue* db,
	const DataBase& serialized_db, const unordered_map<int, string_view>& index_to_stop_name) {
	for (const auto& bus_message : serialized_db.bus()) {
		vector<string_view> stops;
		stops.reserve(bus_message.stop_size());
		for (uint32_t stop_index : bus_message.stop()) {
			stops.push_back(index_to_stop_name.at(stop_index));
		}
		db->AddBus(bus_message.name(), stops, bus_message.is_round_trip());
	}
}

static TransportCatalogue DeserializeTransportCatalogue(const DataBase& serialized_db) {
	TransportCatalogue db;
	auto index_to_stop_name = AddStops(&db, serialized_db);
	SetStopDistances(&db, serialized_db, index_to_stop_name);
	AddBuses(&db, serialized_db, index_to_stop_name);
	return db;
}

static svg::Color GetSvgColor(const Color& color_msg) {
	if (color_msg.has_rgba()) {
		svg::Rgba color;
		color.red = color_msg.rgba().r();
		color.green = color_msg.rgba().g();
		color.blue = color_msg.rgba().b();
		color.opacity = color_msg.rgba().opacity();
		return { color };
	}
	if (color_msg.has_rgb()) {
		svg::Rgb color;
		color.red = color_msg.rgb().r();
		color.green = color_msg.rgb().g();
		color.blue = color_msg.rgb().b();
		return { color };
	}
	if (!color_msg.str_color().empty()) {
		return { color_msg.str_color() };
	}
	return {};
}

static renderer::RenderSettings DeserializeRenderSettings(const DataBase& serialized_db) {
	renderer::RenderSettings render_settings;
	const auto& render_settings_msg = serialized_db.render_settings();
	render_settings.width = render_settings_msg.width();
	render_settings.height = render_settings_msg.height();
	render_settings.padding = render_settings_msg.padding();
	render_settings.stop_radius = render_settings_msg.stop_radius();
	render_settings.line_width = render_settings_msg.line_width();
	render_settings.bus_label_font_size = render_settings_msg.bus_label_font_size();
	render_settings.bus_label_offset.x = render_settings_msg.bus_label_offset().x();
	render_settings.bus_label_offset.y = render_settings_msg.bus_label_offset().y();
	render_settings.stop_label_font_size = render_settings_msg.stop_label_font_size();
	render_settings.stop_label_offset.x = render_settings_msg.stop_label_offset().x();
	render_settings.stop_label_offset.y = render_settings_msg.stop_label_offset().y();
	render_settings.underlayer_color = GetSvgColor(render_settings_msg.underlayer_color());
	render_settings.underlayer_width = render_settings_msg.underlayer_width();
	for (const auto& color_msg : render_settings_msg.color_palette()) {
		render_settings.color_palette.push_back(GetSvgColor(color_msg));
	}
	return render_settings;
}

static transport_router::RoutingSettings DeserializeRoutingSettings(const DataBase& serialized_db) {
	const auto& routing_settings_msg = serialized_db.routing_settings();
	return transport_router::RoutingSettings{}.
		SetBusVelocity(routing_settings_msg.bus_velocity()).
		SetBusWaitTime(routing_settings_msg.bus_wait_time());
}

tuple<TransportCatalogue, renderer::RenderSettings, transport_router::RoutingSettings> Deserialize(
	const std::filesystem::path& path) {
	std::ifstream in(path, std::ios::binary);
	if (!in.is_open()) {
		return {};
	}
	DataBase serialized_db;
	serialized_db.ParseFromIstream(&in);
	TransportCatalogue db = DeserializeTransportCatalogue(serialized_db);
	renderer::RenderSettings render_settings = DeserializeRenderSettings(serialized_db);
	transport_router::RoutingSettings routing_settings = DeserializeRoutingSettings(serialized_db);

	return {move(db), move(render_settings), move(routing_settings)};
}

} // namespace transport_catalogue_serialize 
