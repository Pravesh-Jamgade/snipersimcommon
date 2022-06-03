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
        CBUsage(){
            cacheBlockEvictedFirstTime=false;
            cacheBlockAccess=Helper::Counter(1);
            deadBlock=cacheBlockEvict=cacheBlockLowerHalfHits=cacheBlockReuse=Helper::Counter(0);
            loadCycle=evictCycle=lastAccessCycle=0;
        }

        void increaseCBA(){cacheBlockAccess.increase();}
        void increaseCBR(){cacheBlockReuse.increase();}
        void increaseCBLHH(){cacheBlockLowerHalfHits.increase();}
        void increaseCBE(){cacheBlockEvict.increase();}
        bool IsItKickedAlready(){ return cacheBlockEvictedFirstTime;}
        bool kickedFirstTime(){cacheBlockEvictedFirstTime=true; increaseCBE();}
        bool check(){
            UInt64 total = evictCycle-loadCycle;
            UInt64 tillLastAccess = evictCycle-lastAccessCycle;
            double percentage = (double)tillLastAccess/(double)total;
            bool isItDeadBlock =percentage>0.500001?true:false;
            if(isItDeadBlock)
                deadBlock.increase();
                
            return isItDeadBlock;
        }
        void setEvictCycle(UInt64 cycle){
            evictCycle=cycle;
        }
        void setLastAccessCycle(UInt64 cycle){
            lastAccessCycle=cycle;
        }
        void setLoadCycle(UInt64 cycle){
            evictCycle=lastAccessCycle=loadCycle=cycle;
        }
    };

    class CacheBlockTracker
    {
        std::unordered_map<IntPtr, CBUsage> cbTracker;
        Helper::Counter totalDeadBlocks, totalBlocks;
        public:

        CacheBlockTracker(String path){
            totalBlocks=totalDeadBlocks=Helper::Counter(0);
            _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::DBA, "epoc,addr,lhh,evicts,reuse,access,dead\n");
        }

        void logAndClear(UInt64 epoc){
           
            for(auto addr: cbTracker){
                if(addr.second.deadBlock.getCount() == 0)
                    continue;
                _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::DBA, "%ld,%ld,%ld,%ld,%ld,%ld,%ld\n", 
                    epoc,
                    addr.first, 
                    addr.second.cacheBlockLowerHalfHits.getCount(),
                    addr.second.cacheBlockEvict.getCount(),
                    addr.second.cacheBlockReuse.getCount(),
                    addr.second.cacheBlockAccess.getCount(),
                    addr.second.deadBlock.getCount()
                    );
            }
            cbTracker.clear();
        }

        void addEntry(IntPtr addr, bool pos, UInt64 cycle, bool eviction=false){

            auto findAddr = cbTracker.find(addr);
            if(findAddr!=cbTracker.end()){
                if(eviction){
                    findAddr->second.kickedFirstTime();
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