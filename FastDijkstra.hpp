#pragma once
#include <WGraph.hpp>
#include <queue>
#include <vector>
#include <string>
#include <iostream>
#include <Dijkstra.hpp>

class FastDijkstra {
public:
    using WGraph = graph_tools::WGraph;
    FastDijkstra(const WGraph &wg, int root, int goal) :
        _wg(wg),
        _root(root),
        _goal(goal),
        _traversed_edges(0),
        _fp_compares(0),
        _fp_adds(0) {}


    std::pair<std::vector<int>, std::vector<float>>
    run() {
        _distance.clear();
        _path.clear();
        _teps_to_find.clear();
        
        _distance.resize(_wg.num_nodes(), INFINITY);
        _path.resize(_wg.num_nodes(),-1);
        _teps_to_find.resize(_wg.num_nodes(),-1);
            
        _distance[_root] = 0.0;
        _path[_root] = _root;
        _teps_to_find[_root] = 0;

        auto cmp = [&](int lhs, int rhs) {
            return _distance[lhs] > _distance[rhs];
        };

        std::priority_queue<int,std::vector<int>,decltype(cmp)> queue(cmp);
        queue.push(_root);

        while (!queue.empty()) {
            // approx. deletion with O(logN)
            _fp_compares += ceil(log2(queue.size()));
            int src = queue.top();
            queue.pop();
            if (src == _goal)
                break;

            auto wneib = _wg.wneighbors(src);
            for (auto &pair : wneib) {
                int dst = pair.first;
                float w = pair.second;
                if (_distance[src]+w < _distance[dst]) {
                    _path[dst] = src;
                    _distance[dst] = _distance[src]+w;
                    // approx. insertion with O(logN)
                    _fp_compares += queue.size() == 0 ? 0 : ceil(log2(queue.size()));
                    queue.push(dst);
                }

                if (_teps_to_find[dst] == -1) {
                    _teps_to_find[dst] = _traversed_edges;
                }

                _fp_adds += 1;
                _fp_compares += 1;
                _traversed_edges += 1;
            }
        }

        return {_path, _distance};
    }

    int goal() const { return _goal; }
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
        dijkstra.goal(5.0);

        FastDijkstra fdijkstra(wg, 0, dijkstra.goal());
        fdijkstra.run();
        std::cout << "stats:" << std::endl;
        std::cout << fdijkstra.stats_str() << std::endl;        
        return 0;
    }

private:
    WGraph _wg;
    int  _root;
    int  _goal;
    int  _traversed_edges;
    int  _fp_compares;
    int  _fp_adds;
    std::vector<float> _distance;
    std::vector<int>   _path;
    std::vector<int>   _teps_to_find;
};
