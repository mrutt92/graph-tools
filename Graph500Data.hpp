#pragma once
#include <graph_generator.h>
#include <make_graph.h>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <stdexcept>
#include <new>
#include <vector>
#include <sys/stat.h>
#include <assert.h>
#include <fstream>
namespace graph_tools {

    class Graph500Data {
    public:
        friend class Graph;
        friend class WGraph;
        Graph500Data(packed_edge *edges = NULL, int64_t nedges = 0) :
            _edges(edges), _nedges(nedges) {}

        ~Graph500Data() {
            if (_edges != NULL) free(_edges);
        }

        Graph500Data(const Graph500Data &other) {
            _edges = reinterpret_cast<packed_edge*>(malloc(sizeof(*_edges) * other._nedges));
            if (_edges == NULL)
                throw std::bad_alloc();
            memcpy(_edges, other._edges, other._nedges);
            _nedges = other._nedges;
        }

        Graph500Data & operator=(const Graph500Data &other) {
            _edges = reinterpret_cast<packed_edge*>(malloc(sizeof(*_edges) * other._nedges));
            if (_edges == NULL)
                throw std::bad_alloc();
            memcpy(_edges, other._edges, other._nedges);
            _nedges = other._nedges;
            return *this;
        }

        Graph500Data(Graph500Data &&other) {
            _edges = other._edges;
            _nedges = other._nedges;
            other._edges = NULL;
            other._nedges = 0;
        }

        Graph500Data & operator=(Graph500Data &&other) {
            _edges = other._edges;
            _nedges = other._nedges;
            other._edges = NULL;
            other._nedges = 0;
            return *this;
        }

        void toFile(const std::string & file_name) {
            std::ofstream ofs(file_name);
            ofs.write(reinterpret_cast<char*>(_edges), sizeof(*_edges) * _nedges);
        }

        static Graph500Data FromASCIIFile(const std::string & file_name) {
            struct stat st;
            int err;

            // stat the file
            if ((err = stat(file_name.c_str(), &st)) != 0) {
                std::string errm(strerror(errno));
                throw std::runtime_error("Failed to stat '"
                                         + file_name
                                         + "': "
                                         + errm);
            }

            // open using stdio
            FILE *f = fopen(file_name.c_str(), "rb");
            if (f == NULL) {
                std::string errm(strerror(errno));
                throw std::runtime_error("Failed to open '"
                                         + file_name
                                         + "': "
                                         + errm);
            }

            // read each line
            std::vector<packed_edge> edges;
            int64_t v0, v1;
            while (fscanf(f, "%" SCNd64 " %" SCNd64 "", &v0, &v1) == 2) {
                packed_edge e;
                write_edge(&e, v0, v1);
                edges.push_back(e);
            }

            packed_edge *e = (packed_edge*)malloc(sizeof(*e) * edges.size());
            if (!e)
                throw std::bad_alloc();

            memcpy(e, &edges[0], sizeof(*e) * edges.size());
            Graph500Data data(e, edges.size());
            return data;
        }

        static Graph500Data FromFile(const std::string & file_name) {
            struct stat st;
            int err;

            // stat the file
            if ((err = stat(file_name.c_str(), &st)) != 0) {
                std::string errm(strerror(errno));
                throw std::runtime_error("Failed to stat '"
                                         + file_name
                                         + "': "
                                         + errm);
            }

            // open using stdio
            FILE *f = fopen(file_name.c_str(), "rb");
            if (f == NULL) {
                std::string errm(strerror(errno));
                throw std::runtime_error("Failed to open '"
                                         + file_name
                                         + "': "
                                         + errm);
            }

            // allocate a buffer with malloc
            packed_edge *edge = reinterpret_cast<packed_edge*>(malloc(st.st_size));
            if (edge == NULL) throw std::bad_alloc();

            // read edge list
            fread(edge, st.st_size, 1, f);

            int64_t nedges = st.st_size/sizeof(*edge);
            assert(st.st_size % sizeof(*edge) == 0);

            fclose(f);

            return Graph500Data(edge, nedges);
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

        int64_t num_edges() const { return _nedges; }

    private:
        packed_edge * _edges;
        int64_t _nedges;

    public:

        /* Testing */
        static int Test(int argc, char *argv[]) { return 0; }

    };

}
