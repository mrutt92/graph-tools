#include "WGraph.hpp"
#include <memory>
#include <sstream>
#include <fstream>
#include <vector>
#include <set>
#include <map>

namespace graph_tools {
    class SSSPBase {
    public:
        using WGraph = graph_tools::WGraph;

    public:
        /**
         * Constructors
         */
        SSSPBase()
            :_wg(nullptr)
            ,_traversed_edges(0)
            ,_updates(0)
            ,_frontier_reads(0) {
        }

        SSSPBase(
            const std::shared_ptr<WGraph> &wg
            ,const std::shared_ptr<std::set<int>> &frontier_in
            ,const std::shared_ptr<std::map<int,int>> &parent_in
            ,const std::shared_ptr<std::map<int,float>> &distance_in
            )
            :_wg(wg)
             // inputs
            ,_distance_in(distance_in)
            ,_parent_in(parent_in)
            ,_frontier_in(frontier_in)
             // outputs
            ,_distance_out(new std::map<int,float>)
            ,_parent_out(new std::map<int,int>)
            ,_frontier_out(new std::set<int>)
             // stats
            ,_traversed_edges(0)
            ,_updates(0)
            ,_frontier_reads(0) {
        }

        /**
         * parent_in()
         */
        std::shared_ptr<const std::map<int,int>>
        parent_in_ptr() const {
            return std::const_pointer_cast<const std::map<int,int>>(_parent_in);
        }
        std::shared_ptr<std::map<int,int>>
        parent_in_ptr() {
            return _parent_in;
        }
        const std::map<int,int>&
        parent_in() const {
            return *_parent_in;
        }
        std::map<int,int>&
        parent_in() {
            return *_parent_in;
        }
        /**
         * parent_out()
         */
        std::shared_ptr<const std::map<int,int>>
        parent_out_ptr() const {
            return std::const_pointer_cast<const std::map<int,int>>(_parent_out);
        }
        std::shared_ptr<std::map<int,int>>
        parent_out_ptr() {
            return _parent_out;
        }
        const std::map<int,int>&
        parent_out() const {
            return *_parent_out;
        }
        std::map<int,int>&
        parent_out() {
            return *_parent_out;
        }
        /**
         * distance_out()
         */
        std::shared_ptr<const std::map<int,float>>
        distance_out_ptr() const {
            return std::const_pointer_cast<const std::map<int,float>>(_distance_out);
        }
        std::shared_ptr<std::map<int,float>>
        distance_out_ptr() {
            return _distance_out;
        }

        const std::map<int,float>&
        distance_out() const {
            return *_distance_out;
        }
        std::map<int,float>&
        distance_out() {
            return *_distance_out;
        }
        /**
         * distance_in()
         */
        const std::map<int,float>&
        distance_in() const {
            return *_distance_in;
        }
        std::map<int,float>&
        distance_in() {
            return *_distance_in;
        }
        /**
         * frontier_in()
         */
        const std::set<int>&
        frontier_in() const {
            return *_frontier_in;
        }
        std::set<int> &
        frontier_in() {
            return *_frontier_in;
        }
        /**
         * frontier_out()
         */
        std::shared_ptr<const std::set<int>>
        frontier_out_ptr() const {
            return std::const_pointer_cast<const std::set<int>>(_frontier_out);
        }
        std::shared_ptr<std::set<int>>
        frontier_out_ptr() {
            return _frontier_out;
        }
        const std::set<int>&
        frontier_out() const {
            return *_frontier_out;
        }
        std::set<int>&
        frontier_out() {
            return *_frontier_out;
        }

       /**
        * Stats reporting
        */
        std::string report() const {
            std::stringstream ss;
            ss << "input frontier size:  " << frontier_in().size() << "\n";
            ss << "output frontier size: " << frontier_out().size() << "\n";
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
        int frontier_reads () const { return _frontier_reads; }

        /**
         * Run functions
         */
    public:
        template <typename SSSPType>
        static std::vector<SSSPType> RunSSSP(const WGraph &wg, int root, int iter, bool print = true)
            {
                std::shared_ptr<WGraph> wgptr = std::shared_ptr<WGraph>(new WGraph(wg));
                std::shared_ptr<std::set<int>> frontier(new std::set<int>({root}));
                std::shared_ptr<std::map<int,int>> parent(new std::map<int,int>({{root, root}}));
                std::shared_ptr<std::map<int,float>> distance(new std::map<int,float>({{root, 0}}));

                std::cout << std::endl
                          << "SSSP on graph with " << wgptr->num_nodes() << " vertices"
                          << " and " << wgptr->num_edges() << " edges" << std::endl;
                //std::cout << "graph " << std::endl << wg.to_string() << std::endl;

                std::vector<SSSPType> sssp_runs;
                for (int i = 0; i <= iter; i++) {
                    SSSPType sssp = SSSPType(wgptr, frontier, parent, distance);
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

                    frontier = sssp.frontier_out_ptr();
                    parent = sssp.parent_out_ptr();
                    distance = sssp.distance_out_ptr();
                }

                return sssp_runs;
            }

        template <typename SSSPType>
        static SSSPType RunSSSP_single(const WGraph &wg, int root, int iter, bool print = true)
            {
                std::shared_ptr<WGraph> wgptr = std::shared_ptr<WGraph>(new WGraph(wg));
                std::shared_ptr<std::set<int>> frontier(new std::set<int>({root}));
                std::shared_ptr<std::map<int,int>> parent(new std::map<int,int>({{root,root}}));
                std::shared_ptr<std::map<int,float>> distance(new std::map<int,float>({{root, 0}}));

                std::cout << std::endl
                          << "SSSP on graph with " << wgptr->num_nodes() << " nodes "
                          << " and " << wgptr->num_edges() << " edges" << std::endl;

                SSSPType sssp_result;
                for (int i = 0; i <= iter; i++) {
                    SSSPType sssp = SSSPType(wgptr, frontier, parent, distance);
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

                    frontier = sssp.frontier_out_ptr();
                    parent = sssp.parent_out_ptr();
                    distance = sssp.distance_out_ptr();
                    sssp_result = sssp;
                }

                return sssp_result;
            }
    protected:
        int & traversed_edges() { return _traversed_edges; }
        int & updates()  { return _updates; }
        int & frontier_reads () { return _frontier_reads; }

        /**
         * Members
         */
    protected:
        // inputs
        std::shared_ptr<WGraph> _wg;
        std::shared_ptr<std::map<int,float>> _distance_in;
        std::shared_ptr<std::map<int,int>>   _parent_in;
        std::shared_ptr<std::set<int>>       _frontier_in;

        // outputs
        std::shared_ptr<std::map<int,float>> _distance_out;
        std::shared_ptr<std::map<int,int>>   _parent_out;
        std::shared_ptr<std::set<int>>       _frontier_out;

        // stats
        int _traversed_edges;
        int _updates;
        int _frontier_reads;
    };
}
