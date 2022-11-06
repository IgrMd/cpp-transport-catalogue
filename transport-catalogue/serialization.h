#pragma once;

#include "json_reader.h"
#include "map_renderer.h"
#include "router.h"
#include "transport_catalogue.h"
#include "transport_router.h"

#include <tuple>

namespace transport_catalogue_serialize {

void Serialize(
	const transport_catalogue::TransportCatalogue&	db,
	const renderer::RenderSettings& 				render_settings,
	const transport_router::RoutingSettings&		routing_settings,
	const std::filesystem::path&					path);

std::tuple<
	transport_catalogue::TransportCatalogue,
	renderer::RenderSettings, 
	transport_router::RoutingSettings> Deserialize(const std::filesystem::path& path);

} // namespace transport_catalogue_serialize 