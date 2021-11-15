#include <vector>
#include <set>
#include <sstream>
#include <fstream>
#include <memory>
#include "WGraph.hpp"
#include "SSSPBase.hpp"

namespace graph_tools {
    class SparsePushSSSP : public SSSPBase {
    public:
        using WGraph = SSSPBase::WGraph;
        SparsePushSSSP(): SSSPBase() {}
        SparsePushSSSP(
            const std::shared_ptr<WGraph> &wg
            ,const std::shared_ptr<std::set<int>> &frontier_in
            ,const std::shared_ptr<std::map<int,int>> &parent_in
            ,const std::shared_ptr<std::map<int,float>> &distance_in
            )
            : SSSPBase(
                wg
                ,frontier_in
                ,parent_in
                ,distance_in
                ) {
        }

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

            frontier.reserve(frontier_in().size());
            for (int v : frontier_in())
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


        static std::vector<SparsePushSSSP> RunSSSP(const WGraph &wg, int root, int iter, bool print = true)
            {
                return SSSPBase::RunSSSP<SparsePushSSSP>(wg, root, iter, print);
            }

        static SparsePushSSSP RunSSSP_single(const WGraph &wg, int root, int iter, bool print = true)
            {
                return SSSPBase::RunSSSP_single<SparsePushSSSP>(wg, root, iter, print);
            }
        static std::vector<SparsePushSSSP> RunSSSP_until_empty(const WGraph &wg, int root,  bool print = true)
            {
                return SSSPBase::RunSSSP_until_empty<SparsePushSSSP>(wg, root, print);
            }

        template <typename AfterIteration>
        static SparsePushSSSP RunSSSP_single_until_empty(
            const WGraph &wg
            , int root
            , AfterIteration after
            ) {
            return SSSPBase::RunSSSP_single_until_empty<SparsePushSSSP, AfterIteration>(
                wg
                , root
                , after
                );
        }

        static int Test(int argc, char *argv[]) {
            RunSSSP(WGraph::Uniform(10,40), 0, 0);
            RunSSSP(WGraph::Uniform(10,20), 0, 1);
            RunSSSP(WGraph::Generate(10, 4*1024), 1000, 3);
            return 0;
        }
    };
}
