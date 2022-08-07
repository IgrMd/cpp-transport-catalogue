//Sprint 9. transport_catalogue v0.1.1.

#include "input_reader.h"
#include "stat_reader.h"
#include "transport_catalogue.h"

#include <iostream>

using namespace std::literals;

int main() {
	transport_catalogue::Catalogue catalogue;

	std::vector<std::string> inp_queries = transport_catalogue::io::ReadQueries(std::cin);
	transport_catalogue::io::ProcessQueries(inp_queries, catalogue, std::cout);
	std::vector<std::string> outp_queries = transport_catalogue::io::ReadQueries(std::cin);
	transport_catalogue::io::ProcessQueries(outp_queries, catalogue, std::cout);

	return 0;
}