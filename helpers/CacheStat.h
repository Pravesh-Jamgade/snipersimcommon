#ifndef CACHE_STAT
#define CACHE_STAT
#include <iostream>
#include <map>
#include <vector>
#include <set>
#include <string>
#include <algorithm>
#include "inttypes.h"
#include "fixed_types.h"
#include "mem_component.h"
#include "utility.h"
#include "fixed_types.h"

using namespace std;

class CacheStat{
    private:
    vector<IntPtr> seen_before_i, seen_before_d, uniq_i, uniq_d;

    public:

    CacheStat(){}

    void add_addr(IntPtr addr, MemComponent::component_t pkt_type){
        

        if(pkt_type == MemComponent::component_t::L1_ICACHE){
            // seen before return
            auto findU = std::find(seen_before_i.begin(), seen_before_i.end(), addr);
            if(findU!=seen_before_d.end()){
                return;
            }
           
            auto findK = std::find(uniq_i.begin(), uniq_i.end(), addr);
            if(findK!=uniq_i.end()){
                uniq_i.erase(findK);
                seen_before_i.push_back(addr);
                return;
            }
            else uniq_i.push_back(addr);
        }

        if(pkt_type == MemComponent::component_t::L1_DCACHE){
            
            auto findU = std::find(seen_before_d.begin(), seen_before_d.end(), addr);
            if(findU!=seen_before_d.end()){
                return;
            }
            
            auto findK = std::find(uniq_d.begin(), uniq_d.end(), addr);
            if(findK!=uniq_d.end()){
                uniq_d.erase(findK);
                seen_before_d.push_back(addr);
                return;
            }
            else uniq_d.push_back(addr);
        }
    }

    void print(){
        string s = "llc_unique.out";
        fstream f = FILESTREAM::get_file_stream(s);
        f << "uniq_i, uniq_d, repeated_i, repeated_d\n";
        f << uniq_i.size() << "," << uniq_d.size() << "," << seen_before_i.size() << "," << seen_before_d.size() << '\n';
    }


};
#endif