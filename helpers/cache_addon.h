#ifndef CACHE_ADDON
#define CACHE_ADDON

#include "fixed_types.h"
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <algorithm>
#include "pqueue.h"
#include "lock.h"
#include "helpers.h"

namespace CacheAddonSpace
{   
   
    class AddressHistory
    {
        Helper::Counter totalpcCounter;
        std::unordered_map<IntPtr,Helper::Counter*> addressCount;
        public:
        AddressHistory(IntPtr addr){
            addressCount[addr]=new Helper::Counter();
        }
        UInt64 insert(IntPtr addr);// true; counting for new entries of memory address only
        Helper::Counter* find(IntPtr addr){
            std::unordered_map<IntPtr,Helper::Counter*>::iterator it=addressCount.find(addr);
            if(it!=addressCount.end())
                return it->second;
            return nullptr;
        }
        void deleteEntry(IntPtr addr){
            addressCount.erase(addressCount.find(addr));
        }
        std::unordered_map<IntPtr,Helper::Counter*> getAddresses(){return addressCount;}
        UInt64 getTotalAccessCount(){return totalpcCounter.getCount();}
    };

    class PCMisc
    {
        public:
        Helper::Counter addressCounter;
        Helper::Counter pcCounter;
        Helper::Counter refreshCounter;
        UInt64 prevCycleNumber;
        
        UInt64 getPCCount(){return pcCounter.getCount();}//keep count of new pc only
        UInt64 getAddrCount(){return addressCounter.getCount();}//keep count of new addresses only
        virtual std::unordered_set<IntPtr> action(UInt64 currCycleNumber)=0;
    };

    class PCHistoryTable: virtual public PCMisc
    {
        std::unordered_map<IntPtr, AddressHistory> table;
        Lock *lock;
        UInt64 prevCycleNumber=0;
        UInt64 epocDuration;
        public:
        PCHistoryTable(UInt64 epoc){
            this->epocDuration=epoc;
        }
        void insert(IntPtr pc, IntPtr addr);
        std::unordered_set<IntPtr> action(UInt64 currCycleNumber){

            std::unordered_set<IntPtr> allRemovableAddresses;

            UInt64 modCycle = currCycleNumber%10000;

            if(modCycle<prevCycleNumber)
            {
                //erasing some pc entry
                std::vector<std::pair<UInt64,IntPtr>> allPc;// count-pc
                for(auto e: table)
                {
                    allPc.push_back({e.second.getTotalAccessCount(), e.first});
                }
                std::sort(allPc.begin(),allPc.end());

                for(auto e: allPc)//count-pc
                {
                    std::unordered_map<IntPtr, AddressHistory>::iterator it = table.find(e.second);//pc
                    if(it!=table.end())
                    {
                        for(auto p: it->second.getAddresses())
                        {
                            allRemovableAddresses.insert(p.first);
                        }
                    }
                    table.erase(it);//pc
                }
                refreshCounter.reset();
            }
            
            prevCycleNumber=modCycle;
            return allRemovableAddresses;
        }

    };
};
#endif