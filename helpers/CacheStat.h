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
    vector<IntPtr> seen_before, uniq_i, uniq_d;

    public:

    CacheStat(){}

    void add_addr(IntPtr addr, MemComponent::component_t pkt_type){
        for(auto e: seen_before){
            if(e == addr){
                return;
            }
        }

        if(pkt_type == MemComponent::component_t::L1_ICACHE){
            for(int i=0; i< uniq_i.size(); i++){
                if(uniq_i[i] == addr)
                {
                    uniq_i.erase(uniq_i.begin() + i);
                    seen_before.push_back(addr);
                    return;
                }
            }
        }

        if(pkt_type == MemComponent::component_t::L1_DCACHE){
            for(int i=0; i< uniq_d.size(); i++){
                if(uniq_d[i] == addr)
                {
                    uniq_d.erase(uniq_d.begin()+i);
                    seen_before.push_back(addr);
                }
            }
        }
    }

    void print(){
        string s = "llc_unique.out";
        fstream f = FILESTREAM::get_file_stream(s);
        f << "uniq_i, uniq_d\n";
    }


};
#endif