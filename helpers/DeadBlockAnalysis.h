#ifndef DEAD_BLOCK_ANALYSIS
#define DEAD_BLOCK_ANALYSIS

#include<iostream>
#include<unordered_map>
#include"helpers.h"

namespace DeadBlockAnalysisSpace
{
 class CBUsage
 {
    public:
    Helper::Counter cacheBlockAccess,cacheBlockLowerHalfHits,cacheBlockReuse;
    bool cacheBlockEvictedFirstTime;
    CBUsage(){
        cacheBlockAccess=false;
        cacheBlockAccess=Helper::Counter(1);
        cacheBlockLowerHalfHits=cacheBlockReuse=Helper::Counter(0);
    }

    void increaseCBA(){cacheBlockAccess.increase();}
    void increaseCBR(){cacheBlockReuse.increase();}
    void increaseCBLHH(){cacheBlockLowerHalfHits.increase();}
    bool IsItKickedAlready(){ return cacheBlockEvictedFirstTime;}
    bool kickedFirstTime(){cacheBlockEvictedFirstTime=true;}
 };

 class CacheBlockTracker
 {
    typedef std::unordered_map<IntPtr, CBUsage> CBTracker;
    std::unordered_map<String, CBTracker>cbTracker;
    
    String output_dir="";
    public:
    CacheBlockTracker(String output_dir){
        this->output_dir = output_dir;
    }
    ~CacheBlockTracker(){
        String cbtInfo="cbt_log.out";

        for(auto cacheTracker: cbTracker){
            String cache=cacheTracker.first;
            cache = '/'+cache+'_'+cbtInfo;
            String path = output_dir+cbtInfo;
            std::fstream cbStream;
            cbStream.open(path.c_str(), std::ofstream::out);

            for(auto cb: cacheTracker.second){
                IntPtr addr = cb.first;
                UInt64 cbAccess = cb.second.cacheBlockAccess.getCount();
                UInt64 cbReuse = cb.second.cacheBlockReuse.getCount();
                UInt64 cbLHH = cb.second.cacheBlockLowerHalfHits.getCount();
                cbStream<<addr<<','<<cbAccess<<','<<cbReuse<<','<<cbLHH<<'\n';
            }
            cbStream.close();
        }
        
        
    }
    bool foundAddr(String name, IntPtr Addr){
        return cbTracker[name].find(Addr)!=cbTracker[name].end();
    }
    void increaseCBA(String name, IntPtr addr){cbTracker[name][addr].increaseCBA();}
    void increaseCBR(String name, IntPtr addr){cbTracker[name][addr].increaseCBR();}
    void increaseCBLHH(String name, IntPtr addr){cbTracker[name][addr].increaseCBLHH();}
    bool IsItkickedAlready(String name, IntPtr addr){cbTracker[name][addr].IsItKickedAlready();}
    bool setkickedFistTime(String name, IntPtr addr){cbTracker[name][addr].kickedFirstTime();}
    bool cacheEntryFound(String name){
        return cbTracker.find(name)!=cbTracker.end();
    }
    void doCBUsageTracking(IntPtr addr, bool recenyPos, String name){
        if(!cacheEntryFound(name)){
            cbTracker[name][addr]=CBUsage();
        }

        if(IsItkickedAlready(name,addr)){
            increaseCBR(name,addr);
        }
        else{
            increaseCBA(name,addr);
            // make count if addr is in the lower half of recency list
            if(recenyPos)
                increaseCBLHH(name,addr);
        }
    }

    void setFirstTimeEvict(String name, IntPtr* paddr){
        IntPtr addr = *paddr;

        if(!cacheEntryFound(name))
            return ;        

        if(foundAddr(name, addr)){
            setkickedFistTime(name, addr);
        }
        else{
            printf("[DBA]Something Wrong!!\n");
            exit(0);
        }
    }
 };
}
#endif