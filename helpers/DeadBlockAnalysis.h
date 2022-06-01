#ifndef DEAD_BLOCK_ANALYSIS
#define DEAD_BLOCK_ANALYSIS

#include<iostream>
#include<unordered_map>
#include"helpers.h"
#include"util.h"
#include "log.h"
#include<fstream>

namespace DeadBlockAnalysisSpace
{
    class Int2HexMap{
        public:
        static std::unordered_map<IntPtr, String> intMap;
        static String insert(IntPtr addr){
            auto findAddr = intMap.find(addr);
            if(findAddr==intMap.end()){
                String tmp = Util::Misc::toHex(addr);
                intMap.insert({addr, tmp});
                return tmp;
            }
            return findAddr->second;
        }
    };


 class CBUsage
 {
    public:
    Helper::Counter cacheBlockAccess, cacheBlockLowerHalfHits, cacheBlockReuse, cacheBlockEvict;
    bool cacheBlockEvictedFirstTime;
    CBUsage(){
        cacheBlockEvictedFirstTime=false;
        cacheBlockAccess=Helper::Counter(1);
        cacheBlockEvict=cacheBlockLowerHalfHits=cacheBlockReuse=Helper::Counter(0);
    }

    void increaseCBA(){cacheBlockAccess.increase();}
    void increaseCBR(){cacheBlockReuse.increase();}
    void increaseCBLHH(){cacheBlockLowerHalfHits.increase();}
    void increaseCBE(){cacheBlockEvict.increase();}
    bool IsItKickedAlready(){ return cacheBlockEvictedFirstTime;}
    bool kickedFirstTime(){cacheBlockEvictedFirstTime=true; increaseCBE();}
 };

 class CacheBlockTracker
 {
    typedef std::unordered_map<IntPtr, CBUsage*> CBTracker;
    std::unordered_map<int, CBTracker> cbTracker;

    String output_dir="";
    public:

    CacheBlockTracker(){
    }

    CacheBlockTracker(String output_dir){
        this->output_dir = output_dir;
        std::cout<<"my output @ "<<output_dir<<'\n';
    }

    ~CacheBlockTracker(){
        String cbtInfo="cbt_log.out";

        for(auto cacheTracker: cbTracker){
            String cache=get(cacheTracker.first);
            cache = '/'+cache+'_'+cbtInfo;
            String path = output_dir + cache;
            std::fstream cbStream(path.c_str(), std::fstream::out);

            cbStream<<"addr,access,reuse,lhl,evicts\n";
            for(auto cb: cacheTracker.second){
                IntPtr haddr = cb.first;
                UInt64 cbAccess = cb.second->cacheBlockAccess.getCount();
                UInt64 cbReuse = cb.second->cacheBlockReuse.getCount();
                UInt64 cbLHH = cb.second->cacheBlockLowerHalfHits.getCount();
                UInt64 cbE = cb.second->cacheBlockEvict.getCount();
                cbStream<<haddr<<','<<cbAccess<<','<<cbReuse<<','<<cbLHH<<','<<cbE<<'\n';
                if(cb.second->IsItKickedAlready()){
                    printf("[A]%ld, %ld\n", haddr, cbE);
                }
            }
            cbStream.close();
        }
    }

    void increaseCBA(int name, IntPtr haddr){cbTracker[name][haddr]->increaseCBA();}
    void increaseCBR(int name, IntPtr haddr){cbTracker[name][haddr]->increaseCBR();}
    void increaseCBLHH(int name, IntPtr haddr){cbTracker[name][haddr]->increaseCBLHH();}
    void increaseCBE(int name, IntPtr haddr){cbTracker[name][haddr]->increaseCBE();}
    bool IsItkickedAlready(int name, IntPtr haddr){ 
        return cbTracker[name][haddr]->IsItKickedAlready();
    }
    bool cacheEntryFound(int name){
        if(cbTracker.find(name)!=cbTracker.end()){
            return true;
        }
        return false;
    }

    bool addrEntryFound(int name, IntPtr haddr){
        if(cbTracker[name].find(haddr)!=cbTracker[name].end()){
            return true;
        }
        return false;
    }

    int get(String name){
        if(name == "L1-D") return 0;
        if(name == "L1-I") return 1;
        if(name == "L2") return 2;
        if(name == "L3") return 3;
    }

    String get(int num){
        if(num == 0) return "L1-D";
        if(num == 1) return "L1-I";
        if(num == 2) return "L2";
        if(num == 3) return "L3";
    }

    void doCBUsageTracking(IntPtr haddr, bool recenyPos, String cacheName, bool eviction=false){
        int name= get(cacheName);
       
        if(eviction)
        {
            auto findCacheName = cbTracker.find(name);
            if(findCacheName!=cbTracker.end()){
                auto findAddr = findCacheName->second.find(haddr);
                if(findAddr!=findCacheName->second.end()){
                    //name,addr found
                    findAddr->second->kickedFirstTime();
                    printf("[**] kick=%ld\n", haddr);
                }
            }
            return;
        }

        if(!cacheEntryFound(name)){
            cbTracker[name].insert({haddr, new CBUsage()});
            return;
        }

        if(!addrEntryFound(name,haddr)){
            cbTracker[name].insert({haddr, new CBUsage()});
            return;
        }

        if(IsItkickedAlready(name, haddr)){
            increaseCBR(name, haddr);
        }
        else{
            increaseCBA(name, haddr);
            // make count if addr is in the lower half of recency list
            if(recenyPos)
                increaseCBLHH(name, haddr);
        }
    }
 };

}
#endif