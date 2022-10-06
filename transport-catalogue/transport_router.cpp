#include "transport_router.h"

static const double TIME_INITS_COEFF = 60. / 1000;

namespace transport_router {

TransportRouter::TransportRouter(const transport_catalogue::TransportCatalogue& db, RoutingSettings settings)
	:db_(db)
	, graph_(graph::DirectedWeightedGraph<Weight>(db_.GetStopCount() * 2))
	, settings_(std::move(settings)) {
	BuildRouter();
}

void TransportRouter::BuildRouter() {
	AddStopsToGraph();
	AddBusesToGraph();
	router_ = std::make_unique<Router>(graph_);
}

void TransportRouter::AddStopsToGraph() {
	const auto& stops = db_.GetStops();
	for (const auto& stop : stops) {
		VertexId wait_id = GetNextVertexId();
		VertexId route_id = GetNextVertexId();
		stop_name_to_vertexes_[stop.name] = { wait_id, route_id };
		EdgeId edge = graph_.AddEdge({ wait_id, route_id, settings_.bus_wait_time });
		edge_id_to_info_[edge] = EdgeInfo()
			.SetEdgeType(EdgeInfo::EdgeType::WAIT)
			.SetStop(&stop)
			.SetWeight(settings_.bus_wait_time);
	}
}

void TransportRouter::AddBusesToGraph() {
	const auto& buses = db_.GetBuses();
	for (const auto& bus : buses) {
		
		for (auto from = bus.stops.begin(); from != bus.stops.end(); ++from) {
			Weight weight = 0;
			auto stop_pair_from = from;
			auto stop_pair_to = std::next(from);
			for (; stop_pair_to != bus.stops.end();) {
				std::optional<int> distance = 
					db_.GetStopPairDistance((*stop_pair_from)->name, (*stop_pair_to)->name);
				if (distance.has_value()) {
					weight += ComputeWeight(*distance);
				} else {
					assert(false);
				}
				const VertexId from_id = stop_name_to_vertexes_.at((*from)->name).route_id;
				const VertexId to_id = stop_name_to_vertexes_.at((*stop_pair_to)->name).wait_id;
				EdgeId edge = graph_.AddEdge({ from_id, to_id, weight });
				edge_id_to_info_[edge] = EdgeInfo()
					.SetEdgeType(EdgeInfo::EdgeType::BUS)
					.SetBus(&bus)
					.SetSpanCount(static_cast<int>(std::distance(from, stop_pair_to)))
					.SetWeight(weight);
				++stop_pair_from;
				++stop_pair_to;
			}
		}
	}
}

std::vector<TransportRouter::EdgeInfo> TransportRouter::BuildRoute(
	const std::string_view from, const std::string_view to) const {
	std::vector<EdgeInfo> result;
	assert(from != to);
	if (!stop_name_to_vertexes_.count(from) || !stop_name_to_vertexes_.count(to)) {
		return result;
	}
	VertexId vertex_from = stop_name_to_vertexes_.at(from).wait_id;
	VertexId vertex_to = stop_name_to_vertexes_.at(to).wait_id;
	const auto raw_route = router_->BuildRoute(vertex_from, vertex_to);
	if (!raw_route.has_value()) {
		return result;
	}
	for (const auto edge_id : raw_route->edges) {
		result.push_back(edge_id_to_info_.at(edge_id));
	}
	return result;
}

TransportRouter::Weight TransportRouter::ComputeWeight(int distance) const {
	return distance / settings_.bus_velocity * TIME_INITS_COEFF;
}

TransportRouter::VertexId TransportRouter::GetNextVertexId() {
	return current_vertex_count_++;
}

}// end namespace transport_router