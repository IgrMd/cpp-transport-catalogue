#pragma once

#include "transport_catalogue.h"

#include <iomanip>
#include <iostream>
#include <set>
#include <string_view>

namespace transport_catalogue {

namespace io {

void PrintBusInfo(const detail::BusInfo& bus, std::ostream& out);

void PrintStopInfo(const detail::StopInfo& stop, std::ostream& out);

} //end namespace io

} //end namespace transport_catalogue