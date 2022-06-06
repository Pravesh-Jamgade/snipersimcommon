#ifndef DEAD_BLOCK_ANALYSIS
#define DEAD_BLOCK_ANALYSIS

#include<iostream>
#include<unordered_map>
#include"helpers.h"
#include"util.h"
#include "log.h"
#include<fstream>
#include <math.h>  

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
        bool inserted;
        UInt64 loadCycle, evictCycle, lastAccessCycle;
        double addPercent, mulPercent;
        UInt64 countInserts;
        std::map<UInt64, int> deadperiod;//deadperiod-length
        CBUsage(){
            cacheBlockEvictedFirstTime=false;
            cacheBlockAccess=Helper::Counter(1);
            deadBlock=cacheBlockEvict=cacheBlockLowerHalfHits=cacheBlockReuse=Helper::Counter(0);
            loadCycle=evictCycle=lastAccessCycle=0;
            addPercent=0;
            mulPercent=1;
            inserted=false;
        }

        void increaseCBA(){cacheBlockAccess.increase();}
        void increaseCBR(){cacheBlockReuse.increase();}
        void increaseCBLHH(){cacheBlockLowerHalfHits.increase();}
        void increaseCBE(){cacheBlockEvict.increase();}
        bool IsItKickedAlready(){ return cacheBlockEvictedFirstTime;}
        void setEvictFirstTime(){
            cacheBlockEvictedFirstTime=1;
        }
        void addDeadPeriod(){
            if(deadperiod.find(getDeadPeriod()) != deadperiod.end()){
                deadperiod[getDeadPeriod()]++;
            }
            else deadperiod[getDeadPeriod()]=1;
        }
        UInt64 getTotalLife(){ return evictCycle-loadCycle;}
        UInt64 getDeadPeriod(){ return evictCycle-lastAccessCycle;}
        double deadPercent(){return (double)getDeadPeriod()/(double)getTotalLife();}
        bool check(){
            double percentage = deadPercent();            
            deadBlock.increase();
            addPercent += percentage;
            mulPercent *= percentage;
            return true;
        }
        double geomean(){
            return pow(mulPercent, 1/deadBlock.getCount());
        }
        double mean(){
            return addPercent/(double)deadBlock.getCount();
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
        void setInsert(){
            inserted=true;
        }
        void resetInsert(){
            inserted=false;
        }
        void countInsertOperations(){
            countInserts++;
        }
    };

    class CacheBlockTracker
    {
        std::unordered_map<IntPtr, CBUsage> cbTracker;
        Helper::Counter totalDeadBlocks, totalBlocks;
        public:

        CacheBlockTracker(String path, String name){
            totalBlocks=totalDeadBlocks=Helper::Counter(0);
            _LOG_CUSTOM_LOGGER(Log::Warning, getProperLog(name), "epoc,addr,lhh,evicts,reuse,access,dead,gmean,mean,inserts%s\n", name.c_str());
        }

        Log::LogFileName getProperLog(String name){
            if(name == "L1-D")
                return Log::L1;
            if(name == "L2")
                return Log::L2;
            if(name == "L3")
                return Log::L3;
        }

        void logAndClear(String name, UInt64 cycle, UInt64 epoc=0){
           Log::LogFileName log = getProperLog(name);
           printf("[]log=%d", log);

            // before logging, check if any block is inserted but not evicted yet, set evict cycle for it
            for(auto addr: cbTracker){
                if(addr.second.inserted){
                    addr.second.resetInsert();
                    addr.second.setEvictCycle(cycle);
                    if(addr.second.check()){
                        totalDeadBlocks.increase();
                    }
                }
            }

            for(auto addr: cbTracker){
                if(addr.second.deadBlock.getCount() == 0)
                    continue;
                _LOG_CUSTOM_LOGGER(Log::Warning, static_cast<Log::LogFileName>(log), "%ld,%ld,%ld,%ld,%ld,%ld,%ld,%f,%f,%ld\n", 
                        epoc,
                        addr.first, 
                        addr.second.cacheBlockLowerHalfHits.getCount(),
                        addr.second.cacheBlockEvict.getCount(),
                        addr.second.cacheBlockReuse.getCount(),
                        addr.second.cacheBlockAccess.getCount(),
                        addr.second.deadBlock.getCount(),
                        addr.second.geomean(),
                        addr.second.mean(),
                        addr.second.countInserts
                    );
                
                for(auto deadPeriod: addr.second.deadperiod){
                    _LOG_CUSTOM_LOGGER(Log::Warning, Log::DEBUG, "%ld, %ld, %ld, %d\n", epoc, addr.first, deadPeriod.first, deadPeriod.second);
                }
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
                    findAddr->second.resetInsert();
                    findAddr->second.addDeadPeriod();
                    // check if it is deadblock
                    if(findAddr->second.check()){
                        totalDeadBlocks.increase();
                    }
                    return;
                }
                else{
                    //block is accessed more than once
                    if(findAddr->second.IsItKickedAlready()){
                        findAddr->second.increaseCBR();
                    }

                    // reinserted cache block 
                    if(!findAddr->second.inserted){//false then block was evicted and access came again and reinserted
                        findAddr->second.setInsert();
                        cbTracker[addr].countInsertOperations();
                        cbTracker[addr].setLoadCycle(cycle);
                    }
                    
                    // keeping track of all accesses irrespective of how many time reinserted and evicted
                    findAddr->second.increaseCBA();

                    // keeping track of lower level recency hits
                    if(pos)
                        findAddr->second.increaseCBLHH();

                    // keeping track of last access cycle
                    findAddr->second.setLastAccessCycle(cycle);
                }
            }
            else{
                if(eviction){//entry is not present even then asked  for evicition could be error
                    return;
                }

                cbTracker.insert({addr, CBUsage()});
                // new added block counted
                totalBlocks.increase();
                // count lower level hits of recency list
                if(pos)
                    cbTracker[addr].increaseCBLHH();
                // set load cycle for newly inserted block
                cbTracker[addr].setLoadCycle(cycle);
                // set inserted status true
                cbTracker[addr].setInsert();
                cbTracker[addr].countInsertOperations();
            }
            
            
        }
    };

}
#endif