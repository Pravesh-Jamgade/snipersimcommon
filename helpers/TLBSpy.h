#ifndef TLB_SPY_H
#define TLB_SPY_H
#define fi first
#define se second
#include "fixed_types.h"
#include <bits/stdc++.h>
#include <bitset>
#include "constant.h"
#include "log.h"

using namespace std;

#define MACRO_LOG_PAGE_SIZE 12
#define MACRO_LOG_BLOCK_SIZE_BITS 6
#define MACRO_PAGE_SIZE 4095
#define KEY_SIZE 4


class PageTracker{

    public:

    int counter_64_access = 64;

    // sets block type by calculation diff between memory access during queuing
    vector<ARRAY_TYPE> blockType;

    // calculating array confidence for each page
    vector<int> array_type_confidence;

    // counting if array's were called k times
    int k_times_confidence;
    
    // replacement bits for pages
    int replacement_bit;

    // setting page type based on  array type
    int page_type[2]={0};

    // count access
    vector<int> count_access;

    /*HELPERS*/
    map<UInt64, int> offsets;
    
    int key1[KEY_SIZE] = {2,5,11,13};
    int key2[KEY_SIZE] = {7,17,19,21};

    bool key1_seen[KEY_SIZE] = {false};
    bool key2_seen[KEY_SIZE] = {false};

    int ikey,jkey;

    PageTracker()
    {
        blockType = vector<ARRAY_TYPE>(64, ARRAY_TYPE::NONE);
        array_type_confidence = vector<int>(4, 0);
        replacement_bit = 3;
        count_access = vector<int>(64,0);
        ikey=jkey=0;
    }

    void set_type(UInt64 diff, UInt64 mod, ARRAY_TYPE arr_type=ARRAY_TYPE::NONE){ 
        // blockType[mod] = arr_type; 
        
        bool key1_found=false;
        bool key2_found=false;
        bool finished_matching = false;

        // key(x)_seen[xkey] should be 'false' as this is what we want to get proper sequence 
        if(key1[ikey] == diff && !key1_seen[ikey])
        {
            key1_found=true;
            key1_seen[ikey]=true;
            ikey++;
        }
        if(ikey >= KEY_SIZE) finished_matching=true;

        if(key2[jkey] == diff && !key2_seen[jkey])
        {
            key2_found=true;
            key2_seen[jkey]=true;
            jkey++;
        }
        if(jkey >= KEY_SIZE) finished_matching=true;

        //* implies there is a scope to match our deliberate deltas for respective arrays.
        //* if complete key array key(x)_seen is done, then increase count of seen stride pattern
        //  for respective pattern.
        //* reset key(x)_seen.

        if(finished_matching){
            
            // check if other key also did matched partially.
            // if so then reset it as for this page key1 pattern is win.
            if(jkey>0 && jkey<KEY_SIZE && key1_found) 
            {
                memset(&key1_seen, false, 4);
                page_type[0]++;
                ikey=0; 
            }

            if(ikey>0 && ikey<KEY_SIZE && key2_found)
            {
                memset(&key2_seen, false, 4);
                page_type[1]++;
                jkey=0;
            } 
        }
    } 

    ARRAY_TYPE get_array_type_found(){
        
        array_type_confidence = vector<int>(4,0);
        for(auto e : blockType)
        {
            array_type_confidence[(int)e]++;
        }

        int mnX = 0;
        int index = 0;
        for(int i=0; i< array_type_confidence.size(); i++){
            if(mnX < array_type_confidence[i]){
                mnX = array_type_confidence[i];
                index = i;
            }
        }
        return static_cast<ARRAY_TYPE>(index);
    }
};

class CQ{
    public:
    
    CQ(){}

    map<UInt64, PageTracker> pageTracker;
    list<UInt64> cir_queue;

    void merge(UInt64 va){
        for(auto begin=cir_queue.begin(); begin!=cir_queue.end(); begin++){
            if(*begin == va) 
            {
                cir_queue.erase(begin);
                break;
            }
        }
        cir_queue.push_back(va);
    }

    void insert(UInt64 page, UInt64 va)
    {
        // global queue
        if(cir_queue.size() >= 20){
            merge(va);
            find_dist();
            cir_queue.pop_front();
        }
       
        merge(va);
        if(cir_queue.size()>1){
            find_dist();
        }
    }

    void find_dist(){
        
        // scanning thoughr queue
        list<UInt64>::iterator end = cir_queue.end();
        end--;

        for(auto begin=cir_queue.begin(); begin!= end; begin++)
        {   
            UInt64 page1 = *begin >> MACRO_LOG_PAGE_SIZE;

            // recent pushed page
            UInt64 back = cir_queue.back();
            UInt64 page2 = back >> MACRO_LOG_PAGE_SIZE;

            if(page1 != page2){
                continue;
            }

            UInt64 po1 = *begin & MACRO_PAGE_SIZE;
            UInt64 po2 = back & MACRO_PAGE_SIZE;

            int diff = po1 - po2;
            if(diff <0) diff=-1*diff; 
            diff = diff/4;

            // page size is 12 bit= 6 bit for page block + 6 bit for offset within page block
            UInt64 page_block_index_1 = po1 >> MACRO_LOG_BLOCK_SIZE_BITS;
            UInt64 page_block_index_2 = po2 >> MACRO_LOG_BLOCK_SIZE_BITS;

            int diff2 = page_block_index_1 - page_block_index_2;
            if(diff2<0) diff2 = -1* diff2;
            diff2 /= 4;

            // if(diff2 == 0 && pageTracker[page1].actor<2)
            // {
            //     pageTracker[page1].actor++;
            // }
            // if(diff2 == 0 && pageTracker[page1].actor>=2)
            // {
            //     pageTracker[page1].ac
            // }

            // as there will be only 64 blocks, just make sure not go out of bounds
            UInt64 mod1 = page_block_index_1%64;
            UInt64 mod2 = page_block_index_2%64;

            // TODO: if we were out of bounds then stop and investigate: should not be case
            if(mod1!=page_block_index_1 || mod2!=page_block_index_2){
                cout << "*********************should not happen************************\n";
                exit(0);
                continue;
            }

            // tracking access to page block, for our modeled app, it should be one: if we do k-repeated loop access,
            // then it should be k times
            // only doing it for mod2 as these is recent request
            pageTracker[page1].count_access[mod2] += 1;

            pageTracker[page1].offsets[diff] += 1;

            
            // if(diff == 11){ 
            //     pageTracker[page1].set_type(mod2, ARRAY_TYPE::OFFSET_ARRAY);
            // }
            // if(diff == 13){
            //     pageTracker[page1].set_type(mod2, ARRAY_TYPE::EDGE_ARRAY);
            // }

            pageTracker[page1].set_type(diff, mod1);
                  
        }
    }
};

class Data
{
    public:
    int count;
    int cycle;

    set<int> distance;
    
    Data(){}
    Data(int cycle){
        count=1;
        this->cycle=cycle;
    }

    void add_distance(int curr_cycle)
    {
        int dis = this->cycle - curr_cycle;
        dis = abs(dis);
        distance.insert(dis);
        this->cycle = curr_cycle;
    }
};

class TLBSpy{

    public:
    TLBSpy(){
        cq = CQ();
    }

    map<string, Data> va2pa_map;
    map<UInt64, Data> page_data;
    vector<pair<int, UInt64>> trace;
    CQ cq;

    int cycle=0;

    void insert(UInt64 va, UInt64 pa){
        cycle++;

        UInt64 vp = va >> MACRO_LOG_PAGE_SIZE;
        UInt64 pp = pa >> MACRO_LOG_PAGE_SIZE;

        string s = to_string(va)+","+to_string(pa);
      
        if(va2pa_map.find(s)!=va2pa_map.end()){
            va2pa_map[s].count++;
            va2pa_map[s].add_distance(cycle);
        }
        else{
            va2pa_map.insert({s, Data(cycle)});
        }

        if(page_data.find(vp)!=page_data.end())
        {
            page_data[vp].add_distance(cycle);
        }
        else{
            page_data.insert({vp, Data(cycle)});
        }
        
        trace.push_back({cycle, va});

        cq.insert(vp, va);
    }

    void print(){
        fstream f;
        f.open("va2pa_map.log", fstream::out);
        for(auto e: va2pa_map){
            f <<  e.fi << "," << std::dec << e.se.count << "," << e.se.cycle <<'\n';
        }

        fstream b;
        b.open("page_block_classify.log", fstream::out);
        for(auto entry: cq.pageTracker){
            b << entry.first << ",";
            for(auto e: entry.second.blockType){
                b << to_string(static_cast<int>(e)) << ",";
            }
            b << '\n';
        }

        fstream c;
        c.open("page_block_count.log", fstream::out);
        for(auto entry: cq.pageTracker){
            c << entry.first << ",";
            for(auto e: entry.second.count_access){
                c << e << ",";
            }
            c << '\n';
        }

        fstream d;
        d.open("page_type.log", fstream::out);
        for(auto e: cq.pageTracker){
            d << std::hex << e.first << std::dec << ',' << e.second.page_type[0] << ',' << e.second.page_type[1] << '\n';
        }

        fstream e;
        e.open("page_distance.log", fstream::out);
        for(auto t: page_data){
            for(auto y: t.second.distance)
            {
               e << std::hex << t.first <<","<< std::dec << y << '\n'; 
            }
        }

        fstream fs;
        fs.open("addr_distance.log", fstream::out);
        for(auto t: va2pa_map){
            fs << std::hex << t.fi << ',';
            for(auto y: t.second.distance)
            {
               fs << std::dec << y << ','; 
            }
            fs << '\n';
        }

        fstream g;
        g.open("trace.log", fstream::out);
        for(auto e: trace)
        {
            g << e.fi << "," << std::hex << e.se << '\n';
        }
    }

};

#endif