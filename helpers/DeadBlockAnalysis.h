#ifndef DEAD_BLOCK_ANALYSIS
#define DEAD_BLOCK_ANALYSIS

#include<iostream>
#include<unordered_map>
#include"helpers.h"
#include"util.h"
#include "log.h"
#include<fstream>
#include <math.h>  
#include "fixed_types.h"

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
        Helper::Counter cacheBlockAccess, cacheBlockLowerHalfHits, cacheBlockEvicts;
        UInt64 loadCycle, evictCycle, lastAccessCycle;
        double addPercent, mulPercent;
        std::map<UInt64, int> deadperiod;//deadperiod-length
        bool evicted;
        int pos;
        CBUsage(){
            cacheBlockEvicts=Helper::Counter(0);
            cacheBlockAccess=Helper::Counter(1);
            cacheBlockLowerHalfHits=Helper::Counter(0);
            loadCycle=evictCycle=lastAccessCycle=0;
            addPercent=0;
            mulPercent=1;
            evicted=false;
            pos=0;//lru pos
        }

        void increaseCBA(){cacheBlockAccess.increase();} UInt64 getAccess(){return cacheBlockAccess.getCount();}
        void increaseCBLHH(){cacheBlockLowerHalfHits.increase();} UInt64 getLLH(){return cacheBlockLowerHalfHits.getCount();}
        void increaseEvicts(){cacheBlockEvicts.increase();} UInt64 getEvicts(){return cacheBlockEvicts.getCount();}
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
        bool isDead(){return deadPercent()>5;}

        bool operator<(const CBUsage& c)const{
            return true;
        }

        bool isEvicted(){return evicted;}
        void setEvicted(){evicted=true;}
        void resetEvicted(){evicted=false;}

        void setPos(int pos){pos=pos;}
        bool isLRUBlock(){return pos;}
    };

    class CacheBlockTracker
    {   
        typedef std::map<IntPtr, CBUsage> CacheBlockUsage;
        typedef std::map<String, CacheBlockUsage> Cache_t;
        std::map<core_id_t, Cache_t> core_cbTracker;
        std::map<IntPtr, CBUsage> shared_cbTracker;
        UInt64 totalDeadBlocks, totalBlocks, totalEpoc;
        std::map<core_id_t, std::map<String, UInt32>> mmp;
        UINt64 deadBlocks;
        public:

        CacheBlockTracker(){
            
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
            
            for(auto core : core_cbTracker){
                
                // insert new core entry
                auto findCore = mmp.find(core->first);
                if(findCore==mmp.end()){
                    std:map<String, UInt32> mp;
                    mmp.insert({core, mp});
                }

                core_id_t core_id = core->first;

                std::map<String, UInt32>* cc = findCore->second;
                for(auto cache in core->second){
                    
                    String name = cache->first;

                    // insert new cache entry
                    auto findName = cc->find(name);
                    if(findName==cc->end()){
                        cc->insert({name, 0});
                    }

                    for(auto block in cache->second){
                        CBUsage cbUsage = block->second;
                        if(!cbUsage.isEvicted() && cbUsage.isLRUBlock()){
                            cc->at(name)++;
                        }
                    }
                }
            }
            
            for(auto addr : shared_cbTracker){
                
            }
        }

        void addEntry(IntPtr addr, bool pos, UInt64 cycle, core_id_t core_id, String name, bool shared, bool eviction=false){
            if(shared){
                auto findAddr = shared_cbTracker.find(addr);
                CBUsage* cbUsage = nullptr;
                if(findAddr!=shared_cbTracker.end()){
                    cbUsage=&findAddr->second;
                }

                if(cbUsage!=nullptr)
                    cbUsage->setPos(pos);
                
                if(eviction){
                    if(cbUsage==nullptr)
                        return;
                    cbUsage->setEvicted();
                    return;
                }
                else{
                    if(cbUsage!=nullptr){
                        cbUsage->increaseCBA();
                        // earlier it was evicted, with request now loaded
                        if(cbUsage->isEvicted()){
                            cbUsage->resetEvicted();
                        }
                    }
                    else{
                        cbUsage = new CBUsage();
                        shared_cbTracker.insert({addr, *cbUsage});
                    }
                }

            }
            else{
                auto findCore = core_cbTracker.find(addr);

                //core found
                if(findCore!=core_cbTracker.end()){
                    //get cores cache map
                    Cache_t *cache_x = &(findCore->second);
                    auto findName = cache_x->find(name);

                    //cache found
                    if(findName!=cache_x->end()){
                        //get cache blocks of addresses
                        CacheBlockUsage* cacheBlockusage = &(findName->second);
                        auto findAddr = cacheBlockusage->find(addr);
                        
                        CBUsage* cbUsage = nullptr;
                        if(findAddr!=cacheBlockusage->end()){
                            cbUsage = &(findAddr->second);
                        }
                        if(eviction){
                            if(cbUsage==nullptr)
                                return;
                            cbUsage->setEvicted();
                            return;
                        }
                        else{
                            if(cbUsage!=nullptr){
                                cbUsage->increaseCBA();
                                if(cbUsage->isEvicted()){
                                    cbUsage->resetEvicted();
                                }
                            }
                            else{
                                cbUsage = new CBUsage();
                                cacheBlockusage->insert({addr, *cbUsage});
                            }
                        }
                    }
                    else
                    {
                        CacheBlockUsage tmp;
                        tmp.insert({addr, CBUsage()});
                        cache_x->insert({name, tmp});
                    }
                }
                else{
                    std::map<IntPtr, CBUsage> cbUsage;
                    cbUsage.insert({addr, CBUsage()});

                    std::map<String, CacheBlockUsage> cache_name;
                    cache_name.insert({name, cbUsage});

                    core_cbTracker.insert({core_id, cache_name});
                }
            }            
        }
    };

}
#endif