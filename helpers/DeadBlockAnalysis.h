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
        Helper::Counter cacheBlockAccess, cacheBlockLowerHalfHits, cacheBlockReuse, cacheBlockEvict, deadBlock;
        bool cacheBlockEvictedFirstTime;
        UInt64 loadCycle, evictCycle, lastAccessCycle;
        double percentage;
        CBUsage(){
            cacheBlockEvictedFirstTime=false;
            cacheBlockAccess=Helper::Counter(1);
            deadBlock=cacheBlockEvict=cacheBlockLowerHalfHits=cacheBlockReuse=Helper::Counter(0);
            loadCycle=evictCycle=lastAccessCycle=0;
            percentage=0;
        }

        void increaseCBA(){cacheBlockAccess.increase();}
        void increaseCBR(){cacheBlockReuse.increase();}
        void increaseCBLHH(){cacheBlockLowerHalfHits.increase();}
        void increaseCBE(){cacheBlockEvict.increase();}
        bool IsItKickedAlready(){ return cacheBlockEvictedFirstTime;}
        void setEvictFirstTime(){
            cacheBlockEvictedFirstTime=1;
        }
        bool check(){
            UInt64 total = evictCycle-loadCycle;
            UInt64 tillLastAccess = lastAccessCycle-loadCycle;
            double percentage = (double)tillLastAccess/(double)total;
            bool isItDeadBlock =percentage>0.500001?true:false;
            if(isItDeadBlock)
                deadBlock.increase();
            this->percentage+=percentage;
            return isItDeadBlock;
        }
        double avgDeadPercent(){
            return this->percentage/(double)cacheBlockEvict.getCount();
        }
        void setEvictCycle(UInt64 cycle){
            evictCycle=cycle;
        }
        void setLastAccessCycle(UInt64 cycle){
            lastAccessCycle=cycle;
        }
        void setLoadCycle(UInt64 cycle){
            loadCycle=cycle;
        }
    };

    class CacheBlockTracker
    {
        std::unordered_map<IntPtr, CBUsage> cbTracker;
        Helper::Counter totalDeadBlocks, totalBlocks;
        public:

        CacheBlockTracker(String path, String name){
            totalBlocks=totalDeadBlocks=Helper::Counter(0);
            _LOG_CUSTOM_LOGGER(Log::Warning, getProperLog(name), "epoc,addr,lhh,evicts,reuse,access,dead\n");
        }

        Log::LogFileName getProperLog(String name){
            if(name == "L1-D")
                return Log::L1;
            if(name == "L2")
                return Log::L2;
            if(name == "L3")
                return Log::L3;
        }

        void logAndClear(String name, UInt64 epoc=0){
           Log::LogFileName log = getProperLog(name);
            for(auto addr: cbTracker){
                if(addr.second.deadBlock.getCount() == 0)
                    continue;
                _LOG_CUSTOM_LOGGER(Log::Warning, log, "%ld,%ld,%ld,%ld,%ld,%ld,%ld,%f\n", 
                    epoc,
                    addr.first, 
                    addr.second.cacheBlockLowerHalfHits.getCount(),
                    addr.second.cacheBlockEvict.getCount(),
                    addr.second.cacheBlockReuse.getCount(),
                    addr.second.cacheBlockAccess.getCount(),
                    addr.second.deadBlock.getCount(),
                    addr.second.avgDeadPercent()
                    );
            }
            _LOG_CUSTOM_LOGGER(Log::Warning, log, "%ld,%ld\n", totalDeadBlocks.getCount(), totalBlocks.getCount());
            _LOG_CUSTOM_LOGGER(Log::Warning, log, "--------------------------------------\n");
            cbTracker.clear();
        }

        void addEntry(IntPtr addr, bool pos, UInt64 cycle, bool eviction=false){
            
            auto findAddr = cbTracker.find(addr);
            if(findAddr!=cbTracker.end()){
                if(eviction){
                    findAddr->second.setEvictFirstTime();
                    findAddr->second.setEvictCycle(cycle);
                    findAddr->second.increaseCBE();
                    // check if it is deadblock
                    if(findAddr->second.check()){
                        totalDeadBlocks.increase();
                    }
                    return;
                }
                else{//block is accessed more than once
                    if(findAddr->second.IsItKickedAlready()){
                        findAddr->second.increaseCBR();
                    }
                    findAddr->second.increaseCBA();
                    if(pos)
                        findAddr->second.increaseCBLHH();
                    findAddr->second.setLastAccessCycle(cycle);
                }
            }
            else{
                if(eviction){//entry is not present even then asked  for evicition could be error
                    return;
                }

                cbTracker.insert({addr, CBUsage()});
                totalBlocks.increase();// new added block counted
                if(pos)
                    cbTracker[addr].increaseCBLHH();
                cbTracker[addr].setLoadCycle(cycle);
            }
            
            
        }
    };

}
#endif