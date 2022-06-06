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
        UInt64 step;
        Helper::Counter cacheBlockAccess, cacheBlockLowerHalfHits;
        UInt64 loadCycle, evictCycle, lastAccessCycle;
        double addPercent, mulPercent;
        std::map<UInt64, int> deadperiod;//deadperiod-length
        CBUsage(){
            cacheBlockAccess=Helper::Counter(1);
            cacheBlockLowerHalfHits=Helper::Counter(0);
            loadCycle=evictCycle=lastAccessCycle=0;
            addPercent=0;
            mulPercent=1;
            step=0;
        }

        void increaseCBA(){cacheBlockAccess.increase();}
        void increaseCBLHH(){cacheBlockLowerHalfHits.increase();}
        UInt64 getTotalLife(){ return evictCycle-loadCycle;}
        UInt64 getDeadPeriod(){ return evictCycle-lastAccessCycle;}
        double deadPercent(){return ((double)getDeadPeriod()/(double)getTotalLife())*100;}
        bool check(){
            double percentage = deadPercent();            
            addPercent += percentage;
            mulPercent *= percentage;
            return true;
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
        void insert(){step+=10;}
        void rereference(){step-=2;}
        bool isDead(){return deadPercent()>5;}

        bool operator<(const CBUsage& c)const{
            return true;
        }
    };

    class CacheBlockTracker
    {
        std::unordered_map<IntPtr, CBUsage> cbTracker;
        std::unordered_map<IntPtr, std::set<CBUsage>> dbTracker;
        Helper::Counter totalDeadBlocks, totalBlocks;

        public:
        double geomean(double value){
            return pow(value, 1/dbTracker.size());
        }
        double mean(double value){
            return value/(double)dbTracker.size();
        }

        CacheBlockTracker(String path, String name){
            totalBlocks=totalDeadBlocks=Helper::Counter(0);
            _LOG_CUSTOM_LOGGER(Log::Warning, getProperLog(name), "addr,#deadblocks%s\n", name.c_str());
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

           for(auto deadBlock: dbTracker){
               _LOG_CUSTOM_LOGGER(Log::Warning, static_cast<Log::LogFileName>(log), "%ld,%ld\n",
                    deadBlock.first,
                    deadBlock.second.size());
           }
           _LOG_CUSTOM_LOGGER(Log::Warning, static_cast<Log::LogFileName>(log), "total=%ld,dead=%ld\n",
                    totalBlocks.getCount(),
                    totalDeadBlocks.getCount());
           
        }

        void addEntry(IntPtr addr, bool pos, UInt64 cycle, bool eviction=false){
            auto findAddr = cbTracker.find(addr);
            CBUsage* cbUsage=nullptr;
            if(findAddr!=cbTracker.end()){
                cbUsage=&findAddr->second;
            }

            if(eviction){
                if(cbUsage==nullptr)
                    return;
                cbUsage->setEvictCycle(cycle);
                cbUsage->check();
                cbTracker.erase(findAddr);
                if(cbUsage->isDead()){
                    auto dbFind = dbTracker.find(addr);
                    if(dbFind!=dbTracker.end())
                        dbTracker[addr].insert(*cbUsage);
                    else
                        dbTracker.insert({addr, {*cbUsage}});
                    totalDeadBlocks.increase();
                }
                return;
            }
            
            if(cbUsage!=nullptr){
                cbUsage->rereference();
                cbUsage->setLastAccessCycle(cycle);
                cbUsage->increaseCBA();
            }
            else{
                cbUsage = new CBUsage();
                cbUsage->setLoadCycle(cycle);
                cbUsage->insert();
                cbTracker[addr]=*cbUsage;
                totalBlocks.increase();
            }

            if(pos)
                cbTracker[addr].increaseCBLHH();
        }
    };

}
#endif