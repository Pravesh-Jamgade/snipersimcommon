#ifndef DEAD_BLOCK_ANALYSIS
#define DEAD_BLOCK_ANALYSIS

#include<iostream>
#include<unordered_map>
#include"helpers.h"
#include "log.h"
#include<fstream>
#include <math.h>  
#include "fixed_types.h"
#include<memory>

namespace DeadBlockAnalysisSpace
{
    // class Int2HexMap{
    //     public:
    //     static std::unordered_map<IntPtr, String> intMap;
    //     static String insert(IntPtr addr){
    //         auto findAddr = intMap.find(addr);
    //         if(findAddr==intMap.end()){
    //             String tmp = Util::Misc::toHex(addr);
    //             intMap.insert({addr, tmp});
    //             return tmp;
    //         }
    //         return findAddr->second;
    //     }
    // };

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
        ~CBUsage(){
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

    class KeyObject{
        String cache;
        int core;
        public:
        KeyObject(){}
        KeyObject(String cache, int core){
            this->cache=cache;
            this->core=core;
        }
        bool operator==(KeyObject* obj){
            if(this->cache==obj->cache && this->core==obj->core){
                return true;
            }
            return false;
        }
    };

    class CacheBlockTracker
    {   
        typedef std::map<IntPtr, std::shared_ptr<CBUsage>> CacheBlockUsage;
        std::map<String, CacheBlockUsage> cbTracker;
        std::map<String, UInt64> mmp;
        std::vector<CBUsage*> del;

        public:
        CacheBlockTracker(){
            cbTracker = std::map<String, CacheBlockUsage>();
            mmp = std::map<String, UInt64>();
        }

        void logAndClear(int core, UInt64 epoc, bool isShared){
            UInt64 dead = 0;
            
            for(auto cache : cbTracker){
                for(auto block : cache.second){
                    std::shared_ptr<CBUsage> cbUsage = block.second;
                    if(!cbUsage->isEvicted() && cbUsage->isLRUBlock()){
                        dead++;
                        auto findCache = mmp.find(cache.first);
                        if(findCache!=mmp.end())
                            mmp[cache.first]++;
                        else mmp[cache.first]=0;
                    }
                    else{
                    }
                }
            }

            printf("epoc=%ld,dead=%ld,shared=%d\n", epoc, dead, isShared);

            //cache-CacheBlockUsage
            for(auto cache: cbTracker){
                //log cache dead block
                //address-CBUsage
                for(auto cb : cache.second){
                    _LOG_CUSTOM_LOGGER(Log::Warning, static_cast<Log::LogFileName>(core), "%ld,%s,%ld,%d\n", 
                        epoc, cache.first.c_str(), cb.first, cb.second->isLRUBlock());
                }
            }

            // for(auto cache: cbTracker){
            //     for(auto cb: cache.second){
            //         cb.second.reset();
            //     }
            // }
            cbTracker.clear();
            mmp=std::map<String, UInt64>();
        }

        void addEntry(IntPtr addr, bool pos, UInt64 cycle, String name, bool eviction=false){
            auto findName = cbTracker.find(name);
            //cache found
            if(findName!=cbTracker.end()){
                //get cache blocks of addresses
                CacheBlockUsage* cacheBlockusage = &(findName->second);
                auto findAddr = cacheBlockusage->find(addr);
                
                std::shared_ptr<CBUsage> cbUsage = nullptr;
                if(findAddr!=cacheBlockusage->end()){
                    cbUsage = std::make_shared<CBUsage>();
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
                        cbUsage = std::make_shared<CBUsage>();
                        cacheBlockusage->insert({addr, cbUsage});
                    }
                }
            }
            else
            {
                CacheBlockUsage tmp;
                tmp.insert({addr, std::make_shared<CBUsage>()});
                cbTracker.insert({name, tmp});
            }
        }
    };

    class CBHelper{

        public:
        CacheBlockTracker local;
        std::shared_ptr<CacheBlockTracker> shared;

        CBHelper(){}
        CBHelper(std::shared_ptr<CacheBlockTracker>shCbTracker){
            local=CacheBlockTracker();
            shared=shCbTracker;
        }

        void addEntry(IntPtr addr, bool pos, UInt64 cycle, String name, bool eviction=false, bool isItShared=false){
            if(isItShared){
                local.addEntry(addr,pos,cycle,name,eviction);
            }
            else{
                shared->addEntry(addr,pos,cycle,name,eviction);
            }
        }

        void logAndClear(core_id_t core, UInt64 epoc=0){
            local.logAndClear(core,epoc,false);
            if(shared==nullptr)
                return;
            shared->logAndClear(0,epoc,true);
        }
        
    };
}
#endif