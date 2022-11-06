#include "json_reader.h"
#include "map_renderer.h"
#include "request_handler.h"
#include "serialization.h"
#include "transport_router.h"

using namespace std::literals;

void PrintUsage(std::ostream& stream = std::cerr) {
	stream << "Usage: transport_catalogue [make_base|process_requests]\n"sv;
}

int main(int argc, char* argv[]) {

	using namespace transport_catalogue;
	using namespace transport_router;
	using namespace transport_catalogue_serialize;
	using namespace renderer;
	using namespace json_reader;

	if (argc != 2) {
		PrintUsage();
		return 1;
	}

	const std::string_view mode(argv[1]);

	if (mode == "make_base"sv) {
		auto doc = ReadFromJSON(std::cin);
		TransportCatalogue t_catalogue = ProcessBaseRequests(doc);
		RenderSettings render_settings = ProcessRenderSettings(doc);
		RoutingSettings routing_settings = ProcessRoutingSettings(doc);
		std::filesystem::path path = ProcessPath(doc);
		Serialize(t_catalogue, render_settings, routing_settings, path);

	} else if (mode == "process_requests"sv) {
		auto doc = ReadFromJSON(std::cin);
		std::filesystem::path path = ProcessPath(doc);
		auto [t_catalogue, render_settings, routing_settings] = Deserialize(path);
		MapRenderer renderer(render_settings);
		TransportRouter router(t_catalogue, routing_settings);
		RequestHandler req_handler(t_catalogue, renderer, router);
		ProcessStatRequests(req_handler, doc, std::cout);
	} else {
		PrintUsage();
		return 1;
	}
}