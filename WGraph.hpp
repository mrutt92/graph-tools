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

#define FILENAME_SZ 1024
#define PARA_SZ 16

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

        ///////////////////////
        // Builder Functions //
        ///////////////////////
        static WGraph FromGraph500Buffer(
            packed_edge *edges
            ,float *edge_weights
            ,int64_t nedges
            ,int64_t nodes
            ,bool transpose = false
            ) {
            // construct edge array and offsets
            std::vector<NodeID> degree (nodes+1, 0);
            std::vector<NodeID> offsets (nodes+1, 0);
            std::vector<std::pair<NodeID,float>> tmp_arcs (nedges);
            std::vector<NodeID> arcs (nedges);
            std::vector<float>  weights (nedges);

            // degrees
            std::vector<NodeID> pos(nedges, 0);
            for (int e = 0; e < nedges; e++) {
                int mjr, mnr;
                if (transpose) {
                    mjr = get_v1_from_edge(&edges[e]);
                    mnr = get_v0_from_edge(&edges[e]);
                } else {
                    mjr = get_v0_from_edge(&edges[e]);
                    mnr = get_v1_from_edge(&edges[e]);
                }
                pos[e] = degree[mjr]++;                
            }

            // offsets
            int sum = 0;
            for (int v = 0; v < offsets.size(); v++) {
                offsets[v] = sum;
                sum += degree[v];
            }

            // arcs + weights
            for (int e = 0; e < nedges; e++) {
                int mjr, mnr;
                if (transpose) {
                    mjr = get_v1_from_edge(&edges[e]);
                    mnr = get_v0_from_edge(&edges[e]);
                } else {
                    mjr = get_v0_from_edge(&edges[e]);
                    mnr = get_v1_from_edge(&edges[e]);
                }
                tmp_arcs[offsets[mjr] + pos[e]] = { mnr, edge_weights[e] };
            }

            // sort
            for (int v = 0; v < nodes; v++) {
                int begin = offsets[v];
                int end = begin + degree[v];
                std::sort(tmp_arcs.begin()+begin, tmp_arcs.begin()+end,
                          [=](const std::pair<NodeID,float> &lhs, const std::pair<NodeID,float> &rhs){
                              return lhs.first < rhs.first;
                          });
            }

            // copy
            for (int e = 0; e < nedges; e++) {
                arcs[e] = tmp_arcs[e].first;
                weights[e] = tmp_arcs[e].second;
            }
            
            WGraph g;
            g._neighbors = std::move(arcs);
            g._offsets   = std::move(offsets);
            g._degrees   = std::move(degree);
            g._weights   = std::move(weights);
            
            return g;
        }

        /* Build from CSR TXT Files */
        // helper function
        static void read_file_float(char* filename, float* buffer, char* mat_sz_m,char* mat_sz_n,char* mat_sz_nnz, char* density_str) {
          FILE* file = fopen(filename, "r");
          char* line_buf;
          size_t line_buf_size = 0;
          ssize_t line_size;
          int line_cnt = 0;
          // check whether the file canbe open
          if (!file) {
            printf("Can't open file: '%s'", filename);
          }
          // get the first line 
          line_size = getline(&line_buf, &line_buf_size, file);
          while (line_size >= 0) {
            if (line_cnt == 0) {
              for (int i = 0; i < PARA_SZ; i++) {
                mat_sz_m[i] = line_buf[i];
              }
            }
            else if (line_cnt == 1) {
              for (int i = 0; i < PARA_SZ; i++) {
                mat_sz_n[i] = line_buf[i];
              }
            }
            else if (line_cnt == 3) {
              for (int i = 0; i < PARA_SZ; i++) {
                density_str[i] = line_buf[i];
                //printf("current lincnt is %d, while line buf is: %s",line_cnt,line_buf);
              }
            }
            else if (line_cnt == 4) {
              for (int i = 0; i < PARA_SZ; i++) {
                mat_sz_nnz[i] = line_buf[i];
              }
            }
            buffer[line_cnt] = atof(line_buf);
            line_cnt++;
            /* Show the line details */
            //printf("line[%06d]: chars=%06zd, buf size=%06zu, contents: %s", line_cnt, line_size, line_buf_size, line_buf);

            /* Get the next line */
            line_size = getline(&line_buf, &line_buf_size, file);
          }

          free(line_buf);
          line_buf = NULL;  
          // close the file
          fclose(file);
        }

        static void read_file_int(char* filename, int* buffer) {
          FILE* file = fopen(filename, "r");
          char* line_buf;
          size_t line_buf_size = 0;
          ssize_t line_size;
          int line_cnt = 0;
          // check whether the file canbe open
          if (!file) {
            printf("Can't open file: '%s'", filename);
          }
          // get the first line 
          line_size = getline(&line_buf, &line_buf_size, file);
          while (line_size >= 0) {
            buffer[line_cnt] = atoi(line_buf);
            line_cnt++;
            /* Show the line details */
            //printf("line[%06d]: chars=%06zd, buf size=%06zu, contents: %s", line_cnt, line_size, line_buf_size, line_buf);

            /* Get the next line */
            line_size = getline(&line_buf, &line_buf_size, file);
          }

          free(line_buf);
          line_buf = NULL;  
          // close the file
          fclose(file);
        }

        


        static WGraph FromCSR(const std::string & file_name,const std::string & file_path, int ite, int num_pods) {
        const char* filename_base ;
        filename_base = file_path.c_str();
        char filename_info[FILENAME_SZ];
        // concat the string
        strcpy(filename_info, filename_base);
        strcat(filename_info, "spm_tb_info.dat");
        float tb_info[5];
        char  mat_m_str[PARA_SZ];
        char  mat_n_str[PARA_SZ];
        char  mat_nnz_str[PARA_SZ];
        char  density_str[PARA_SZ];
        read_file_float(filename_info, tb_info, mat_m_str,mat_n_str,mat_nnz_str, density_str);
        // parameters
	    int mat_sz = 1024;
	    int m = atoi(mat_m_str);
	    int n = atoi(mat_n_str);
        int A_len  = atoi(mat_nnz_str);
        //for (int i = 0; i < 5; i++) {
        //    printf("tb_info[%d]: %f \n", i, tb_info[i]);
        //}
        
        // allocate memory for graph
        int* A_idx = (int*) malloc (A_len*sizeof(int));
        int* A_ptr = (int*) malloc ((m+1)*sizeof(int));
        
        
        char filename_b[FILENAME_SZ];
        strcpy(filename_b, filename_base);
        strcat(filename_b, file_name.c_str());
        
        //printf("Test benchmark: %s", filename_b);
        //printf("Reading A_val...............................................................\n");
        char f_A_idx[FILENAME_SZ];
        strcpy(f_A_idx, filename_b);
        strcat(f_A_idx, "_A_idx.dat");
        read_file_int(f_A_idx, A_idx);

        char f_A_ptr[FILENAME_SZ];
        strcpy(f_A_ptr, filename_b);
        strcat(f_A_ptr, "_A_ptr.dat");
        read_file_int(f_A_ptr, A_ptr);

        std::vector<NodeID> degree;
        std::vector<NodeID> neighbors;
        std::vector<NodeID> offsets;
        NodeID offset_cnt = 0;
        for(int i=ite; i<m; i += num_pods){
            degree.push_back(A_ptr[i+1]-A_ptr[i]);
            for(int j=A_ptr[i]; j<A_ptr[i+1]; j++){
                neighbors.push_back(A_idx[j]);   
            }
            offsets.push_back(offset_cnt);
            offset_cnt += A_ptr[i+1]-A_ptr[i];
        }


        
        //printf("++++++++++++++++++++++++++++size of degree: %d",degree.size());

        WGraph g;
        g._neighbors = std::move(neighbors);
        g._offsets   = std::move(offsets);
        g._degrees   = std::move(degree);

        free(A_idx);
        free(A_ptr);

        std::vector<float> weights(offset_cnt);
        std::uniform_real_distribution<float> dist(0.99,1.01);
        std::uniform_int_distribution<int> idist(0, m-1);
        std::default_random_engine gen;

        for (int i = 0; i < offset_cnt; i++)
            weights[i] = dist(gen);

        g._weights = std::move(weights);
        
        return g;
        
        }  

        static WGraph FromCSC(const std::string & file_name,const std::string & file_path, int ite, int num_pods) {
        const char* filename_base ;
        filename_base = file_path.c_str();
        char filename_info[FILENAME_SZ];
        // concat the string
        strcpy(filename_info, filename_base);
        strcat(filename_info, "spm_tb_info.dat");
        float tb_info[5];
        char  mat_m_str[PARA_SZ];
        char  mat_n_str[PARA_SZ];
        char  mat_nnz_str[PARA_SZ];
        char  density_str[PARA_SZ];
        read_file_float(filename_info, tb_info, mat_m_str,mat_n_str,mat_nnz_str, density_str);
        // parameters
	    int mat_sz = 1024;
	    int m = atoi(mat_m_str);
	    int n = atoi(mat_n_str);
        int A_len  = atoi(mat_nnz_str);
        //for (int i = 0; i < 5; i++) {
        //    printf("tb_info[%d]: %f \n", i, tb_info[i]);
        //}
        
        // allocate memory for graph
        int* A_idx = (int*) malloc (A_len*sizeof(int));
        int* A_ptr = (int*) malloc ((n+1)*sizeof(int));
    
        
        char filename_b[FILENAME_SZ];
        strcpy(filename_b, filename_base);
        strcat(filename_b, file_name.c_str());
        
        //printf("Test benchmark: %s", filename_b);
        //printf("Reading A_val...............................................................\n");
        char f_A_idx[FILENAME_SZ];
        strcpy(f_A_idx, filename_b);
        strcat(f_A_idx, "_A_idx.dat");
        read_file_int(f_A_idx, A_idx);

        char f_A_ptr[FILENAME_SZ];
        strcpy(f_A_ptr, filename_b);
        strcat(f_A_ptr, "_A_ptr.dat");
        read_file_int(f_A_ptr, A_ptr);

        std::vector<NodeID> degree;
        std::vector<NodeID> neighbors;
        std::vector<NodeID> offsets;
        NodeID offset_cnt = 0;
        for(int i=0; i<n; i ++){
            NodeID degree_col = 0;
            for(int j=A_ptr[i]; j<A_ptr[i+1]; j++){
                if(A_idx[j]%num_pods==ite){
                    neighbors.push_back(A_idx[j]/num_pods); 
                    degree_col ++;   
                }
            }
            degree.push_back(degree_col);
            offsets.push_back(offset_cnt);
            offset_cnt += degree_col;
        }
        //number of nodes will be n!


        
        //printf("++++++++++++++++++++++++++++size of degree: %d",degree.size());

        WGraph g;
        g._neighbors = std::move(neighbors);
        g._offsets   = std::move(offsets);
        g._degrees   = std::move(degree);

        free(A_idx);
        free(A_ptr);

        std::vector<float> weights(offset_cnt);
        std::uniform_real_distribution<float> dist(0.99,1.01);
        std::uniform_int_distribution<int> idist(0, m-1);
        std::default_random_engine gen;

        for (int i = 0; i < offset_cnt; i++)
            weights[i] = dist(gen);

        g._weights = std::move(weights);
        
        return g;
        
        }  

        static WGraph FromCSR(const std::string & file_name,const std::string & file_path) {
        const char* filename_base ;
        filename_base = file_path.c_str();
        char filename_info[FILENAME_SZ];
        // concat the string
        strcpy(filename_info, filename_base);
        strcat(filename_info, "spm_tb_info.dat");
        float tb_info[5];
        char  mat_m_str[PARA_SZ];
        char  mat_n_str[PARA_SZ];
        char  mat_nnz_str[PARA_SZ];
        char  density_str[PARA_SZ];
        read_file_float(filename_info, tb_info, mat_m_str,mat_n_str,mat_nnz_str, density_str);
        // parameters
        int mat_sz = 1024;
        int m = atoi(mat_m_str);
        int n = atoi(mat_n_str);
        int A_len  = atoi(mat_nnz_str);
        //for (int i = 0; i < 5; i++) {
        //    printf("tb_info[%d]: %f \n", i, tb_info[i]);
        //}
        
        // allocate memory for graph
        int* A_idx = (int*) malloc (A_len*sizeof(int));
        int* A_ptr = (int*) malloc ((m+1)*sizeof(int));


        
        
        char filename_b[FILENAME_SZ];
        strcpy(filename_b, filename_base);
        strcat(filename_b, file_name.c_str());
        
        //printf("Test benchmark: %s", filename_b);
        //printf("Reading A_val...............................................................\n");
        char f_A_idx[FILENAME_SZ];
        strcpy(f_A_idx, filename_b);
        strcat(f_A_idx, "_A_idx.dat");
        read_file_int(f_A_idx, A_idx);

        char f_A_ptr[FILENAME_SZ];
        strcpy(f_A_ptr, filename_b);
        strcat(f_A_ptr, "_A_ptr.dat");
        read_file_int(f_A_ptr, A_ptr);


        std::vector<NodeID> neighbors;
        for(int i=0; i<A_len; i++){
            neighbors.push_back(A_idx[i]);
        }

        std::vector<NodeID> offsets;
        for(int i=0; i<m; i++){
            offsets.push_back(A_ptr[i]);
        }

        std::vector<NodeID> degree;
        for(int i=0; i<m; i++){
            degree.push_back(A_ptr[i+1]-A_ptr[i]);
        }
        printf("++++++++++++++++++++++++++++size of degree: %d",degree.size());

        WGraph g;
        g._neighbors = std::move(neighbors);
        g._offsets   = std::move(offsets);
        g._degrees   = std::move(degree);

        free(A_idx);
        free(A_ptr);

        std::vector<float> weights(A_len);
        std::uniform_real_distribution<float> dist(0.99,1.01);
        std::uniform_int_distribution<int> idist(0, m-1);
        std::default_random_engine gen;

        for (int i = 0; i < A_len; i++)
            weights[i] = dist(gen);

        g._weights = std::move(weights);
        
        return g;
        
        
        
        }   

        static WGraph C2SR(const std::string & file_name,const std::string & file_path) {
        const char* filename_base ;
        filename_base = file_path.c_str();
        char filename_info[FILENAME_SZ];
        // concat the string
        strcpy(filename_info, filename_base);
        strcat(filename_info, "spm_tb_info.dat");
        float tb_info[5];
        char  mat_m_str[PARA_SZ];
        char  mat_n_str[PARA_SZ];
        char  mat_nnz_str[PARA_SZ];
        char  density_str[PARA_SZ];
        read_file_float(filename_info, tb_info, mat_m_str,mat_n_str,mat_nnz_str, density_str);
        // parameters
	    int mat_sz = 1024;
	    int m = atoi(mat_m_str);
	    int n = atoi(mat_n_str);
        int A_len  = atoi(mat_nnz_str);
        //for (int i = 0; i < 5; i++) {
        //    printf("tb_info[%d]: %f \n", i, tb_info[i]);
        //}
        
        // allocate memory for graph
        int* A_idx = (int*) malloc (A_len*sizeof(int));
        int* A_ptr = (int*) malloc ((m+1)*sizeof(int));


        
        
        char filename_b[FILENAME_SZ];
        strcpy(filename_b, filename_base);
        strcat(filename_b, file_name.c_str());
        
        //printf("Test benchmark: %s", filename_b);
        //printf("Reading A_val...............................................................\n");
        char f_A_idx[FILENAME_SZ];
        strcpy(f_A_idx, filename_b);
        strcat(f_A_idx, "_A_idx.dat");
        read_file_int(f_A_idx, A_idx);

        char f_A_ptr[FILENAME_SZ];
        strcpy(f_A_ptr, filename_b);
        strcat(f_A_ptr, "_A_ptr.dat");
        read_file_int(f_A_ptr, A_ptr);


        //set 32 vectors to sort neighbours into 32 banks
        std::vector<NodeID> bank_contents[32];
        //set 32 variables to record the ptr of each row in the bank_contents
        int local_ptr[32];
        for(int i=0; i<32; i++){
            local_ptr[i] = 0;
        }
        //declare the offset vector
        std::vector<NodeID> offsets;
        std::vector<NodeID> degree;
        for(int i=0; i<m; i++){
            for(int j=A_ptr[i]; j<A_ptr[i+1]; j++){
                bank_contents[i%32].push_back(A_idx[j]);
                //if(i==829558){
                //    std::cout<<"======================= node 829558's edge pushed: "<<A_idx[j]<<"============================"<<std::endl;
                //}
            }
            
            //update offset after store one row    
            offsets.push_back((i%32)*32+(local_ptr[i%32]/32)*1024+(local_ptr[i%32]%32));
            //if(i==829558){
                //std::cout<<"======================= node 829558's offset pushed: "<<offsets.back()<<"============================"<<std::endl;
                //std::cout<<"======================= node 829558's local ptr: "<<local_ptr[i%32]<<"============================"<<std::endl;
            //}
            local_ptr[i%32] += A_ptr[i+1]- A_ptr[i]; 
            degree.push_back(A_ptr[i+1]-A_ptr[i]);     
        }
        
        std::vector<NodeID> neighbors;
        //construct the neighbors vector
        uint32_t remain = 0;
        while(1){
            //iterate over 32 bank_content vectors
            for(int i=0; i<32; i++){
                //iterate over 32 words per bank
                for(int j=0; j<32; j++){
                    if(bank_contents[i].size()>0){
                        neighbors.push_back(bank_contents[i].front());
                        bank_contents[i].erase(bank_contents[i].begin());
                    }
                    else{
                        neighbors.push_back(0); 
                    }
                }
            }
            for(int j=0; j<32; j++){
                remain += bank_contents[j].size();
            }
            if(remain == 0)
                break;
            else
                remain = 0;
        }
        
        //debug
        /*int ptr_829558 = offsets[829558];
        std::cout<<"======================= node 829558's offset: "<<ptr_829558<<"============================"<<std::endl;
        int degree_829558 = degree[829558];
        std::cout<<"======================= node 829558's degree: "<<degree_829558<<"============================"<<std::endl;
        for(int i=0; i<degree_829558; i++){
            std::cout<<"======================= node 829558's edge "<<i<<" is: "<<neighbors[ptr_829558]<<"offset is "<<ptr_829558<<"============================"<<std::endl;    
            ptr_829558 += 1;
            if(ptr_829558%32==0){
                ptr_829558 += 992;
            }
        }*/




        WGraph g;
        g._neighbors = std::move(neighbors);
        g._offsets   = std::move(offsets);
        g._degrees   = std::move(degree);

        free(A_idx);
        free(A_ptr);

        std::vector<float> weights(A_len);
        std::uniform_real_distribution<float> dist(0.99,1.01);
        std::uniform_int_distribution<int> idist(0, m-1);
        std::default_random_engine gen;

        for (int i = 0; i < A_len; i++)
            weights[i] = dist(gen);

        g._weights = std::move(weights);
        
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
#if 0
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
#endif
        static WGraph FromGraph500Data(const Graph500Data &data, float *weights, bool transpose = false) {
            return WGraph::FromGraph500Buffer(data._edges, weights, data._nedges, data._nodes, transpose);
        }


        static WGraph FromGraph500Data(const Graph500Data &data, bool transpose = false) {
            std::vector<float> weights;
            weights.reserve(data.num_edges());
            std::uniform_real_distribution<float> dist(0.99,1.01);
            std::default_random_engine gen;

            for (int i = 0; i < data.num_edges(); i++)
                weights.push_back(dist(gen));

            return WGraph::FromGraph500Buffer(data._edges, &weights[0], data._nedges, data._nodes, transpose);
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
#if 0
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
#endif
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
