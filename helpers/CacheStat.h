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

using namespace std;

class CacheStat{
    private:
    set<IntPtr> seen_before;
    set<IntPtr> uniq_d, uniq_i, uniq_inv; 

    public:

    CacheStat(){}
    void add_addr(IntPtr addr, MemComponent::component_t pkt_type){
        if(seen_before.find(addr)!=seen_before.end()) return;

        if(pkt_type == MemComponent::component_t::L1_ICACHE){
            if(uniq_i.find(addr)!=uniq_i.end()){
                uniq_i.erase(addr);
                seen_before.insert(addr);
            }
            else uniq_i.insert(addr);
        }
        else if(pkt_type == MemComponent::component_t::L1_DCACHE){
            if(uniq_d.find(addr)!=uniq_d.end()){
                uniq_d.erase(addr);
                seen_before.insert(addr);
            }
            else uniq_d.insert(addr);
        }
        else{
            if(uniq_inv.find(addr)!=uniq_inv.end()){
                uniq_inv.erase(addr);
                seen_before.insert(addr);
            }
            else uniq_inv.insert(addr);
        }
    }

    void print(){
        string s = "llc_unique.out";
        fstream f = FILESTREAM::get_file_stream(s);
        f << "uniq_i, uniq_d\n";
        f << uniq_d.size() << ',' << uniq_i.size() << '\n';
    }


};
#endif