#ifndef DEAD_BLOCK_ANALYSIS
#define DEAD_BLOCK_ANALYSIS

#include<iostream>
#include<unordered_map>
#include"helpers.h"
#include"util.h"
#include "log.h"

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
    typedef std::unordered_map<String, CBUsage> CBTracker;
    std::unordered_map<String, CBTracker> cbTracker;

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
            String cache=cacheTracker.first;
            cache = '/'+cache+'_'+cbtInfo;
            String path = output_dir + cache;
            std::fstream cbStream;
            cbStream.open(path.c_str(), std::ofstream::out);

            for(auto cb: cacheTracker.second){
                String addr = cb.first;
                UInt64 cbAccess = cb.second.cacheBlockAccess.getCount();
                UInt64 cbReuse = cb.second.cacheBlockReuse.getCount();
                UInt64 cbLHH = cb.second.cacheBlockLowerHalfHits.getCount();
                cbStream<<addr<<','<<cbAccess<<','<<cbReuse<<','<<cbLHH<<'\n';
            }
            cbStream.close();
        }
    }

    void increaseCBA(String name, String  addr){cbTracker[name][addr].increaseCBA();}
    void increaseCBR(String name, String  addr){cbTracker[name][addr].increaseCBR();}
    void increaseCBLHH(String name, String  addr){cbTracker[name][addr].increaseCBLHH();}
    bool IsItkickedAlready(String name, String  addr){ 
        return cbTracker[name][addr].IsItKickedAlready();
    }
    bool setkickedFistTime(String name, String  addr){cbTracker[name][addr].kickedFirstTime();}
    bool cacheEntryFound(String name){
        auto findAddr = cbTracker.find(name);
        if(findAddr!=cbTracker.end())
            return true;
        return false;
    }

    bool addrEntryFound(String name, String addr){
        if(cbTracker[name].find(addr)!=cbTracker[name].end()){
            return true;
        }
        return false;
    }

    void doCBUsageTracking(IntPtr addr, bool recenyPos, String name){
        std::cout<<addr<<" "<<recenyPos<<" "<<name<<std::endl;

        String haddr = &(std::to_string(addr))[0];
        // String haddr = Util::Misc::toHex(addr);

        if(!cacheEntryFound(name)){
            cbTracker[name][haddr]=CBUsage();
            return;
        }

        if(!addrEntryFound(name,haddr)){
            cbTracker[name][haddr]=CBUsage();
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

    void setFirstTimeEvict(String name, IntPtr* paddr){

        IntPtr addr = *paddr;
        String haddr = Util::Misc::toHex(addr);
        
        if(!cacheEntryFound(name)){
            _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::DEBUG, "[Notfound CacheName]\n");
            return ;
        }

        if(addrEntryFound(name, haddr)){
            setkickedFistTime(name, haddr);
        }
        else{
            _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::DEBUG, "[Notfound Address]%s\n", haddr.c_str());
            return ;
        }
    }
 };
}
#endif