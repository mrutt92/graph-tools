#pragma once
#include <Graph.hpp>
#include <set>
#include <functional>
#include <cstdlib>
namespace graph_tools {

    using VertexSet = std::set<Graph::NodeID>;

    class VertexSetBuilder {
    public:
        virtual VertexSet build(const Graph & graph) = 0;
    };

    class VertexSetBuilderBase : public VertexSetBuilder {
    public:
        VertexSet build(const Graph & graph) {
            VertexSet set;
            while (!done(graph, set)) insert_next_vertex(graph, set);
            return set;
        }

        virtual void insert_next_vertex(const Graph &graph, VertexSet &set) = 0;
        virtual bool done(const Graph &graph, const VertexSet &set) const = 0;
    };

    template <typename DoneFunction>
    class RandomVertexSetBuilder final : public VertexSetBuilderBase {
    public:
        RandomVertexSetBuilder(DoneFunction done_f):
            _done_f(done_f) {
            srand(0);
        }
        void insert_next_vertex(const Graph &graph, VertexSet &set) {
            set.insert(rand()%graph.num_nodes());
        }
        bool done (const Graph& graph, const VertexSet &set ) const {
            return _done_f(graph, set);
        }

    private:
        DoneFunction _done_f;
    };

    class BFSVertexSetBuilder : public VertexSetBuilderBase {
    };
}
