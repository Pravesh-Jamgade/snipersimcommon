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
        // particular pc how many times been used;
        UInt64 totalpcCounter;
        std::map<IntPtr,UInt64> addressCount;
        public:
        AddressHistory(IntPtr addr){
            addressCount = std::map<IntPtr,UInt64>();
            addressCount.emplace(addr, 1);
        }
        ~AddressHistory(){
            addressCount.clear();
        }
        bool insert(IntPtr addr);// true; counting for new entries of memory address only
        std::map<IntPtr,UInt64>& getAddresses(){return addressCount;}
        UInt64 getTotalAccessCount(){return totalpcCounter;}
        // increase variable
        void increase(UInt64& counter){counter++;}
    };

    class PCHistoryTable
    {
        std::map<IntPtr, AddressHistory> table;
        
        public:
        PCHistoryTable(){
            table=std::map<IntPtr, AddressHistory>();
        }
        ~PCHistoryTable(){
            table.clear();
        }
        UInt64 prevCycleNumber=0;
        UInt64 epocDuration;
        // increase variable
        void increase(UInt64& counter){counter++;}
        // insert pc and addr entry
        void insert(IntPtr pc, IntPtr addr);
        // epoc
        std::unordered_set<IntPtr> action();
        
    };
};
#endif