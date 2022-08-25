#include "json_reader.h"
#include "map_renderer.h"
#include "request_handler.h"
//#include "TESTS.h"

#include <iostream>
#include <fstream>

int main() {
using namespace std::string_literals;
using namespace transport_catalogue;

// TestJSON();

std::ifstream input("input.json"s);
//std::ifstream input("input.txt"s);
//std::ofstream output_json("output.json"s);
//std::ofstream output_svg("output.svg"s
std::ofstream output_svg("output.txt"s);

auto doc = json_reader::ReadFromJSON(input);
//auto doc = json_reader::ReadFromJSON(std::cin);
TransportCatalogue t_catalogue;

json_reader::ProcessBaseRequests(t_catalogue, doc);
//json_reader::ProcessStatRequests(t_catalogue, doc, std::cout);

renderer::MapRenderer renderer(json_reader::ProcessRenderSettings(doc));
RequestHandler req_handler(t_catalogue, renderer);

svg::Document map;
req_handler.RenderMap(map);
//map.Render(std::cout);
map.Render(output_svg);
}