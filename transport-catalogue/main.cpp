#include "json_reader.h"
#include "map_renderer.h"
#include "request_handler.h"

//#include "TESTS.h"

#include <iostream>
//#include <fstream>

int main() {
	//TestJSON();
	using namespace std::string_literals;
	using namespace transport_catalogue;

	//std::ifstream inp("s10_final_opentest_1.json");

	auto doc = json_reader::ReadFromJSON(std::cin);
	TransportCatalogue t_catalogue;

	json_reader::ProcessBaseRequests(t_catalogue, doc);
	renderer::MapRenderer renderer(json_reader::ProcessRenderSettings(doc));
	RequestHandler req_handler(t_catalogue, renderer);
	json_reader::ProcessStatRequests(req_handler, doc, std::cout);

}