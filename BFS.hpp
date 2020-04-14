#pragma once
#include <set>
#include <memory>

namespace graph_tools {
    class BFS {
    public:
        BFS(Graph* g = nullptr) :
            _g(g),
            _traversed(0)
            {}

        Graph*& graph() { return _g; }

        void run(Graph::NodeID root, int iter, bool forward = true) {
            _visited.clear();
            _active.clear();

            _visited.insert(root);
            _active.insert(root);
            _traversed = 0;

            if (forward) {
                run_forward(root, iter);
            } else {
                run_back(root, iter);
            }
        }

        void run_forward(Graph::NodeID root, int iter) {
            int i = 0;
            while (!_active.empty() && i++ < iter) {
                std::set<Graph::NodeID> _next;
                for (auto src : _active) {
                    for (auto dst : _g->neighbors(src)) {
                        // skip visited
                        _traversed += 1;
                        if (_visited.find(dst) != _visited.end())
                            continue;
                        // update
                        _visited.insert(dst);
                        _next.insert(dst);
                    }
                }
                _active = _next;
            }
        }

        void run_back(Graph::NodeID root, int iter) {
            int i = 0;
            Graph _r = _g->transpose();
            while (!_active.empty() && i++ < iter) {
                std::set<Graph::NodeID> _next;
                for (Graph::NodeID dst = 0; dst < _g->num_nodes(); dst++) {
                    // skip visited
                    if (_visited.find(dst) != _visited.end()) break;
                    for (auto src : _r.neighbors(dst)) {
                        _traversed += 1;
                        // skip inactive
                        if (_active.find(src) == _active.end()) continue;
                        // update
                        _visited.insert(dst);
                        _next.insert(dst);
                        break;
                    }
                }
                _active = _next;
            }
        }

    public:
        std::set<Graph::NodeID> & visited() { return _visited; }
        std::set<Graph::NodeID> & active()  { return _active; }
        Graph::NodeID traversed() const { return _traversed; }

    private:
        Graph*  _g;
        std::set<Graph::NodeID> _visited;
        std::set<Graph::NodeID> _active;
        Graph::NodeID _traversed;
    };
}
