#pragma once

#include "svg.h"
#include "geo.h"
#include "domain.h"

#include <algorithm>
#include <optional>
#include <string>
#include <variant>
#include <vector>
#include <utility>

namespace renderer {

using transport_catalogue::domain::Bus;
using transport_catalogue::domain::Stop;
using transport_catalogue::geo::Coordinates;

static const double EPSILON = 1e-6;

class SphereProjector {
public:
	// points_begin и points_end задают начало и конец интервала элементов geo::Coordinates
	template <typename PointInputIt>
	SphereProjector(PointInputIt points_begin, PointInputIt points_end,
		double max_width, double max_height, double padding);

	// Проецирует широту и долготу в координаты внутри SVG-изображения
	svg::Point operator()(transport_catalogue::geo::Coordinates coords) const;

private:
	double padding_;
	double min_lon_ = 0;
	double max_lat_ = 0;
	double zoom_coeff_ = 0;

	static bool IsZero(double value);
};

struct RenderSettings {
	double width = 0;
	double height = 0;
	double padding = 0;
	double stop_radius = 0;
	double line_width = 0;
	int bus_label_font_size = 0;
	svg::Point bus_label_offset;
	int stop_label_font_size = 0;
	svg::Point stop_label_offset;
	svg::Color underlayer_color;
	double underlayer_width = 0;
	std::vector<svg::Color> color_palette;
};

class MapRenderer {
public:
	MapRenderer(const RenderSettings& render_settings);

	MapRenderer(RenderSettings&& render_settings);

	const RenderSettings& GetSettings() const;

	void RenderMap(svg::Document& map, const std::vector<Bus>& buses,
		const std::vector<const Stop*>& stops) const;

private:
	RenderSettings render_settings_;

	//Ломаные линии маршрутов
	void RenderBusRouts(svg::Document& map,
		const std::vector<Bus>& buses, const SphereProjector& proj) const;

	//Названия маршрутов
	void RenderBusNames(svg::Document& map,
		const std::vector<Bus>& buses, const SphereProjector& proj) const;

	//Точки остановок
	void RenderStopPoints(svg::Document& map,
		const std::vector<const Stop*>& stops, const SphereProjector& proj) const;

	//Названия остановок
	void RenderStopNames(svg::Document& map,
		const std::vector<const Stop*>& stops, const SphereProjector& proj) const;
};

template <typename PointInputIt>
SphereProjector::SphereProjector(PointInputIt points_begin, PointInputIt points_end,
	double max_width, double max_height, double padding)
	: padding_(padding) {
	if (points_begin == points_end) {
		return;
	}
	const auto [left_it, right_it] = std::minmax_element(
		points_begin, points_end,
		[](auto lhs, auto rhs) { return lhs.lng < rhs.lng; });
	min_lon_ = left_it->lng;
	const double max_lon = right_it->lng;
	const auto [bottom_it, top_it] = std::minmax_element(
		points_begin, points_end,
		[](auto lhs, auto rhs) { return lhs.lat < rhs.lat; });
	const double min_lat = bottom_it->lat;
	max_lat_ = top_it->lat;
	std::optional<double> width_zoom;
	if (!IsZero(max_lon - min_lon_)) {
		width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
	}
	std::optional<double> height_zoom;
	if (!IsZero(max_lat_ - min_lat)) {
		height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
	}

	if (width_zoom && height_zoom) {
		zoom_coeff_ = std::min(*width_zoom, *height_zoom);
	} else if (width_zoom) {
		zoom_coeff_ = *width_zoom;
	} else if (height_zoom) {
		zoom_coeff_ = *height_zoom;
	}
}

}//end namespace renderer