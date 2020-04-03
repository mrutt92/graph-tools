#pragma once
#include <Graph500Data.hpp>
#include <cstdint>
#include <string>
#include <map>
#include <new>
#include <algorithm>
#include <fstream>
#include <list>
#include <string.h>
#include <sstream>
#include <functional>
#include <memory>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <boost/serialization/vector.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

namespace graph_tools {

    class Graph {
    public:
        using NodeID = uint32_t;
        class Neighborhood {
        public:
            Neighborhood(const NodeID *begin = nullptr, const NodeID *end = nullptr) :
                _begin(begin), _end(end) {}

            NodeID operator[](size_t i) const { return _begin[i]; }
            NodeID size() const { return _end - _begin; }
            const NodeID *begin() const { return _begin; }
            const NodeID *end()   const { return _end;   }
        private:
            const NodeID *_begin;
            const NodeID *_end;
        };

        Graph() {}
        Neighborhood neighbors(NodeID v) const {
            return Neighborhood(&_neighbors[_offsets[v]], &_neighbors[_offsets[v]]+_degrees[v]);
        }

        NodeID num_nodes() const { return _degrees.size(); }
        NodeID num_vertices() const { return num_nodes(); }
        NodeID num_edges() const { return _neighbors.size(); }
        NodeID degree(NodeID v) const { return _degrees[v]; }
        NodeID offset(NodeID v) const { return _offsets[v]; }

        Graph transpose() const {
            std::vector<std::list<NodeID>> adjl(num_nodes());
            Graph t;
            for (NodeID src = 0; src < num_nodes(); src++) {
                for (NodeID dst : neighbors(src)) {
                    adjl[dst].push_back(src);
                }
            }

            // sort each list
            for (auto & l : adjl) l.sort();

            for (NodeID dst = 0; dst < num_nodes(); dst++) {
                t._offsets.push_back(t._neighbors.size());
                t._degrees.push_back(adjl[dst].size());
                t._neighbors.insert(t._neighbors.end(), adjl[dst].begin(), adjl[dst].end());
            }

            return t;
        }

    private:
        std::vector<NodeID> _offsets;
        std::vector<NodeID> _neighbors;
        std::vector<NodeID> _degrees;
    public:
        std::vector<NodeID>& get_offsets()   { return _offsets; }
        std::vector<NodeID>& get_neighbors() { return _neighbors; }
        std::vector<NodeID>& get_degrees()   { return _degrees; }

    public:
        /* Serialization */
        template <class Archive>
        void serialize(Archive &ar, int file_version) {
            ar & _offsets;
            ar & _neighbors;
            ar & _degrees;
        }

        void toFile(const std::string & fname) {
            std::ofstream os (fname);
            boost::archive::binary_oarchive oa (os);
            oa << *this;
            return;
        }

        std::string string() const {
            std::stringstream ss;
            for (NodeID src = 0; src < num_nodes(); src++){
                ss << src << " : ";
                for (NodeID dst : neighbors(src))
                    ss << dst << ",";

                ss << "\n";
            }
            return ss.str();
        }

        static Graph FromFile(const std::string & fname) {
            std::ifstream is (fname);
            boost::archive::binary_iarchive ia (is);
            Graph g;
            ia >> g;
            return g;
        }

        /* Builder functions */
        static Graph FromGraph500Buffer(packed_edge *edges, int64_t nedges, bool transpose = false) {
            // build an adjacency list
            std::map<NodeID, std::list<NodeID>> neighbors;

            std::for_each(edges, edges+nedges,
                          [&](packed_edge & e){
                              NodeID src, dst;
                              if (!transpose) {
                                  src = static_cast<NodeID>(get_v0_from_edge(&e));
                                  dst = static_cast<NodeID>(get_v1_from_edge(&e));
                              } else {
                                  src = static_cast<NodeID>(get_v1_from_edge(&e));
                                  dst = static_cast<NodeID>(get_v0_from_edge(&e));
                              }

                              auto rslt = neighbors.find(src);
                              if (rslt == neighbors.end()) {
                                  neighbors.insert({src, {dst}});
                              } else {
                                  auto & adjl = rslt->second;
                                  adjl.push_back(dst);
                              }
                          });

            // construct edge array and offsets
            std::vector<NodeID> degree;
            std::vector<NodeID> offsets;
            std::vector<NodeID> arcs;
            NodeID maxv = 0;

            for (auto pair : neighbors) {
                NodeID src = pair.first;
                auto & adjl = pair.second;

                // sort the dst nodes
                adjl.sort();

                // fill in gaps
                while (src > static_cast<NodeID>(offsets.size())) {
                    offsets.push_back(arcs.size());
                    degree.push_back(0);
                }

                // set the current offset
                offsets.push_back(arcs.size());
                degree.push_back(adjl.size());

                // add dst nodes
                for (NodeID dst : adjl) {
                    //std::cout << src << ","  << dst << std::endl;
                    maxv = std::max(dst, maxv);
                    arcs.push_back(dst);
                }

                maxv = std::max(src, maxv);
            }

            // fill out the rest of the offset list
            while (maxv >= static_cast<NodeID>(offsets.size())) {
                offsets.push_back(arcs.size());
                degree.push_back(0);
            }

            Graph g;

            g._neighbors = std::move(arcs);
            g._offsets   = std::move(offsets);
            g._degrees   = std::move(degree);

            return g;
        }

        static Graph FromGraph500File(std::string & file_name) {
            std::vector<packed_edge> edges;
            std::ifstream ifs(file_name);

            //  get the size of the file
            ifs.seekg(0, ifs.end);
            auto sz = ifs.tellg();

            // resize so we can do a block read
            edges.resize(sz/sizeof(packed_edge));
            ifs.read(reinterpret_cast<char*>(edges.data()), sz);

            // build from the read buffer
            return Graph::FromGraph500Buffer(edges.data(), edges.size());
        }

        static Graph FromGraph500Data(const Graph500Data &data, bool transpose = false) {
            return Graph::FromGraph500Buffer(data._edges, data._nedges, transpose);
        }

        static Graph Generate(int scale, int64_t nedges, bool transpose = false, uint64_t seed1 = 2, uint64_t seed2 = 3) {
            return Graph::FromGraph500Data(Graph500Data::Generate(scale, nedges, seed1, seed2), transpose);
        }

        static Graph Tiny(bool transpose = false) {
            return Generate(6, 1<<6, transpose);
        }

        static Graph Kila(bool transpose = false) {
            return Generate(16, 16<<16, transpose);
        }

        static Graph Standard(bool transpose = false) {
            return Kila(transpose);
        }

        static int Test(int argc, char *argv[]) {
                       using namespace boost;
            using namespace archive;
            using namespace std;
            // Serialization
            {
                std::string file_name = "/tmp/g.arch";
                Graph g = Graph::Tiny();
                std::cout << "Generated graph (g) with " << g.num_nodes() << " nodes "
                          << "and " << g.num_edges() << " edges" << std::endl;

                g.toFile(file_name);
                std::cout << "Dumped (g) to " << file_name << std::endl;

                std::cout << "Reading graph (h) from " << file_name << std::endl;
                Graph h = Graph::FromFile(file_name);
                std::cout << "Read graph (h) from " << file_name << " with " << h.num_nodes() << " nodes "
                          << "and " << h.num_edges() << " edges " << std::endl;
            }
            // Transpose by static methods
            {
                Graph fwd = Graph::Tiny();
                Graph bck = Graph::Tiny(true);

                for (NodeID src = 0; src < fwd.num_nodes(); src++) {
                    for (NodeID dst : fwd.neighbors(src)) {
                        // check that src is a neighbor of dst in the transpose graph
                        auto sources = bck.neighbors(dst);
                        // assert that we can find src in sources
                        assert(std::find(sources.begin(), sources.end(), src) != sources.end());
                    }
                }
            }
            // Transpose by member function
            {
                Graph fwd = Graph::Tiny();
                Graph bck = fwd.transpose();
                for (NodeID src = 0; src < fwd.num_nodes(); src++) {
                    for (NodeID dst : fwd.neighbors(src)) {
                        // check that src is a neighbor of dst in the transpose graph
                        auto sources = bck.neighbors(dst);
                        // assert that we can find src in sources
                        assert(std::find(sources.begin(), sources.end(), src) != sources.end());
                    }
                }
            }

            return 0;
        }
    };

}
