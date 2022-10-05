#include "json_reader.h"
#include "map_renderer.h"
#include "request_handler.h"
#include "transport_router.h"

//#include "TESTS.h"

#include <iostream>
#include <fstream>

int main() {
	using namespace std::string_literals;
	using namespace transport_catalogue;
	using namespace transport_router;
	using namespace renderer;
	using namespace json_reader;

	//std::ifstream inp("input4.json");
	//std::ofstream out("output.json");

	auto doc = ReadFromJSON(std::cin);
	//auto doc = ReadFromJSON(inp);
	TransportCatalogue t_catalogue;
	ProcessBaseRequests(t_catalogue, doc);

	TransportRouter router(t_catalogue, ProcessRoutingSettings(doc));

	MapRenderer renderer(ProcessRenderSettings(doc));
	RequestHandler req_handler(t_catalogue, renderer, router);
	ProcessStatRequests(req_handler, doc, std::cout);
	//ProcessStatRequests(req_handler, doc, out);

}