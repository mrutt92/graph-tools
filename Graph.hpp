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

    private:
        std::vector<NodeID> _offsets;
        std::vector<NodeID> _neighbors;
        std::vector<NodeID> _degrees;
    public:
        std::vector<NodeID>& get_offsets()   { return _offsets; }
        std::vector<NodeID>& get_neighbors() { return _neighbors; }
        std::vector<NodeID>& get_degrees()   { return _degrees; }

    public:
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
            return Graph::FromGraph500Data(Graph500Data::Generate(scale, nedges, seed1, seed2));
        }

        static int Test(int argc, char *argv[]);
    };

}
