#include "input_reader.h"

namespace transport_catalogue {

using namespace std::literals;
using detail::QueryType;

namespace detail {

const std::string_view DIGITS = "0123456789"sv;

void CutOff(std::string_view& str) {
	while (str.front() == ' ' || str.front() == '-' || str.front() == '>') {
		str.remove_prefix(1);
	}
	while (str.back() == ' ' || str.back() == '-' || str.back() == '>') {
		str.remove_suffix(1);
	}
}

std::string_view FirstCommand(std::string_view str) {
	size_t end = str.find(' ');
	assert(end != std::string::npos);
	return str.substr(0, end);
}

std::string_view SecondCommand(std::string_view str) {
	size_t begin = str.find_first_of(' ') + 1;
	assert(begin != std::string::npos);
	size_t end = str.find(':');
	end = (end == std::string::npos) ? str.size() : end + 1;
	return str.substr(begin, end - begin);
}

QueryType GetQueryType(const std::string& query) {
	std::string_view first_command = FirstCommand(query);
	std::string_view second_command = SecondCommand(query);
	if (first_command == "Stop"sv) {
		return (second_command.back() == ':') ? QueryType::AddStop : QueryType::PrintStop;
	}
	if (first_command == "Bus"sv) {
		return (second_command.back() == ':') ? QueryType::AddBus : QueryType::PrintBus;
	}
	return QueryType::INCORRECT;
}

AddBusQuery ParseBusQuery(const std::string_view str) {
	AddBusQuery query;
	query.name = SecondCommand(str);
	query.name.remove_suffix(1);
	size_t stop_name_begin = str.find(':') + 1;
	size_t stop_name_end = str.find_first_of("->", stop_name_begin);
	assert(stop_name_end != std::string::npos);
	query.is_circle = str[stop_name_end] == '>';
	std::string_view stop;
	while (stop_name_end != std::string::npos) {
		stop = str.substr(stop_name_begin, stop_name_end - stop_name_begin);
		CutOff(stop);
		query.stops.push_back(stop);
		stop_name_begin = stop_name_end + 1;
		stop_name_end = str.find_first_of("->", stop_name_begin);
	}
	stop = str.substr(stop_name_begin, str.size() - stop_name_begin);
	CutOff(stop);
	query.stops.push_back(stop);
	return query;
}

AddStopQuery ParseStopQuery(const std::string_view str) {
	AddStopQuery query;
	query.name = SecondCommand(str);
	query.name.remove_suffix(1);
	size_t coord_begin = str.find(':') + 2;
	size_t coord_end = str.find(',', coord_begin);
	assert(coord_end != std::string::npos);
	query.latitude = std::stod(std::string{ str.substr(coord_begin, coord_end - coord_begin) });
	coord_begin = str.find_first_of(DIGITS, coord_end);
	assert(coord_begin != std::string::npos);
	coord_end = str.find(',', coord_begin);
	bool is_end = coord_end == std::string::npos;
	coord_end = (is_end) ? str.size() : coord_end;
	query.longitude = std::stod(std::string{ str.substr(coord_begin, coord_end - coord_begin) });
	if (is_end) { return query; }

	size_t distance_begin, distance_end, name_begin, name_end;
	name_end = coord_end;
	do {
		distance_begin = str.find_first_of(DIGITS, name_end);
		distance_end = str.find('m', distance_begin);
		assert(distance_end != std::string::npos);
		int distance = std::stoi(std::string{ str.substr(distance_begin, distance_begin - distance_end) });
		name_begin = str.find("to"sv, distance_end);
		assert(name_begin != std::string::npos);
		name_begin += 3;
		name_end = str.find(',', name_begin);
		is_end = name_end == std::string::npos;
		std::string_view name_to = str.substr(name_begin,
			(is_end ? str.size() : name_end) - name_begin
		);
		query.name_to_dist[name_to] = distance;
	} while (!is_end);
	return query;
}

std::string ReadLine(std::istream& in) {
	std::string s;
	std::getline(in, s);
	return s;
}

int ReadLineWithNumber(std::istream& in) {
	int result;
	in >> result;
	ReadLine(in);
	return result;
}

} //end namespace detail

namespace io {

std::vector<std::string> ReadQueries(std::istream& in) {
	const int count = detail::ReadLineWithNumber(in);
	std::vector<std::string> queries;
	queries.reserve(count);
	for (int i = 0; i < count; ++i) {
		queries.push_back(std::move(detail::ReadLine(in)));
	}
	return queries;
}

void ProcessQueries(std::vector<std::string>& queries, Catalogue& catalogue, std::ostream& out) {
	std::vector<detail::AddStopQuery> add_stop_queries;
	std::vector<detail::AddBusQuery> add_bus_queries;
	std::vector<detail::PrintQuery> print_queries;

	for (const std::string& query : queries) {
		QueryType type = detail::GetQueryType(query);
		switch (type) {
		case QueryType::INCORRECT:
			assert(false);
			break;
		case QueryType::AddStop:
			add_stop_queries.push_back(std::move(detail::ParseStopQuery(query)));
			break;
		case QueryType::AddBus:
			add_bus_queries.push_back(std::move(detail::ParseBusQuery(query)));
			break;
		case QueryType::PrintStop:
			print_queries.push_back({ QueryType::PrintStop, detail::SecondCommand(query) });
			break;
		case QueryType::PrintBus:
			print_queries.push_back({ QueryType::PrintBus,detail::SecondCommand(query) });
			break;
		default:
			assert(false);
			break;
		}

	}
	for (const auto& add_stop_query : add_stop_queries) {
		catalogue.AddStop(add_stop_query.name, add_stop_query.latitude, add_stop_query.longitude);
	}
	for (const auto& add_stop_query : add_stop_queries) {
		catalogue.SetStopDistances(add_stop_query.name, add_stop_query.name_to_dist);
	}
	for (const auto& add_bus_query : add_bus_queries) {
		catalogue.AddBus(add_bus_query.name, add_bus_query.stops, add_bus_query.is_circle);
	}
	for (const auto& print_query : print_queries) {
		switch (print_query.type) {
		case QueryType::PrintStop:
			PrintStopInfo(catalogue.GetStopInfo(print_query.name), out);
			break;
		case QueryType::PrintBus:
			PrintBusInfo(catalogue.GetBusInfo(print_query.name), out);
			break;
		default:
			assert(false);
			break;
		}
	}
}

} //end namespace io

} //end namespace transport_catalogue