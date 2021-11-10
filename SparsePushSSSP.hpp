#include <vector>
#include <set>
#include <sstream>
#include <fstream>
#include <memory>
#include "WGraph.hpp"

namespace graph_tools {
    class SparsePushSSSP {
    public:
        using WGraph = graph_tools::WGraph;
        SparsePushSSSP() :
            _wg(nullptr),
            _traversed_edges(0),
            _updates(0),
            _frontier_reads(0) {}

        SparsePushSSSP(const std::shared_ptr<WGraph> &wg,
                       const std::set<int> &frontier_in,
                       const std::map<int,int> &parent_in,
                       const std::map<int,float> &distance_in) :
            _wg(wg),
            _distance_in(distance_in),
            _parent_in(parent_in),
            _frontier_in(frontier_in),
            _traversed_edges(0),
            _updates(0),
            _frontier_reads(0) {}

        void run() {
            // setup
            std::vector<int> frontier;
            std::vector<int> next(_wg->num_nodes(), 0);
            std::vector<float> distance(_wg->num_nodes(), INFINITY);
            std::vector<int> parent(_wg->num_nodes(), -1);

            auto &neib = _wg->get_neighbors();
            auto &weig = _wg->get_weights();
            auto &offs = _wg->get_offsets();
            auto &degs = _wg->get_degrees();

            frontier.reserve(_frontier_in.size());
            for (int v : _frontier_in)
                frontier.push_back(v);

            for (auto kv : distance_in()) {
                distance[kv.first] = kv.second;
            }

            for (auto kv : parent_in()) {
                parent[kv.first] = kv.second;
            }

            // run iteration
            for (int src_i = 0; src_i < frontier.size(); src_i++) {
                int src = frontier[src_i];
                _frontier_reads++;

                for (int dst_i = 0; dst_i < degs[src]; dst_i++) {
                    int dst = neib[offs[src]+dst_i];
                    float w = weig[offs[src]+dst_i];
                    _traversed_edges++;
                    // update
                    float old_dist = distance[dst];
                    float new_dist = distance[src]+w;
                    if (new_dist < old_dist) {
                        _updates++;
                        distance[dst] = new_dist;
                        parent[dst] = src;
                        next[dst] = 1;
                    }
                }
            }

            for (int v = 0; v < _wg->num_nodes(); v++) {
                // build frontier_out
                if (next[v] == 1) {
                    frontier_out().insert(v);
                }
                if (parent[v] != -1) {
                    distance_out().insert({v, distance[v]});
                    parent_out().insert({v, parent[v]});
                }
            }
        }

        const std::map<int,int>& parent_in() const { return _parent_in; }
        std::map<int,int>& parent_in() { return _parent_in; }

        const std::map<int,int>& parent_out() const { return _parent_out; }
        std::map<int,int>& parent_out() { return _parent_out; }

        const std::map<int,float>& distance_out() const { return _distance_out; }
        std::map<int,float>& distance_out() { return _distance_out; }

        const std::map<int,float>& distance_in() const { return _distance_in; }
        std::map<int,float>& distance_in() { return _distance_in; }

        const std::set<int>& frontier_in() const { return _frontier_in; }
        std::set<int> & frontier_in() { return _frontier_in; }

        const std::set<int>& frontier_out() const { return _frontier_out; }
        std::set<int>& frontier_out() { return _frontier_out; }

        std::string report() const {
            std::stringstream ss;
            ss << "input frontier size:  " << _frontier_in.size() << "\n";
            ss << "output frontier size: " << _frontier_out.size() << "\n";
            ss << "traversed_edges:      " << traversed_edges() << "\n";
            ss << "updates:              " << updates() << "\n";
            return ss.str();
        }

        void dump(const std::string &fname) {
            std::ofstream f(fname);
            f << report();
        }

        int traversed_edges() const { return _traversed_edges; }
        int updates() const { return _updates; }

        static std::vector<SparsePushSSSP> RunSSSP(const WGraph &wg, int root, int iter, bool print = true)
            {
                std::shared_ptr<WGraph> wgptr = std::shared_ptr<WGraph>(new WGraph(wg));
                std::set<int> frontier = {root};
                std::map<int,int> parent = {{root, root}};
                std::map<int,float> distance = {{root, 0}};

                std::cout << std::endl << "SSSP on graph with " << wgptr->num_nodes() << " and " << wgptr->num_edges() << std::endl;
                //std::cout << "graph " << std::endl << wg.to_string() << std::endl;

                std::vector<SparsePushSSSP> sssp_runs;
                for (int i = 0; i <= iter; i++) {
                    SparsePushSSSP sssp = SparsePushSSSP(wgptr, frontier, parent, distance);
                    sssp.run();

                    if (print)
                        std::cout << "frontier_out (" << i << "): ";

                    if (print)
                        for (int v : sssp.frontier_out())
                            std::cout << v << " ";

                    if (print) {
                        std::cout << std::endl;
                        std::cout << "traversed edges: " << sssp.traversed_edges() << std::endl;
                        std::cout << "updates: " << sssp.updates() << std::endl;
                        std::cout << std::endl;
                    }

                    sssp_runs.push_back(sssp);

                    frontier = sssp.frontier_out();
                    parent = sssp.parent_out();
                    distance = sssp.distance_out();
                }

                return sssp_runs;
            }

            static SparsePushSSSP RunSSSP_single(const WGraph &wg, int root, int iter, bool print = true)
            {
                std::shared_ptr<WGraph> wgptr = std::shared_ptr<WGraph>(new WGraph(wg));
                std::set<int> frontier = {root};
                std::map<int,int> parent = {{root,root}};
                std::map<int,float> distance = {{root, 0}};

                std::cout << std::endl << "SSSP on graph with " << wgptr->num_nodes() << " and " << wgptr->num_edges() << std::endl;
                //std::cout << "graph " << std::endl << wg.to_string() << std::endl;

                SparsePushSSSP sssp_result;
                for (int i = 0; i <= iter; i++) {
                    SparsePushSSSP sssp = SparsePushSSSP(wgptr, frontier, parent, distance);
                    sssp.run();

                    if (print)
                        std::cout << "frontier_out (" << i << "): ";

                    if (print)
                        for (int v : sssp.frontier_out())
                            std::cout << v << " ";

                    if (print) {
                        std::cout << std::endl;
                        std::cout << "traversed edges: " << sssp.traversed_edges() << std::endl;
                        std::cout << "updates: " << sssp.updates() << std::endl;
                        std::cout << std::endl;
                    }

                    frontier = sssp.frontier_out();
                    parent = sssp.parent_out();
                    distance = sssp.distance_out();
                    sssp_result = sssp;
                }

                return sssp_result;
            }

        static int Test(int argc, char *argv[]) {
            RunSSSP(WGraph::Uniform(10,40), 0, 0);
            RunSSSP(WGraph::Uniform(10,20), 0, 1);
            RunSSSP(WGraph::Generate(10, 4*1024), 1000, 3);
            return 0;
        }

    private:
        std::shared_ptr<WGraph> _wg;
        std::map<int,float> _distance_in;
        std::map<int,float> _distance_out;
        std::map<int,int>   _parent_in;
        std::map<int,int>   _parent_out;
        std::set<int> _frontier_in;
        std::set<int> _frontier_out;

        int _traversed_edges;
        int _updates;
        int _frontier_reads;
    };
}
