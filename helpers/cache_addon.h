#ifndef CACHE_ADDON
#define CACHE_ADDON
#include "fixed_types.h"
#include <iostream>
#include <vector>
#include <map>
#include <list>

namespace CacheAddonSpace
{   
    class AddressHistory
    {
        // count - address
        std::list<std::pair<UInt32,IntPtr>> addrAndCount;
        UInt32 cumulativeAccess=0;
        int entryLimit=10;
        public:

        bool valid(){ return addrAndCount.size()<entryLimit;}

        IntPtr addEntry(IntPtr addr);

        int getTotalAccessCount(){return cumulativeAccess;}

        std::list<std::pair<UInt32,IntPtr>> getAddress(){return addrAndCount;}
    };

    class PCHistoryTable
    {
        // pc - list(pair(count - address))
        std::map<IntPtr, AddressHistory*> table;
        int tableLimit=500;

        public:
        PCHistoryTable(){}
        ~PCHistoryTable(){}

        bool valid(){ return table.size()<tableLimit;}

        void addEntry(IntPtr pc, IntPtr addr, IntPtr& retPC, IntPtr& retAddr);

        std::list<std::pair<UInt32,IntPtr>> getAddress(IntPtr pc);

        void eraseEntry(IntPtr pc){
            std::map<IntPtr, AddressHistory*>::iterator it = table.find(pc);
            if(it!=table.end()){
                table.erase(pc);
            }
        }
    };
};
#endif