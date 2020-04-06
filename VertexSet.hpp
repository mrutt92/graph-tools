#pragma once
#include <Graph.hpp>
#include <set>
#include <functional>
#include <cstdlib>
namespace graph_tools {

    using VertexSet = std::set<Graph::NodeID>;

    class BFSVertexSetBuilder {
    public:
        BFSVertexSetBuilder() {}
        void build(const Graph & graph, Graph::NodeID root = 0) {
            _next.clear();
            _visited.clear();
            _frontier.clear();

            _visited.insert(root);
            _frontier.insert(root);

            while (!done(graph)) {
                for (Graph::NodeID src : _frontier) {
                    for (Graph::NodeID dst : graph.neighbors(src)) {
                        if (_visited.find(dst) != _visited.end()) continue;
                        _visited.insert(dst);
                        _next.insert(dst);
                    }
                }
                _frontier.clear();
                _frontier = std::move(_next);
            }
        }
        VertexSet & get_active()  { return _frontier; }
        VertexSet & get_visited() { return _visited; }
        virtual bool done(const Graph & graph) { return _frontier.empty(); }
    protected:
        VertexSet _visited;
        VertexSet _frontier;
        VertexSet _next;
    };

    class EdgeThresholdBFSVertexSetBuilder : public BFSVertexSetBuilder {
    public:
        EdgeThresholdBFSVertexSetBuilder(double threshold = 0.05) :
            _threshold(threshold),
            BFSVertexSetBuilder()  {
        }

        virtual bool done(const Graph &graph) {
            Graph::NodeID sum = 0;
            for (Graph::NodeID src : _frontier) {
                sum += graph.degree(src);
            }
            return _frontier.empty() || sum > graph.num_edges() * _threshold;
        }

    private:
        double _threshold;
    };
}
