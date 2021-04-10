#pragma once
#include <WGraph.hpp>
#include <queue>
#include <vector>
#include <string>
#include <iostream>
#include <limits.h>

class Dijkstra {
public:
    using WGraph = graph_tools::WGraph;
    Dijkstra(const WGraph &wg, int root) :
        _wg(wg.transpose()),
        _root(root),
        _goal(-1),
        _traversed_edges(0),
        _fp_compares(0),
        _fp_adds(0) {}


    std::pair<std::vector<int>, std::vector<float>>
    run() {
        _distance.clear();
        _path.clear();
        
        _distance.resize(_wg.num_nodes(), INFINITY);
        _path.resize(_wg.num_nodes(),-1);       
        
        _distance[_root] = 0.0;
        _path[_root] = _root;

        bool converged = false;
        while (!converged) {
            converged = true;
            for (int dst = 0; dst < _wg.num_nodes(); dst++) {
                for (auto warc : _wg.wneighbors(dst)) {
                    int src = warc.first;
                    float w = warc.second;
#ifdef DEBUG_DIJKSTRA_HOST
                    printf("walking src=%d,dst=%d,w=%f\n",src,dst,w);
                    printf("distance[%d](%f)+%f < distance[%d](%f) ? %d\n",
                           src, _distance[src], w, dst, _distance[dst],
                           _distance[src]+w < _distance[dst]);
#endif
                    
                    if (_distance[src]+w < _distance[dst]) {
                        _distance[dst] = _distance[src]+w;
                        _path[dst] = src;
                        converged = false;
                    }
                    // increment count
                    _fp_compares++;
                    _fp_adds++;
                    _traversed_edges++;
                }
            }
        }
        
        return {_path, _distance};
    }

    int goal(float max_distance = INFINITY) {
        if (_goal != -1) {
            return _goal;
        } else {
            // find the maximum distance under threshold
            int maxv = _root;
            for (int v = 0; v < _wg.num_nodes(); v++) {
                if ((_distance[v] > _distance[maxv])
                    && !std::isinf(_distance[v])
                    && _distance[v] <= max_distance) {
#ifdef DEBUG_DIJKSTRA_HOST
                    printf("distance[v]=%f,distance[maxv]=%f,isinf(%f)=%d\n",
                           _distance[v], _distance[maxv], _distance[maxv], std::isinf(_distance[v]));
#endif
                    maxv = v;
                }
            }
            _goal = maxv;
            return _goal;
        }
    }
    std::vector<float> & distance() { return _distance; }
    std::vector<int>   & path() { return _path; }
    std::vector<float> distance() const { return _distance; }
    std::vector<int>   path() const { return _path; }
        
    void stats(const std::string &fname) const {
        std::ofstream of(fname);
        of << stats_str();
    }

    std::string stats_str() const {
        std::stringstream ss;
        ss << "nodes:                 " << _wg.num_nodes() << "\n";
        ss << "edges:                 " << _wg.num_edges() << "\n";
        ss << "root:                  " << _root << "\n";
        ss << "goal:                  " << _goal << "\n";
        ss << "traversed edges:       " << _traversed_edges << "\n";
        ss << "fp compares:           " << _fp_compares << "\n";
        ss << "fp adds:               " << _fp_adds << "\n";
        ss << "fp total:              " << _fp_compares+_fp_adds << "\n";
        ss << "fp total analytical:   " << 2 * (_wg.num_nodes()-1) * _wg.num_edges() << "\n";
        ss << "distance (root->goal): " << _distance[_goal] << "\n";
        return ss.str();
    }

    static int Test(int argc, char *argv[]) {
        auto wg = WGraph::Uniform(10*1000, 32*1000);
        Dijkstra dijkstra(wg, 0);
        dijkstra.run();
        dijkstra.goal(3.0);
        std::cout << "stats:" << std::endl;
        std::cout << dijkstra.stats_str() << std::endl;
        return 0;
    }
private:
    WGraph _wg;
    int    _root;
    int    _goal;
    int    _traversed_edges;
    float  _fp_compares;
    float  _fp_adds;
    std::vector<float> _distance;
    std::vector<int>   _path;
};
