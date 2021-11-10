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
