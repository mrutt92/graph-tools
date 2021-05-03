#include <vector>
#include <set>
#include "WGraph.hpp"
namespace graph_tools {
    class SparsePushBFS {
    public:
        using WGraph = graph_tools::WGraph;
        SparsePushBFS(const WGraph &wg,
                      const std::set<int> &frontier_in,
                      const std::set<int> &visited_in) :
            _wg(wg),
            _visited_in(visited_in),
            _frontier_in(frontier_in),
            _traversed_edges(0),
            _updates(0),
            _frontier_reads(0) {}
        
        void run() {
            // setup
            std::vector<int> frontier;
            std::vector<int> next(_wg.num_nodes(), 9);
            std::vector<int> visited(_wg.num_nodes(), 0);
            auto &neib = _wg.get_neighbors();
            auto &offs = _wg.get_offsets();
            auto &degs = _wg.get_degrees();
            
            frontier.reserve(_frontier_in.size());
            for (int v : _frontier_in)
                frontier.push_back(v);

            for (int v : _visited_in)
                visited[v] = 1;

            // run iteration
            for (int src_i = 0; src_i < frontier.size(); src_i++) {
                int src = frontier[src_i];
                _frontier_reads++;
                
                for (int dst_i = 0; dst_i < degs[src]; dst_i++) {
                    int dst = neib[offs[src]+dst_i];
                    _traversed_edges++;
                    if (visited[dst] == 0) {
                        _updates++;
                        visited[dst] = 1;
                        next[dst] = 1;
                    }
                }
            }

            for (int v = 0; v < _wg.num_nodes(); v++) {
                // build frontier_out                
                if (next[v] == 1)
                    _frontier_out.insert(v);

                // build visited_out
                if (visited[v] == 1)
                    _visited_out.insert(v);
            }
        }

        const std::set<int>& frontier_in() const { return _frontier_in; }
        const std::set<int>& visited_in() const { return _visited_in; }
        const std::set<int>& frontier_out() const { return _frontier_out; }
        const std::set<int>& visited_out() const { return _visited_out; }
        int traversed_edges() const { return _traversed_edges; }
        int update() const { return _updates; }

        static SparsePushBFS RunBFS(const WGraph &wg, int root, int iter, bool print = true)
            {
                std::set<int> frontier = {root};
                std::set<int> visited = {root};
                std::cout << std::endl << "BFS on graph with " << wg.num_nodes() << " and " << wg.num_edges() << std::endl;
                //std::cout << "graph " << std::endl << wg.to_string() << std::endl;
                
                for (int i = 0; i <= iter; i++) {
                    SparsePushBFS bfs(wg, frontier, visited);                    
                    bfs.run();

                    if (print)
                        std::cout << "frontier_out (" << i << "): ";

                    if (print)
                        for (int v : bfs.frontier_out())
                            std::cout << v << " ";

                    if (print) {
                        std::cout << std::endl;
                        std::cout << "traversed edges: " << bfs.traversed_edges() << std::endl;
                        std::cout << "updates: " << bfs.update() << std::endl;
                        std::cout << std::endl;
                    }

                    if (i == iter)
                        return bfs;
                    
                    frontier = bfs.frontier_out();
                    visited = bfs.visited_out();
                }
            }
        
        static int Test(int argc, char *argv[]) {
            RunBFS(WGraph::Uniform(10,40), 0, 0);
            RunBFS(WGraph::Uniform(10,20), 0, 1);
            RunBFS(WGraph::Generate(10, 4*1024), 1000, 3);
            return 0;
        }
        
    private:
        WGraph _wg;
        std::set<int> _visited_in;
        std::set<int> _visited_out;
        std::set<int> _frontier_in;
        std::set<int> _frontier_out;

        int _traversed_edges;
        int _updates;
        int _frontier_reads;
    };
}
