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
#include <random>
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
            std::vector<std::list<float>> wadjl(num_nodes());
            WGraph t;
            for (NodeID src = 0; src < num_nodes(); src++) {
                for (NodeID dst_i = 0; dst_i < degree(src); dst_i++) {
                    NodeID dst = _neighbors[_offsets[src]+dst_i];
                    float  w   = _weights  [_offsets[src]+dst_i];
                    adjl[dst].push_back(src);
                    wadjl[dst].push_back(w);
                }
            }

            for (NodeID dst = 0; dst < num_nodes(); dst++) {
                t._offsets.push_back(t._neighbors.size());
                t._degrees.push_back(adjl[dst].size());
                t._neighbors.insert(t._neighbors.end(), adjl[dst].begin(), adjl[dst].end());
                t._weights.insert(t._weights.end(), wadjl[dst].begin(), wadjl[dst].end());
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
        std::vector<float> & get_weights()   { return _weights; }

        std::vector<std::pair<int,float>> wneighbors(NodeID v) const {
            std::vector<std::pair<int,float>> r;
            int dst_0 = _offsets[v];
            for (int dst_i = 0; dst_i < degree(v); dst_i++) {
                r.push_back({_neighbors[dst_0+dst_i], _weights[dst_0+dst_i]});
            }
            return r;
        }

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

        /**
         * Graph with uniform degree
         */
        static
        WGraph Uniform(int n_nodes, int n_edges) {
            std::vector<float> weights(n_edges);
            std::uniform_real_distribution<float> dist(0.99,1.01);
            std::uniform_int_distribution<int> idist(0, n_nodes-1);
            std::default_random_engine gen;

            for (int i = 0; i < n_edges; i++)
                weights[i] = dist(gen);

            return WGraph::FromGraph500Data(Graph500Data::Uniform(n_nodes, n_edges), &weights[0]);
        }

        /**
         * Graph with shape of linked list
         */ 
        static
        WGraph List(int n_nodes, int n_edges) {
            std::vector<float> weights(n_edges);
            std::uniform_real_distribution<float> dist(0.99,1.01);
            std::uniform_int_distribution<int> idist(0, n_nodes-1);
            std::default_random_engine gen;

            for (int i = 0; i < n_edges; i++)
                weights[i] = dist(gen);

            return WGraph::FromGraph500Data(Graph500Data::List(n_nodes, n_edges), &weights[0]);
        }

        static WGraph BalancedTree(int scale, int nedges) {
            int nnodes = 1<<scale;
            std::vector<float> weights(nedges);
            std::uniform_real_distribution<float> dist(0.99,1.01);
            std::uniform_int_distribution<int> idist(0, nnodes-1);
            std::default_random_engine gen;

            for (int i = 0; i < nedges; i++)
                weights[i] = dist(gen);

            return WGraph::FromGraph500Data(Graph500Data::BalancedTree(scale, nedges), &weights[0]);            
        }

        static WGraph FromGraph500Data(const Graph500Data &data, float *weights, bool transpose = false) {
            return WGraph::FromGraph500Buffer(data._edges, weights, data._nedges, transpose);
        }


        static WGraph FromGraph500Data(const Graph500Data &data, bool transpose = false) {
            std::vector<float> weights;
            weights.reserve(data.num_edges());
            std::uniform_real_distribution<float> dist(0.99,1.01);
            std::default_random_engine gen;

            for (int i = 0; i < data.num_edges(); i++)
                weights.push_back(dist(gen));

            return WGraph::FromGraph500Buffer(data._edges, &weights[0], data._nedges, transpose);
        }

        static WGraph Generate(int scale, int64_t nedges, bool transpose = false, uint64_t seed1 = 2, uint64_t seed2 = 3) {
            
            std::vector<float> weights;
            std::uniform_real_distribution<float> dist(0.99,1.01);
            std::default_random_engine gen;
            for (int64_t i = 0; i < nedges; i++)
                weights.push_back(dist(gen));
            
            return WGraph::FromGraph500Data(Graph500Data::Generate(scale, nedges, seed1, seed2), &weights[0], transpose);
        }
        
        std::string to_string() const {
            std::stringstream ss;
            for (NodeID v = 0; v < num_nodes(); v++) {
                ss << v << " : ";
                NodeID dst_0 = _offsets[v];
                NodeID dst_n = _degrees[v];
                for (NodeID dst_i = 0; dst_i < dst_n; dst_i++) {
                    float  w   = _weights[dst_0+dst_i];
                    NodeID dst = _neighbors[dst_0+dst_i];
                    ss << "(" << v << "," << dst << "," << w << "), ";
                }
                ss << "\n";
            }
            return ss.str();
        }

        static int Test(int argc, char* argv[]) {
            {
                WGraph wg = WGraph::List(8,8);
                std::cout << wg.to_string() << std::endl;
            }
            {
                WGraph wg = WGraph::List(32,32);
                std::cout << wg.to_string() << std::endl;
            }
            {
                WGraph wg = WGraph::List(16,8);
                std::cout << wg.to_string() << std::endl;
            }
            {
                WGraph wg = WGraph::List(8,16);
                std::cout << wg.to_string() << std::endl;
            }
            {
                WGraph wg = WGraph::List(8,8);
                std::cout << wg.to_string() << std::endl;
                std::cout << wg.transpose().to_string() << std::endl;
            }
            {
                WGraph wg = WGraph::BalancedTree(4, 4);
                std::cout << wg.to_string() << std::endl;
            }
            {
                WGraph wg = WGraph::BalancedTree(10, 254);
                std::cout << wg.to_string() << std::endl;
            }
            {
                WGraph wg = WGraph::BalancedTree(7, 126);
                std::cout << wg.to_string() << std::endl;
            }
            {
                WGraph wg = WGraph::Generate(10, 128);
                std::cout << wg.to_string() << std::endl;
                std::cout << wg.transpose().to_string() << std::endl;
            }
            {
                std::cout << "Making uniform graph |V| = 10, |E|=32" << std::endl;
                WGraph wg = WGraph::Uniform(10, 32);
                std::cout << wg.to_string() << std::endl;
                std::cout << wg.transpose().to_string() << std::endl;
            }
            {
                std::cout << "Making uniform graph |V| = 10K, |E|=32K" << std::endl;
                WGraph wg = WGraph::Uniform(10*1000, 32*1000);
                std::cout << wg.to_string() << std::endl;
                std::cout << wg.transpose().to_string() << std::endl;
            }            
            return 0;
        }
    };

}
