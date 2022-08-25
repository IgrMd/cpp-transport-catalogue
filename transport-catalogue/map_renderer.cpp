#include "map_renderer.h"

/*
 * В этом файле вы можете разместить код, отвечающий за визуализацию карты маршрутов в формате SVG.
 * Визуализация маршрутов вам понадобится во второй части итогового проекта.
 * Пока можете оставить файл пустым.
 */

namespace renderer {

using namespace transport_catalogue;

MapRenderer::MapRenderer(const RenderSettings& render_settings)
	: render_settings_(render_settings) {}

MapRenderer::MapRenderer(RenderSettings&& render_settings)
	: render_settings_(std::move(render_settings)) {}

const RenderSettings& MapRenderer::GetSettings() const {
	return render_settings_;
}

void MapRenderer::RenderMap(svg::Document& map, std::vector<domain::Bus>& buses,
	std::vector<const domain::Stop*>& stops) const {
	std::vector<geo::Coordinates> geo_coords(stops.size());
	std::transform(
		stops.begin(),
		stops.end(),
		geo_coords.begin(),
		[](const Stop* stop) {return stop->coordinates; }
	);
	const SphereProjector proj{
		geo_coords.begin(), geo_coords.end(), render_settings_.width,
		render_settings_.height,
		render_settings_.padding
	};

	const size_t colors_count = render_settings_.color_palette.size();
	size_t color_index = 0;
	//Ломаные линии маршрутов
	for (const auto& bus : buses) {
		if (bus.stops.empty()) { continue; }
		const auto& color = render_settings_.color_palette[color_index];
		svg::Polyline route;
		for (const auto& stop : bus.stops) {
			route.AddPoint(proj(stop->coordinates));
		}
		route
			.SetStrokeColor(color)
			.SetStrokeWidth(render_settings_.line_width)
			.SetFillColor({})
			.SetStrokeLineCap(svg::StrokeLineCap::ROUND)
			.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
		map.Add(route);
		++color_index;
		color_index = (color_index >= colors_count) ? 0 : color_index;
	}
	//Названия маршрутов
	color_index = 0;
	for (const auto& bus : buses) {
		if (bus.stops.empty()) { continue; }
		const auto& color = render_settings_.color_palette[color_index];
		svg::Text name;
		name
			.SetFillColor(color)
			.SetData(bus.name)
			.SetFontSize(render_settings_.bus_label_font_size)
			.SetFontFamily("Verdana")
			.SetFontWeight("bold")
			.SetPosition(proj(bus.stops.front()->coordinates))
			.SetOffset(render_settings_.bus_label_offset);
		svg::Text underlayer_name = name;
		underlayer_name
			.SetFillColor(render_settings_.underlayer_color)
			.SetStrokeColor(render_settings_.underlayer_color)
			.SetStrokeWidth(render_settings_.underlayer_width)
			.SetStrokeLineCap(svg::StrokeLineCap::ROUND)
			.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
		map.Add(underlayer_name);
		map.Add(name);
		const auto& last_stop = bus.stops.size() > 2 ?
			bus.stops[bus.stops.size() / 2] : bus.stops.back();
		if (!bus.is_roundtrip && bus.stops.front()->name != last_stop->name) {
			name
				.SetPosition(proj(last_stop->coordinates))
				.SetOffset(render_settings_.bus_label_offset);
			underlayer_name
				.SetPosition(proj(last_stop->coordinates))
				.SetOffset(render_settings_.bus_label_offset);;
			map.Add(underlayer_name);
			map.Add(name);
		}
		++color_index;
		color_index = (color_index >= colors_count) ? 0 : color_index;
	}
	//Точки остановок
	svg::Circle stop_point;
	stop_point
		.SetFillColor("white")
		.SetRadius(render_settings_.stop_radius);
	for (const auto stop : stops) {
		stop_point.SetCenter(proj(stop->coordinates));
		map.Add(stop_point);
	}
	//Названия остановок
	svg::Text stop_name;
	stop_name
		.SetFillColor("black")
		.SetFontSize(render_settings_.stop_label_font_size)
		.SetFontFamily("Verdana")
		.SetOffset(render_settings_.stop_label_offset);
	svg::Text stop_underlayer_name = stop_name;
	stop_underlayer_name
		.SetFillColor(render_settings_.underlayer_color)
		.SetStrokeColor(render_settings_.underlayer_color)
		.SetStrokeWidth(render_settings_.underlayer_width)
		.SetStrokeLineCap(svg::StrokeLineCap::ROUND)
		.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
	for (const auto stop : stops) {
		stop_name
			.SetData(stop->name)
			.SetPosition(proj(stop->coordinates));
		stop_underlayer_name
			.SetData(stop->name)
			.SetPosition(proj(stop->coordinates));
		map.Add(stop_underlayer_name);
		map.Add(stop_name);
	}
}

svg::Point SphereProjector::operator()(geo::Coordinates coords) const {
	return {
		(coords.lng - min_lon_) * zoom_coeff_ + padding_,
		(max_lat_ - coords.lat) * zoom_coeff_ + padding_
	};
}

bool SphereProjector::IsZero(double value) {
	return std::abs(value) < EPSILON;
}

}//end namespace renderer