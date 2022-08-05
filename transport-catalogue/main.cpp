//Sprint 9. TransportCatalogue v0.1.0.

#include "input_reader.h"
#include "stat_reader.h"
#include "transport_catalogue.h"

#include <iostream>

using namespace std::literals;

int main() {
	transportCatalogue::Catalogue catalogue;

	std::vector<std::string> inp_queries = transportCatalogue::io::ReadQueries(std::cin);
	transportCatalogue::io::ProcessQueries(inp_queries, catalogue, std::cout);
	std::vector<std::string> outp_queries = transportCatalogue::io::ReadQueries(std::cin);
	transportCatalogue::io::ProcessQueries(outp_queries, catalogue, std::cout);

	return 0;
}