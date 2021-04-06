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

namespace graph_tools {

    class WGraph {
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

        WGraph() {}
        Neighborhood neighbors(NodeID v) const {
            return Neighborhood(&_neighbors[_offsets[v]], &_neighbors[_offsets[v]]+_degrees[v]);
        }

        NodeID num_nodes() const { return _degrees.size(); }
        NodeID num_vertices() const { return num_nodes(); }
        NodeID num_edges() const { return _neighbors.size(); }
        NodeID degree(NodeID v) const { return _degrees[v]; }
        NodeID offset(NodeID v) const { return _offsets[v]; }

        NodeID node_with_max_degree() const {
            NodeID max = 0;
            for (NodeID v = 0; v < num_nodes(); v++) {
                if (degree(v) > degree(max))
                    max = v;
            }
            return max;
        }

        NodeID node_with_degree(NodeID target) const {
            for (NodeID v = 0; v < num_nodes(); v++) {
                if (degree(v) == target)
                    return v;
            }
            return 0;
        }

        NodeID node_with_avg_degree() const {
            return node_with_degree(avg_degree());
        }

        NodeID avg_degree() const {
            return static_cast<NodeID>(static_cast<double>(num_edges())/num_nodes());
        }

        WGraph transpose() const {
            std::vector<std::list<NodeID>> adjl(num_nodes());
            WGraph t;
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
        std::vector<float>  _weights;
    public:
        std::vector<NodeID>& get_offsets()   { return _offsets; }
        std::vector<NodeID>& get_neighbors() { return _neighbors; }
        std::vector<NodeID>& get_degrees()   { return _degrees; }

    public:
#if 0
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

        static WGraph FromFile(const std::string & fname) {
            std::ifstream is (fname);
            boost::archive::binary_iarchive ia (is);
            WGraph g;
            ia >> g;
            return g;
        }

#endif
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

        /* Builder functions */
        static WGraph FromGraph500Buffer(packed_edge *edges, float *edge_weights, int64_t nedges, bool transpose = false) {
            // build an adjacency list
            std::map<NodeID, std::list<std::pair<NodeID, float>>> neighbors;
            for (int64_t i = 0; i < nedges; i++) {
                packed_edge &e = edges[i];
                float w = edge_weights[i];
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
                    neighbors.insert({src, {{dst, w}}});
                } else {
                    auto & adjl = rslt->second;
                    adjl.push_back({dst,w});
                }
            }
            
            // construct edge array and offsets
            std::vector<NodeID> degree;
            std::vector<NodeID> offsets;
            std::vector<NodeID> arcs;
            std::vector<float>  weights;
            NodeID maxv = 0;

            for (auto pair : neighbors) {
                NodeID src = pair.first;
                auto & adjl = pair.second;

                // sort the dst nodes
                //adjl.sort();

                // fill in gaps
                while (src > static_cast<NodeID>(offsets.size())) {
                    offsets.push_back(arcs.size());
                    degree.push_back(0);
                }

                // set the current offset
                offsets.push_back(arcs.size());
                degree.push_back(adjl.size());

                // add dst nodes
                for (std::pair<NodeID,float> e : adjl) {
                    NodeID dst = e.first;
                    float  w = e.second;
                    //std::cout << src << ","  << dst << std::endl;
                    maxv = std::max(dst, maxv);
                    arcs.push_back(dst);
                    weights.push_back(w);
                }

                maxv = std::max(src, maxv);
            }

            // fill out the rest of the offset list
            while (maxv >= static_cast<NodeID>(offsets.size())) {
                offsets.push_back(arcs.size());
                degree.push_back(0);
            }

            WGraph g;

            g._neighbors = std::move(arcs);
            g._offsets   = std::move(offsets);
            g._degrees   = std::move(degree);
            g._weights   = std::move(weights);
            
            return g;
        }

        static
        WGraph List(int n_nodes, int n_edges) {
            packed_edge *edges = new packed_edge [n_edges];
            float *weights = new float [n_edges];
            std::vector<NodeID> nodes;
            nodes.reserve(n_nodes);

            for (int v_i = 0 ; v_i < n_nodes; v_i++)
                nodes.push_back(v_i);

            // the root is always 0
            std::random_shuffle(nodes.begin()+1,nodes.end());
            std::uniform_real_distribution<float> dist(0.0,0.1);
            std::default_random_engine generator;
            
            // pick the first n to be the chain
            write_edge(&edges[0], nodes[0], nodes[1]);
            weights[0] = dist(generator);

            // make chain
            for (int e_i = 1; e_i < n_edges; e_i++) {
                write_edge(&edges[e_i], nodes[e_i], nodes[e_i+1]);            
                weights[e_i] = dist(generator);
            }

            delete [] edges;
            delete [] weights;
            return WGraph::FromGraph500FromBuffer(edges, weights, n_edges);
        }
    };

}
