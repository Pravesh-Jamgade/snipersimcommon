#ifndef DEAD_BLOCK_ANALYSIS
#define DEAD_BLOCK_ANALYSIS

#include<iostream>
#include<unordered_map>
#include"helpers.h"
#include"util.h"

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
    // typedef std::unordered_map<IntPtr, CBUsage> CBTracker;
    // std::unordered_map<uint64_t, CBTracker>cbTracker;
    // std::unordered_map<uint64_t, std::string> cbName;

    std::unordered_map<std::string, CBUsage> cbTracker;

    std::hash<std::string> hashFunc;
    
    String output_dir="";
    public:

    CacheBlockTracker(String output_dir){
        this->output_dir = output_dir;
    }

    ~CacheBlockTracker(){
        // String cbtInfo="cbt_log.out";

        // // for(auto cacheTracker: cbTracker){
        //     // String cache=&(getKeyName(cacheTracker.first))[0];
        //     // cache = '/'+cache+'_'+cbtInfo;
        //     String path = output_dir+cbtInfo;
        //     std::fstream cbStream;
        //     cbStream.open(path.c_str(), std::ofstream::out);

        //     for(auto cb: cbTracker){
        //         IntPtr addr = cb.first;
        //         UInt64 cbAccess = cb.second.cacheBlockAccess.getCount();
        //         UInt64 cbReuse = cb.second.cacheBlockReuse.getCount();
        //         UInt64 cbLHH = cb.second.cacheBlockLowerHalfHits.getCount();
        //         cbStream<<addr<<','<<cbAccess<<','<<cbReuse<<','<<cbLHH<<'\n';
        //     }
        //     cbStream.close();
        // // }
    }

    bool foundAddr(std::string  addr){
        return cbTracker.find(addr)!=cbTracker.end();
    }
    void increaseCBA(std::string  addr){cbTracker[addr].increaseCBA();}
    void increaseCBR(std::string  addr){cbTracker[addr].increaseCBR();}
    void increaseCBLHH(std::string  addr){cbTracker[addr].increaseCBLHH();}
    bool IsItkickedAlready(std::string  addr){cbTracker[addr].IsItKickedAlready();}
    bool setkickedFistTime(std::string  addr){cbTracker[addr].kickedFirstTime();}
    bool cacheEntryFound(std::string addr){
        auto findAddr = cbTracker.find(addr);
        if(findAddr!=cbTracker.end())
            return true;
        return false;
    }

    void doCBUsageTracking(IntPtr addr, bool recenyPos, String name){
        std::string s(begin(name), end(name));

        String haddr = Util::Misc::toHex(addr);
        std::string shaddr(begin(haddr), end(haddr));
        auto findAddr = cbTracker.find(shaddr);
        if(findAddr!=cbTracker.end())
            cbTracker[shaddr]=CBUsage();

        if(IsItkickedAlready(shaddr)){
            increaseCBR(shaddr);
        }
        else{
            increaseCBA(shaddr);
            // make count if addr is in the lower half of recency list
            if(recenyPos)
                increaseCBLHH(shaddr);
        }
    }

    void setFirstTimeEvict(String name, IntPtr* paddr){
        std::string s(begin(name), end(name));
        uint64_t hashkey = hashFunc(s);
        IntPtr addr = *paddr;
        String haddr = Util::Misc::toHex(addr);
        std::string shaddr(begin(haddr), end(haddr));
        if(!cacheEntryFound(shaddr))
            return ;        

        if(foundAddr( shaddr)){
            setkickedFistTime(shaddr);
        }
        else{
            printf("[DBA]Something Wrong!!\n");
            exit(0);
        }
    }
 };
}
#endif