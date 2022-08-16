#ifndef PCDUMP_H
#define PCDUMP_H

#include<iostream>
#include<unordered_map>
#include "fixed_types.h"

using namespace std;

class Dump{
    public:
    int miss,hit,access;
    string hexpc;

    Dump(){}
    Dump(IntPtr pc){
        miss=hit=access=0;
        std::stringstream ss;
        ss << std::hex << pc; 
        ss >> hexpc; 
        ss.clear();
    }

    void inc(bool cache_hit){
        access++;
        if(cache_hit)
            hit++;
        else miss++;
    }

    double missratio(){ return (double)miss/(double)access; }
};

class PCDump{

    public:
    unordered_map<IntPtr, Dump> info;

    // tracking PC
    void insert(IntPtr pc, bool cache_hit){
        auto findPC = info.find(pc);
        if(findPC!=info.end()){
            info[pc].inc(cache_hit);            
        }
        else{
            info.insert({pc, Dump(pc)});
            info[pc].inc(cache_hit);
        }
    }    
};

#endif