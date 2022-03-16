#include "cache_addon.h"
#include<iostream>
#include<unordered_map>
using namespace CacheAddonSpace;

void PCHistoryTable::insert(IntPtr pc, IntPtr addr)
{
    lock->acquire();
    std::unordered_map<IntPtr,AddressHistory>::const_iterator const_it=table.find(pc);
    if(const_it!=table.end())
    {
        AddressHistory addrh=const_it->second;
        if(addrh.insert(addr))
            addressCounter.increase();// only counting new addr 
    }
    else 
    {
        table.insert({pc,AddressHistory(addr)});
        pcCounter.increase();// only counting new pc entry
    }
    lock->release();
}

bool AddressHistory::insert(IntPtr addr)
{
    std::unordered_map<IntPtr, Counter>::const_iterator const_it=addressCount.find(addr);
    if(const_it!=addressCount.end())
    {
        Counter count=const_it->second;
        count.increase();
        return false;
    }
    else
    {
        addressCount.insert({addr,Counter()});
        return true;
    }
}