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
#include <algorithm>
#include "mmio.h"

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

        static std::pair<Graph500Data, float*>
        FromMatrixMarketFile(const std::string & file_name) {
            // open the file
            MM_typecode matcode;
            int err;
            FILE *f = fopen(file_name.c_str(), "r");
            if (f == NULL) {
                std::string errm(strerror(errno));
                throw std::runtime_error("Failed to open '"
                                         + file_name
                                         + "': "
                                         + errm);
            }
            // use mm header to read
            err = mm_read_banner(f, &matcode);
            if (err != 0) {
                throw std::runtime_error("Failed to read MM banner");
            }
            // coordinate or array?
            if (!mm_is_sparse(matcode)) {
                throw std::runtime_error(
                    "Only sparse inputs"
                    );
            }
            // read the meta data
            int M, N, nz;
            err = mm_read_mtx_crd_size(f, &M, &N, &nz);
            if (err != 0) {
                throw std::runtime_error(
                    "Early end of file '"
                    + file_name
                    + "'");
            }
            // allocate arrays
            Graph500Data g500;
            int64_t nedges = (mm_is_symmetric(matcode) ? 2*nz : nz);
            packed_edge *edges = (packed_edge*)malloc(nedges * sizeof(packed_edge));
            float *weights = (float*)malloc(nedges * sizeof(float));
            
            int e = 0;
            for (; e < nz; e++) {
                // scan each line
                int count;
                int i, j;
                float  d;
                if (mm_is_real(matcode)) {
                    count = fscanf(f, "%d %d %f", &i, &j, &d);
                    if (count != 3) {
                        throw std::runtime_error(
                            "Unexpected end-of-file: '"
                            + file_name
                            + "'"
                            );
                    }
                } else if (mm_is_pattern(matcode)) {
                    count = fscanf(f, "%d %d", &i, &j);
                    if (count != 2) {
                        throw std::runtime_error(
                            "Unexpected end-of-file: '"
                            + file_name
                            + "'"
                            );
                    }
                } else {
                    throw std::runtime_error(
                        "Only pattern and real matrices supported"
                        );
                }
                // matrix market files are 1-indexed
                i -= 1;
                j -= 1;
                write_edge(&edges[e], i, j);
                weights[e] = d;
            }

            // make reverse copy of each edge
            // if symmetric
            if (mm_is_symmetric(matcode)) {
                e = 0;
                for (; e < nz; e++) {
                    // get from start of the array
                    int64_t i, j;
                    float d;
                    i = get_v0_from_edge(&edges[e]);
                    j = get_v1_from_edge(&edges[e]);
                    d = weights[e];
                    // set new edge
                    write_edge(&edges[nz+e], j, i);
                    weights[nz+e] = d;
                }
            }
            // set g500
            g500._edges = edges;
            g500._nedges = nedges;
            g500._nodes = nodes;
            return {g500, weights};
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
            int64_t n = 0;
            std::vector<packed_edge> edges;
            int64_t v0, v1;
            while (fscanf(f, "%" SCNd64 " %" SCNd64 "", &v0, &v1) == 2) {
                n = std::max(n, v0+1);
                n = std::max(n, v1+1);
                packed_edge e;
                write_edge(&e, v0, v1);
                edges.push_back(e);
            }

            packed_edge *e = (packed_edge*)malloc(sizeof(*e) * edges.size());
            if (!e)
                throw std::bad_alloc();

            memcpy(e, &edges[0], sizeof(*e) * edges.size());
            Graph500Data data(e, edges.size());
            data._nodes = n;
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

            int64_t n = 0;
            for (int e = 0; e < nedges; e++) {
                n = std::max(n, get_v0_from_edge(&edge[e]));
                n = std::max(n, get_v1_from_edge(&edge[e]));
            }

            Graph500Data gdata(edge, nedges);
            gdata._nodes = n;
            return gdata;
        }

        static Graph500Data Generate(int scale, int64_t nedges, uint64_t seed1 = 2, uint64_t seed2 = 3) {
            packed_edge *result; // the edge list
            int64_t rnedges; // number of edges

            // generate graph
            make_graph(scale, nedges, seed1, seed2, &rnedges, &result);

            Graph500Data data;
            data._edges = result;
            data._nedges = rnedges;
            data._nodes = 1<<scale;
            return data;
        }

        static Graph500Data Uniform(int n_nodes, int n_edges) {
            packed_edge *edges = reinterpret_cast<packed_edge*>(malloc(sizeof(packed_edge)*n_edges));
            std::vector<int> nodes;

            nodes.reserve(n_nodes);

            for (int i = 0; i < n_nodes; i++)
                nodes.push_back(i);

            std::random_shuffle(nodes.begin(), nodes.end());

            int degree = n_edges/n_nodes;
            int rem = n_edges%n_nodes;

            int e_i = 0;
            int dst_i = 0;
            for (int src = 0; src < n_nodes; src++){
                for (int e = 0; e < degree; e++) {
                    write_edge(&edges[e_i], src, nodes[(dst_i++%n_nodes)]);
                    e_i++;
                }
            }

            // fill out the remainder
            for (int e = 0; e < rem; e++) {
                int src = nodes[dst_i++%n_nodes];
                int dst = nodes[dst_i++%n_nodes];
                write_edge(&edges[e_i], src, dst);
                e_i++;
            }

            Graph500Data data;
            data._edges = edges;
            data._nedges = n_edges;
            data._nodes = n_nodes;
            return data;
        }

        static Graph500Data List(int n_nodes, int n_edges) {
            packed_edge *edges = reinterpret_cast<packed_edge*>(malloc(sizeof(packed_edge)*n_edges));
            std::vector<int> nodes;
            nodes.reserve(n_nodes);

            for (int v_i = 0 ; v_i < n_nodes; v_i++) {
                nodes.push_back(v_i);
            }

            // make chain
            for (int e_i = 0; e_i < n_edges; e_i++) {
                write_edge(&edges[e_i], nodes[e_i%n_nodes], nodes[(e_i+1)%n_nodes]);
            }

            Graph500Data data;
            data._edges = edges;
            data._nedges = n_edges;
            data._nodes = n_nodes;
            return data;
        }

        static Graph500Data BalancedTree(int scale, int nedges) {
            std::vector<int> nodes;
            packed_edge *edges = reinterpret_cast<packed_edge*>(malloc(sizeof(packed_edge)*nedges));

            //int64_t nedges = (1<<scale)-2;
            int nnodes = 1<<scale;

            nodes.reserve(nnodes);

            for (int i = 0; i < (1<<scale); i++)
                nodes.push_back(i);

            // shuffle - keep 0 in place
            std::random_shuffle(nodes.begin()+1, nodes.end());

            for (int e_i = 0; e_i < nedges; e_i++) {
                write_edge(&edges[e_i], nodes[(e_i/2) % nnodes], nodes[(e_i+1) % nnodes]);
            }

            Graph500Data data;
            data,_nodes = nnodes;
            data._edges = edges;
            data._nedges = nedges;
            return data;
        }


        packed_edge *begin() { return _edges; }
        packed_edge *end()   { return _edges + _nedges; }

        int64_t num_edges() const { return _nedges; }
        int64_t num_nodes() const { return _nodes; }

    private:
        packed_edge * _edges;
        int64_t _nedges;
        int64_t _nodes;

    public:

        /* Testing */
        static int Test(int argc, char *argv[]) { return 0; }

    };

}
