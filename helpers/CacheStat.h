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
    map<IntPtr, set<UInt32>> seen_before_i;
    map<IntPtr, set<UInt32>> seen_before_d;
    map<IntPtr, set<UInt32>> uniq_i;
    map<IntPtr, set<UInt32>> uniq_d;

    public:

    CacheStat(){}
    void add_addr(IntPtr tag, UInt32 set, MemComponent::component_t pkt_type){
        
        if(pkt_type == MemComponent::component_t::L1_ICACHE){

            auto seenTag = seen_before_i.find(tag);
            if(seenTag!=seen_before_i.end())
            {
                auto &sec = seenTag->second;
                if(sec.find(set)!=sec.end()){
                    //tag+set seen before
                    return;
                }
            }

            seenTag = uniq_i.find(tag);
            if(seenTag!=uniq_i.end()){
                auto &sec = seenTag->second;
                if(sec.find(set)!=sec.end()){
                    // erase
                    sec.erase(set);
                    seen_before_i[tag].insert(set);
                }
            }
            else uniq_i[tag].insert(set);
        }
        else if(pkt_type == MemComponent::component_t::L1_DCACHE){
            
            auto seenTag = seen_before_d.find(tag);
            if(seenTag!=seen_before_d.end())
            {
                auto &sec = seenTag->second;
                if(sec.find(set)!=sec.end()){
                    //tag+set seen before
                    return;
                }
            }

            seenTag = uniq_d.find(tag);
            if(seenTag!=uniq_d.end()){
                auto &sec = seenTag->second;
                if(sec.find(set)!=sec.end()){
                    // erase
                    sec.erase(set);
                    seen_before_d[tag].insert(set);
                }
            }
            else uniq_d[tag].insert(set);
        }
        else{
            assert(0);
        }
    }

    void print(){
        string s = "llc_unique.out";
        fstream f = FILESTREAM::get_file_stream(s);
        f << "uniq_i, uniq_d\n";
        int total_i=0;
        int total_d=0;
        for(auto tag: uniq_i){
            total_i += tag.second.size();
        }
        for(auto tag: uniq_d){
            total_d += tag.second.size();
        }
        f << total_i << "," << total_d << '\n';
    }


};
#endif