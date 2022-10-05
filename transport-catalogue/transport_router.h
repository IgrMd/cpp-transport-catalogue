#pragma once

#include "graph.h"
#include "router.h"
#include "transport_catalogue.h"

#include  <memory>
#include  <optional>
#include <string_view>
#include <unordered_map>

namespace transport_router {

using transport_catalogue::domain::Bus;
using transport_catalogue::domain::Stop;

struct RoutingSettings {
	double bus_wait_time = 0;
	double bus_velocity = 0;
	RoutingSettings& SetBusWaitTime(double time) {
		this->bus_wait_time = time;
		return *this;
	}
	RoutingSettings& SetBusVelocity(double velocity) {
		this->bus_velocity = velocity;
		return *this;
	}
};

class TransportRouter {
	using Weight = double;
	using VertexId = size_t;
	using EdgeId = size_t;
	using Graph = graph::DirectedWeightedGraph<Weight>;
	using Router = graph::Router<Weight>;
	
	struct StopVertexes {
		VertexId wait_id;
		VertexId route_id;
	};

public:
	struct EdgeInfo{
		enum class EdgeType {
			WAIT,
			BUS
		};
		EdgeType type;
		const Bus* bus_ptr = nullptr;
		const Stop* stop_ptr = nullptr;
		int span_count = 0;
		Weight weight = 0;

		EdgeInfo& SetEdgeType(EdgeType type) {
			this->type = type;
			return *this;
		}
		EdgeInfo& SetBus(const Bus* bus_ptr) {
			this->bus_ptr = bus_ptr;
			return *this;
		}
		EdgeInfo& SetStop(const Stop* stop_ptr) {
			this->stop_ptr = stop_ptr;
			return *this;
		}
		EdgeInfo& SetSpanCount(int span_count) {
			this->span_count = span_count;
			return *this;
		}
		EdgeInfo& SetWeight(Weight weight) {
			this->weight = weight;
			return *this;
		}
	};

public:
	TransportRouter(const transport_catalogue::TransportCatalogue& db, RoutingSettings settings);

	std::vector<EdgeInfo> BuildRoute(const std::string_view from, const std::string_view to) const;

private:
	void BuildRouter();
	void AddStopsToGraph();
	void AddBussesToGraph();

	Weight ComputeWeight(int distance) const;

	VertexId GetNextVertexId();
	
	const transport_catalogue::TransportCatalogue& db_;

	Graph graph_;
	std::unique_ptr<Router> router_;

	RoutingSettings settings_;

	std::unordered_map<std::string_view, StopVertexes> stop_name_to_vertexes_;
	std::unordered_map<EdgeId, EdgeInfo> edge_id_to_info_;

	size_t current_vertex_count_ = 0;

};

}// end namespace transport_router