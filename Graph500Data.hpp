#pragma once
#include <graph_generator.h>
#include <make_graph.h>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <new>

namespace graph_tools {

    class Graph500Data {
    public:
        friend class Graph;
        Graph500Data(packed_edge *edges = nullptr, int64_t nedges = 0) :
            _edges(edges), _nedges(nedges) {}

        ~Graph500Data() {
            if (_edges != nullptr) free(_edges);
        }

        Graph500Data(const Graph500Data &other) {
            _edges = reinterpret_cast<packed_edge*>(malloc(sizeof(*_edges) * other._nedges));
            if (_edges == nullptr)
                throw std::bad_alloc();
            memcpy(_edges, other._edges, other._nedges);
            _nedges = other._nedges;
        }

        Graph500Data & operator=(const Graph500Data &other) {
            _edges = reinterpret_cast<packed_edge*>(malloc(sizeof(*_edges) * other._nedges));
            if (_edges == nullptr)
                throw std::bad_alloc();
            memcpy(_edges, other._edges, other._nedges);
            _nedges = other._nedges;
            return *this;
        }

        Graph500Data(Graph500Data &&other) {
            _edges = other._edges;
            _nedges = other._nedges;
            other._edges = nullptr;
            other._nedges = 0;
        }

        Graph500Data & operator=(Graph500Data &&other) {
            _edges = other._edges;
            _nedges = other._nedges;
            other._edges = nullptr;
            other._nedges = 0;
            return *this;
        }

        static Graph500Data Generate(int scale, int64_t nedges, uint64_t seed1 = 2, uint64_t seed2 = 3) {
            packed_edge *result; // the edge list
            int64_t rnedges; // number of edges

            // generate graph
            make_graph(scale, nedges, seed1, seed2, &rnedges, &result);

            Graph500Data data;
            data._edges = result;
            data._nedges = rnedges;

            return data;
        }

        packed_edge *begin() { return _edges; }
        packed_edge *end()   { return _edges + _nedges; }

    private:
        packed_edge * _edges;
        int64_t _nedges;

    public:

        /* Testing */
        static int Test(int argc, char *argv[]) {}

    };

}
