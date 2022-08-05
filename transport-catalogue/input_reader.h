#pragma once

#include "stat_reader.h"
#include "transport_catalogue.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>

namespace transportCatalogue {

namespace detail {

enum class QueryType {
	INCORRECT,
	AddStop,
	AddBus,
	PrintStop,
	PrintBus
};
struct AddStopQuery {
	std::string_view name;
	double latitude = 0;
	double longitude = 0;
	std::unordered_map<std::string_view, int> name_to_dist;
};
struct AddBusQuery {
	std::string_view name;
	std::vector<std::string_view> stops;
	bool is_circle = false;
};
struct PrintQuery {
	QueryType type;
	std::string_view name;
};
void CutOff(std::string_view& str);

std::string_view FirstCommand(std::string_view str);

std::string_view SecondCommand(std::string_view str);

QueryType GetQueryType(const std::string& query);

AddBusQuery ParseAddBusQuery(const std::string_view str);

AddStopQuery ParseAddStopQuery(const std::string_view str);

std::string ReadLine(std::istream& in);

int ReadLineWithNumber(std::istream& in);

} //end namespace detail

namespace io {

void ProcessQueries(std::vector<std::string>& queries, Catalogue& catalogue, std::ostream& out);

std::vector<std::string> ReadQueries(std::istream& in);

} //end namespace io

} //end namespace transportCatalogue