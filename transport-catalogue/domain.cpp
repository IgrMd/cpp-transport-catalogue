#include "domain.h"


namespace transport_catalogue {

namespace domain {


bool StopPair::operator==(const StopPair& other) const {
	return this->from == other.from && this->to == other.to;
}

size_t StopPairHasher::operator()(const StopPair& stop_pair) const {
	return std::hash<std::string_view>{}(stop_pair.from->name) +
		std::hash<std::string_view>{}(stop_pair.to->name) * 43;
}

}//end namespace domain

}//end namespace transport_catalogue
